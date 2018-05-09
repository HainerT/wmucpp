/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2016, 2017, 2018 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define USE_TC1_AS_HARDPPM

#ifndef USE_TC1_AS_HARDPPM
# define USE_RPM2_ON_OPTO2
#endif

#define USE_TC0_AS_SWUSART

//#define USE_PPM_ON_OPTO2

//#ifdef USE_PPM_ON_OPTO2
//# define USE_ICP1
//# ifdef USE_RPM2_ON_OPTO2
//#  error "wrong config"
//# endif
//#endif

#define MEM
#define NDEBUG

#include "local.h"
#include "rcsensorled03c.h"
#include "console.h"
#include "util/meta.h"
#include "hal/eeprom.h"
#include "external/hott/menu.h"

#include "container/tree.h"

#include <vector>

template<typename F>
struct StringConverter;

template<uint8_t N>
struct StringConverter<FixedPoint<int16_t, N>> {
    template<uint8_t L>
    inline static FixedPoint<int16_t, N> parse(const StringBuffer<L>& str) {
        int16_t value = 0;
        int8_t decimals = -1;
        bool negative = false;
        
        uint8_t index = 0;
        
        if (str[0] == '-') {
            negative = true;
            ++index;
        }
        
        for(; index < L; ++index) {
            if (str[index] == '.') {
                decimals = 0;
            }
            else if ((str[index] >= '0') && (str[index] <= '9')) {
                value *= 10;
                value += (str[index] - '0');
                if (decimals >= 0) {
                    ++decimals;
                }
            }
            else if (str[index] == '\0') {
                break;
            }
        }
        value <<= N;
        while(decimals > 0) {
            value /= 10;
            --decimals;
        }
        if (negative) {
            value = -value;
        }
        return FixedPoint<int16_t, N>::fromRaw(value);
    }
};

using testPin1 = AVR::Pin<PortC, 0>;
using testPin2 = AVR::Pin<PortC, 1>;
using testPin3 = AVR::Pin<PortC, 2>;

struct Storage {
    enum class AVKey : uint8_t {TSensor1 = 0, TSensor2, RpmSensor1, RpmSensor2, Spannung1, Spannung2, 
                                StromOffset, PWM, PWMMOde, Leds1Channel, Leds1Sequence, Leds2Channel, Leds2Sequence, _Number};
    
    class ApplData : public EEProm::DataBase<ApplData> {
    public:
        uint_NaN<uint8_t>& operator[](AVKey key) {
            return AValues[static_cast<uint8_t>(key)];
        }
    private:
        std::array<uint_NaN<uint8_t>, static_cast<uint8_t>(AVKey::_Number)> AValues;
    };
    
    inline static constexpr uint8_t NumberOfOWireDevs = 4;
    inline static constexpr uint8_t NumberOfI2CDevs = 4;
    
    inline static std::vector<OneWire::ow_rom_t, NumberOfOWireDevs> dsIds;
    inline static std::vector<TWI::Address, NumberOfI2CDevs> i2cDevices;
    
    inline static std::array<FixedPoint<int, 4>, NumberOfOWireDevs> temps;
    inline static std::array<std::RPM, 2> rpms;
    
    inline static std::array<FixedPoint<int, 4>, 2> batts;
    inline static std::array<FixedPoint<int, 4>, 2> minCells;
    
    inline static std::array<FixedPoint<int, 4>, 2> currents;
    
    inline static uint8_t switches = 0;
};

using eeprom = EEProm::Controller<Storage::ApplData>;
auto& appData = eeprom::data();

namespace {
    constexpr bool useTerminal = true;
}

namespace Constants {
    static constexpr std::hertz pwmFrequency = 8000_Hz * 256; 
    static constexpr const std::hertz fSCL = 100000_Hz;
    //    static constexpr std::hertz pwmFrequency = 1000_Hz;
    static constexpr Color cRed{Red{32}};
    static constexpr Color cBlue{Blue{32}};
    static constexpr Color cGreen{Green{32}};
}

struct AsciiHandler;
struct BinaryHandler;
struct BCastHandler;

using sensorUsart = AVR::Usart<0, Hott::SensorProtocollAdapter<0, UseEvents<false>, AsciiHandler, BinaryHandler, BCastHandler>, 
MCU::UseInterrupts<true>, UseEvents<false>> ;
using rcUsart = AVR::Usart<1, Hott::SumDProtocollAdapter<0>, MCU::UseInterrupts<true>, UseEvents<false>>;

using terminalDevice = std::conditional<useTerminal, rcUsart, void>::type;
using terminal = std::basic_ostream<terminalDevice>;

using namespace std::literals::quantity;

struct I2CInterrupt : public IsrBaseHandler<AVR::ISR::Int<1>> {
    static void isr() {
    }
};
using i2cInterruptHandler = I2CInterrupt;

using isrRegistrar = IsrRegistrar<rcUsart::RxHandler, rcUsart::TxHandler, 
sensorUsart::RxHandler, sensorUsart::TxHandler
#ifdef USE_RPM2_ON_OPTO2
, softPpm::OCAHandler, softPpm::OCBHandler
#else
//, ppmDecoder
#endif
,i2cInterruptHandler
#ifdef USE_TC0_AS_SWUSART
, gpsUart::RxHandler, gpsUart::TxHandler, gpsUart::StartBitHandler
#endif
>;

using sensorData = Hott::SensorProtocollBuffer<0>;
using crWriterSensorBinary = ConstanteRateWriter<sensorData, sensorUsart>;
using menuData = Hott::SensorTextProtocollBuffer<0>;
using crWriterSensorText = ConstanteRateWriter<menuData, sensorUsart>;



void menuAction(uint8_t index, const auto& callable);

template<auto... II>
struct Indexes {};

template<typename IndexType = uint8_t>
class MenuItem {
    typedef IndexType index_type;
public:
    inline constexpr MenuItem() {}
    inline constexpr bool hasChildren() const {return false;}
    inline constexpr bool isSelected() const {return false;}
    
    constexpr inline uint_NaN<index_type> processKey(Hott::key_t) {return {};}
    constexpr inline void textTo(Hott::Display& ) const {}
    constexpr inline void putTextInto(Hott::BufferString& buffer) const {}
private:
    bool mSelected = false;
};

template<uint8_t Size = 7, typename IndexType = uint8_t>
class Menu {
public:
    typedef IndexType index_type;
    constexpr Menu(const PgmStringView& title, uint_NaN<IndexType> parent = uint_NaN<IndexType>{}, 
                   const std::array<IndexType, Size>& children = std::array<IndexType, 7>{}) : 
        mTitle{title}, mParent{parent}, mChildren{children} {}
    
    inline constexpr bool hasChildren() const {return true;}
    inline constexpr bool isSelected() const {return mSelected;}

    inline constexpr void titleInto(Hott::BufferString& buffer) const {
        buffer.insertAtFill(0, mTitle);
    }
    
    inline void putTextInto(Hott::BufferString& buffer) const {
        buffer[0] = ' ';
        buffer.insertAtFill(1, mTitle);
    }
    
    inline void textTo(Hott::Display& display) const {
        static uint_ranged_circular<uint8_t, 0, Hott::Display::size - 1> line;
        if (line == 0) {
            titleInto(display[0]);
            ++line;
        }
        else {
            if (line <= mChildren.size) { 
                uint8_t cindex = mChildren[line - 1];
                menuAction(cindex, [&](auto& child) {
                    child.putTextInto(display[line]);
                    return 0;
                });
                if (mSelectedItem && (mSelectedItem == (line - 1))) {
                    display[line][0] = '>';
                }
                ++line;
            }
            else {
                line = 0;
            }
        }
    }
    
    inline uint_NaN<uint8_t> processKey(Hott::key_t key) {
        uint_NaN<uint8_t> selectedChildIndex;
        if (mSelectedItem) {
            selectedChildIndex = mChildren[mSelectedItem];
        }
        
        if (selectedChildIndex) {
            menuAction(selectedChildIndex, [&](auto& item){
                if (item.isSelected()) {
                    item.processKey(key);
                }
                return 0;
            });            
        }
        switch (key) {
        case Hott::key_t::down:
            if (mSelectedItem) {
                if ((mSelectedItem + 1) < mChildren.size) {
                    ++mSelectedItem;
                }
            }
            else {
                mSelectedItem = 0;
            }
            break;
        case Hott::key_t::up:
            if (mSelectedItem) {
                --mSelectedItem;
            }
            else {
                mSelectedItem = 0;
            }
            break;
        case Hott::key_t::left:
            if (mParent) {
                return mParent;
            }
            return {};
            break;
        case Hott::key_t::right:
            break;
        case Hott::key_t::set:
            if (mSelectedItem) {
                bool hasChildren = false;
                menuAction(selectedChildIndex, [&](auto& item){
                    if (item.hasChildren()) {
                        hasChildren = true;
                    }
                    return 0;
                });
                if (hasChildren) {
                    return selectedChildIndex;
                }
                else {
                    menuAction(selectedChildIndex, [&](auto& item){
                        item.processKey(key);
                        return 0;
                    });
                }
            }
            break;
        case Hott::key_t::nokey:
            break;
        }
        return{};
    }
private:
    uint_ranged_NaN<uint8_t, 0, Size - 1> mSelectedItem;
    bool mSelected{false};
    const PgmStringView mTitle;
    const uint_NaN<index_type> mParent;
    const std::array<index_type, Size> mChildren;
};

template<typename Key, typename Provider>
class TextWithValue : public MenuItem<> {
public:
    constexpr TextWithValue(const PgmStringView& text, Provider& provider, Key key, uint8_t max) : 
        mTitle(text), mProvider(provider), mKey(key), mMax(max) {}
    
    inline void valueToText(uint8_t value, UI::span<3, char> buffer) const {
        Util::itoa_r<10>(value, buffer.mData);
    }
    
    inline void putTextInto(Hott::BufferString& buffer) const  {
        buffer[0] = ' ';
        buffer.insertAtFill(1, mTitle);
        
        auto& value = mProvider[mKey];
        if (value) {
            valueToText(*value, UI::make_span<18, 3>(buffer));
        }
        else {
            buffer[18] = '-';
            buffer[19] = '-';
            buffer[20] = '-';
        }
        
        if (mSelected) {
            buffer[18] |= 0x80;
            buffer[19] |= 0x80;
            buffer[20] |= 0x80;
        }
    }
    inline uint_NaN<uint8_t>  processKey(Hott::key_t key) {
        auto& v = mProvider[mKey];
        switch (key) {
        case Hott::key_t::up:
            if (mSelected) {
                if (v) {
                    if (*v > 0) {
                        --v;
                    }
                    else {
                        v.setNaN();
                    }
                }
                else {
                    *v = 0;
                }
            }
            break;
        case Hott::key_t::down:
            if (mSelected) {
                if (v) {
                    if (*v < mMax) {
                        ++v;
                    }
                    else {
                        v.setNaN();
                    }
                }
                else {
                    *v = 0;
                }
            }
            break;
        case Hott::key_t::left:
            if (mSelected) {
                mSelected = false;
            }
            break;
        case Hott::key_t::right:
            break;
        case Hott::key_t::set:
            if (mSelected) {
                mProvider.change();
            }
            mSelected = !mSelected;
            break;
        case Hott::key_t::nokey:
            break;
        }
        return {};
    }
private:
    const PgmStringView mTitle;
    Provider& mProvider;
    const Key mKey = Hott::key_t::nokey;
    const uint8_t mMax = 0;
};



class TSensorId final : public MenuItem<> {
public:
    constexpr TSensorId(uint8_t number) : mNumber{number} {
        assert(number < Storage::dsIds.capacity);
    }
    void putTextInto(Hott::BufferString& buffer) const {
        if (Storage::dsIds[mNumber]) {
            for(uint8_t i = 0; i < (Storage::dsIds[mNumber].size - 2); ++i) {
                uint8_t d = Storage::dsIds[mNumber][i + 1];
                Util::itoa_r<16>(d, &buffer[i * 3]);
            }
            for(uint8_t i = 0; i < (Storage::dsIds[mNumber].size - 3); ++i) {
                buffer[i * 3 + 2] = ':';
            }
        }
        else {
            buffer.insertAtFill(0, "--:--:--:--:--:--"_pgm);
        }
    }
private:
    const uint8_t mNumber = 0;
};

template<typename T, uint8_t L1, uint8_t LField>
class DualValue : public MenuItem<> {
public:
    inline static constexpr uint8_t csize = 0;
    constexpr DualValue(const PgmStringView& text, const T& v1, const T& v2) : mText(text), mData1{v1}, mData2{v2} {}
    
    void putTextInto(Hott::BufferString& buffer) const {
        buffer.clear();
        buffer.insertAt(0, mText);
        putValue(mData1, UI::make_span<L1, LField>(buffer));
        putValue(mData2, UI::make_span<L1 + LField, LField>(buffer));
    }
private:
    template<typename I, uint8_t F>
    inline void putValue(const FixedPoint<I, F>& value, UI::span<LField, char> b) const{
        uint8_t i = value.integerAbs();
        Util::itoa_r(i, b);
        auto f = value.fraction();
        auto b2 = UI::make_span<3, 5>(b);
        Util::ftoa(f, b2);
    }
    inline void putValue(const std::RPM& rpm, UI::span<LField, char> b) const {
        Util::itoa_r(rpm.value(), b);
    }
    const PgmStringView mText;
    const T& mData1;
    const T& mData2;
};

class Switch final : public MenuItem<> {
public:
    constexpr Switch(uint8_t number) : mNumber{number} {}
    void putTextInto(Hott::BufferString& buffer) const {
        buffer.insertAt(0, "Switch "_pgm);
        buffer[7] = '0' + (2 * mNumber);
        buffer[8] = ':';
        buffer[9] = '1' + (2 * mNumber);
        
        buffer.insertAt(11, "on "_pgm);
        
        if (mNumber == 1) {
            buffer[11] |= 0x80;
            buffer[12] |= 0x80;
            buffer[13] |= 0x80;
        }
        
        buffer.insertAt(15, "off "_pgm);
    }
private:
    const uint8_t mNumber = 0;
};

class PWMType : public TextWithValue<Storage::AVKey, Storage::ApplData> {
public:
    constexpr PWMType(const PgmStringView& title, Storage::ApplData& data, Storage::AVKey k) :
        TextWithValue(title, data, k, 3) {}
    void valueToText(uint8_t value, UI::span<3, char> buffer) const {
        if (value == 0) {
            buffer[0] = 'V';
            buffer[1] = ' ';
            buffer[2] = ' ';
        }
        else if (value == 1) {
            buffer[0] = 'R';
            buffer[1] = ' ';
            buffer[2] = ' ';
        }
        else if (value == 2) {
            buffer[0] = 'V';
            buffer[1] = '/';
            buffer[2] = 'R';
        }
    }
private:
};

class I2CId final : public MenuItem<> {
public:
    constexpr I2CId(uint8_t number) : mNumber{number} {
    }
    void putTextInto(Hott::BufferString& buffer) const {
        buffer.insertAtFill(0, "--:--:--:--:--:--"_pgm);
    }
private:
    const uint8_t mNumber = 0;
};


auto flat_tree = [&]{
    constexpr auto menuTree = Node(Menu{"WM SensMod HW 3 SW 21"_pgm}, 
                                   Node(Menu{"Uebersicht"_pgm}, 
                                        DualValue<FixedPoint<int, 4>, 5, 8>{"T1/2"_pgm, Storage::temps[0], Storage::temps[1]},
                                        DualValue<FixedPoint<int, 4>, 5, 8>{"T3/4"_pgm, Storage::temps[2], Storage::temps[3]},
                                        DualValue<std::RPM, 5, 8>{"R1/2"_pgm, Storage::rpms[0], Storage::rpms[1]},
                                        DualValue<FixedPoint<int, 4>, 5, 8>{"B1/2"_pgm, Storage::batts[0], Storage::batts[1]},
                                        DualValue<FixedPoint<int, 4>, 5, 8>{"M1/2"_pgm, Storage::minCells[0], Storage::minCells[1]},
                                        DualValue<FixedPoint<int, 4>, 5, 8>{"A1/2"_pgm, Storage::currents[0], Storage::currents[1]}
                                        ),
                                   Node(Menu{"Temperatur"_pgm},
                                        Node(Menu{"Sensoren"_pgm},
                                             TSensorId(0),
                                             TSensorId(1),
                                             TSensorId(2),
                                             TSensorId(3)
                                             )
                                       ),
                                   Node(Menu{"Spannung"_pgm},
                                        TextWithValue<Storage::AVKey, Storage::ApplData>{"Anzeige 1"_pgm, appData, Storage::AVKey::Spannung1, 1},
                                        TextWithValue<Storage::AVKey, Storage::ApplData>{"Anzeige 2"_pgm, appData, Storage::AVKey::Spannung2, 1}
                                        ),
                                   Node(Menu{"Drehzahl"_pgm},
                                        TextWithValue<Storage::AVKey, Storage::ApplData>{"Anzeige 1"_pgm, appData, Storage::AVKey::RpmSensor1, 1},
                                        TextWithValue<Storage::AVKey, Storage::ApplData>{"Anzeige 2"_pgm, appData, Storage::AVKey::RpmSensor2, 1}
                                        ),
                                   Node(Menu{"Strom"_pgm},
                                        TextWithValue<Storage::AVKey, Storage::ApplData>{"Offset"_pgm, appData, Storage::AVKey::StromOffset, 255}
                                        ),
                                   Node(Menu{"Aktoren"_pgm},
                                        Node(Menu{"Multiswitch"_pgm},
                                             Switch(0),
                                             Switch(1),
                                             Switch(2),
                                             Switch(3)
                                             ),
                                        Node(Menu{"PWM"_pgm},
                                             TextWithValue<Storage::AVKey, Storage::ApplData>{"Kanal"_pgm, appData, Storage::AVKey::PWM, 8},
                                             PWMType{"Modus"_pgm, appData, Storage::AVKey::PWMMOde}
                                             ),
                                        Node(Menu{"Led"_pgm},
                                             TextWithValue<Storage::AVKey, Storage::ApplData>{"Kanal 1"_pgm, appData, Storage::AVKey::Leds1Channel, 8},
                                             TextWithValue<Storage::AVKey, Storage::ApplData>{"Folge 1"_pgm, appData, Storage::AVKey::Leds1Sequence, 4},
                                             TextWithValue<Storage::AVKey, Storage::ApplData>{"Kanal 2"_pgm, appData, Storage::AVKey::Leds2Channel, 8},
                                             TextWithValue<Storage::AVKey, Storage::ApplData>{"Folge 2"_pgm, appData, Storage::AVKey::Leds2Sequence, 4}
                                             ),
                                        Node(Menu{"Geraete"_pgm},
                                             I2CId(0),
                                             I2CId(1),
                                             I2CId(2),
                                             I2CId(3)
                                             )
                                        )
                                   );
    
    constexpr auto ftree = make_tuple_of_tree(menuTree);
    return ftree;
};

template<typename T>
struct isMenu : std::false_type {};
template<uint8_t S, typename T>
struct isMenu<Menu<S, T>> : std::true_type {};

template<auto... II, Util::Callable L>
constexpr auto make_menu(std::index_sequence<II...>, const L& callable) {
    constexpr auto inode = callable();
    constexpr auto menu = inode.mData;
    typedef std::remove_const_t<std::remove_reference_t<decltype(menu)>> tm;
    static_assert(isMenu<tm>::value);
    return Menu<sizeof...(II)>(menu.mTitle, uint_NaN<uint8_t>(inode.mParent), {inode.mChildren[II]...});
}

template<Util::Callable L>
constexpr auto transform(const L& callable) {
    constexpr auto tuple = callable();
    static_assert(Util::isTuple(tuple), "use constexpr callabe returning a tuple");
    
    if constexpr(Util::size(tuple) == 0) {
        return std::tuple<>();
    }
    else {
        constexpr auto first = std::get<0>(tuple);    
        constexpr auto rest = [&]{return Util::tuple_tail(tuple);};
        typedef typename decltype(first)::type dataType;
        if constexpr(isMenu<dataType>::value) {
            auto tailoredMenu = make_menu(std::make_index_sequence<first.mChildren.size>{}, [&]{return first;});
            return std::tuple_cat(std::tuple(tailoredMenu), transform(rest));
        }
        else {
            return std::tuple_cat(std::tuple(first.mData), transform(rest));
        }
        
    }
}

auto t2 = transform(flat_tree); 

//decltype(t2)::_;

inline void menuAction(uint8_t index, const auto& callable) {
    Meta::visitAt(t2, index, callable);
}

template<typename PA>
class HottMenu final {
    HottMenu() = delete;
public:
    inline static void periodic() {
        Hott::key_t k = Hott::key_t::nokey;
        {
            Scoped<DisbaleInterrupt<>> di;
            k = mKey;
            mKey = Hott::key_t::nokey;
        }
        if (k != Hott::key_t::nokey) {
            processKey(k);
        }
        menuAction(mMenu, [&](auto& i) {
            i.textTo(PA::text());
            return 0;
        });
    }
    inline static void processKey(Hott::key_t key) {
        menuAction(mMenu, [&](auto& i) {
            if (auto m = i.processKey(key); m && (m != mMenu)) {
                mMenu = m;
                for(auto& line : PA::text()) {
                    line.clear();
                }
            }
            return 0;
        });
    }
    inline static void isrKey(std::byte b) {
        mKey = Hott::key_t{b};
    }
private:
    inline static volatile Hott::key_t mKey = Hott::key_t::nokey;
    inline static uint8_t mMenu = std::tuple_size<decltype(t2)>::value - 1;
};

using menu = HottMenu<menuData>;




struct AsciiHandler {
    static void start() {
        crWriterSensorText::enable<true>();
    }    
    static void stop() {
        // not: das disable sollte automatisch laufen
        //        crWriterSensorText::enable<false>();
    }    
    static void process(std::byte key) {
        menu::isrKey(key);        
    }
};
struct BinaryHandler {
    static void start() {
        crWriterSensorBinary::enable<true>();
    }    
    static void stop() {
        // not: das disable sollte automatisch laufen
        //        crWriterSensorBinary::enable<false>();
    }    
};
struct BCastHandler {
    static void start() {
        std::outl<terminal>("hbr start"_pgm);
        crWriterSensorBinary::enable<true>();
    }    
    static void stop() {
    }    
};

template<typename Sensor, typename Store, typename Alarm>
class TempFSM {
public:
    inline static void init() {
        mMeasureTimer = Alarm::create(750_ms, AlarmFlags::Disabled | AlarmFlags::OneShot);
        mTempTimer = Alarm::create(3000_ms, AlarmFlags::Periodic);
    }
    inline static void tick(uint8_t timer) {
        if (timer == *mMeasureTimer) {
            process(Event::WaitOver);
        }
        else if (timer == *mTempTimer) {
            process(Event::Measure);
        }
    }
    inline static void periodic() {
        Sensor::periodic([]{
            mState = State::Start;
            if (mSensorNumber == 0) {
                sensorData::temp1(Sensor::temperature());
                std::outl<terminal>("Temp1: "_pgm, Sensor::temperature());
            }
            if (mSensorNumber == 1) {
                sensorData::temp2(Sensor::temperature());
            }
            Storage::temps[mSensorNumber] = Sensor::temperature();
            mSensorNumber = (mSensorNumber + 1) % Store::dsIds.size();
        });
    }
private:
    enum class State : uint8_t {Start, Wait, WaitRead};
    enum class Event : uint8_t {Measure, WaitOver};
    
    inline static void process(Event e) {
        switch (mState) {
        case State::Start:
            if (e == Event::Measure) {
                Sensor::convert();
                alarmTimer::start(*mMeasureTimer);
                mState = State::Wait;
            }
            break;
        case State::Wait:
            if (e == Event::WaitOver) {
                if (Store::dsIds[mSensorNumber]) {
                    Sensor::startGet(Store::dsIds[mSensorNumber]);
                    mState = State::WaitRead;
                }
                else {
                    mSensorNumber = 0;
                    mState = State::Start;
                }
            }            
            break;
        case State::WaitRead:
            break;
        }
    }
    inline static State mState = State::Start;
    inline static uint8_t mSensorNumber = 0;
    inline static std::optional<uint7_t> mMeasureTimer;
    inline static std::optional<uint7_t> mTempTimer;
};

using tempFSM = TempFSM<ds18b20, Storage, alarmTimer>;

template<typename LEDs, typename Alarm, uint8_t Offset = 1>
class LedFSM {
    enum class State : uint8_t {};
public:
    inline static void init() {
        mTimer = Alarm::create(1000_ms, AlarmFlags::Periodic);
    }
    inline static void tick(uint8_t timer) {
        if (timer == *mTimer) {
            LEDs::template set<false>(Color{});
            LEDs::set(mActual, Constants::cBlue);
            ++mActual;
        }
    }
private:    
    inline static std::optional<uint7_t> mTimer;
    inline static uint_ranged_circular<uint8_t, Offset, LEDs::size - 1> mActual;
};

using ledFSM = LedFSM<leds, alarmTimer>;

class Measurements {
    inline static constexpr uint8_t hottScale = adcController::mcu_adc_type::VRef / 0.02;
    inline static constexpr auto rawMax = adcController::value_type::Upper;
    inline static constexpr uint8_t battScale = adcController::mcu_adc_type::VRef / 0.1;
public:
    static inline void update() {
        switch (part) {
        case 0:
            start();
            update0();
            break;
        case 1:
            update1();
            break;
        case 2:
            update2();
            break;
        case 3:
            update3();
            break;
        case 4:
            update4();
            break;
        default:
            assert(false);
        }
        ++part;
    }
private:
    static inline void start() {
        zmin = std::numeric_limits<uint16_t>::max();
        zminNum = uint_NaN<uint8_t>();
    }
    static inline void update0() {
        uint16_t v1 = adcController::value(1) * 4;
        uint16_t v2 = adcController::value(2) * 8;
        uint16_t v3 = adcController::value(0) * 12;
        
        uint16_t z1 = (v1 * hottScale) / rawMax;
        uint16_t z2 = ((v2 - v1) * hottScale) / rawMax;
        uint16_t z3 = ((v3 - v2) * hottScale) / rawMax;
        
        if ((z1 > 0) && (z1 < zmin)) {
            zmin = z1;
            zminNum = 0;
        }
        if ((z2 > 0) && (z2 < zmin)) {
            zmin = z2;
            zminNum = 1;
        }
        if ((z3 > 0) && (z3 < zmin)) {
            zmin = z3;
            zminNum = 2;
        }
        sensorData::cellVoltageRaw(0, z1);
        sensorData::cellVoltageRaw(1, z2);
        sensorData::cellVoltageRaw(2, z3);
        uint16_t batt = (v3 * battScale) / rawMax;
        sensorData::batteryVoltageRaw(0, batt);
        sensorData::mainVoltageRaw(batt);
        
        Storage::batts[0] = FixedPoint<int, 4>::fromRaw((batt << 4) / 10);
        if (zminNum) {
            Storage::minCells[0] = FixedPoint<int, 4>::fromRaw((zmin << 4) / 50);
            sensorData::batteryMinimumRaw(zminNum, zmin);
        }
    }
    static inline void update1() {
        uint16_t v1 = adcController::value(4) * 4;
        uint16_t v2 = adcController::value(5) * 8;
        uint16_t v3 = adcController::value(3) * 12;
        
        uint16_t z1 = (v1 * hottScale) / rawMax;
        uint16_t z2 = ((v2 - v1) * hottScale) / rawMax;
        uint16_t z3 = ((v3 - v2) * hottScale) / rawMax;
        
        if ((z1 > 0) && (z1 < zmin)) {
            zmin = z1;
            zminNum = 3;
        }
        if ((z2 > 0) && (z2 < zmin)) {
            zmin = z2;
            zminNum = 4;
        }
        if ((z3 > 0) && (z3 < zmin)) {
            zmin = z3;
            zminNum = 5;
        }
        
        sensorData::cellVoltageRaw(3, z1);
        sensorData::cellVoltageRaw(4, z2);
        sensorData::cellVoltageRaw(5, z3);
        uint16_t batt = (v3 * battScale) / rawMax;
        sensorData::batteryVoltageRaw(1, batt);
        
        Storage::batts[1] = FixedPoint<int, 4>::fromRaw((batt << 4) / 10);
        if (zminNum) {
            Storage::minCells[1] = FixedPoint<int, 4>::fromRaw((zmin << 4) / 50);
            sensorData::batteryMinimumRaw(zminNum, zmin);
        }
    }
    static inline void update2() {
        // todo: Skalierung berechnen    
        uint16_t a = adcController::value(6);
        uint16_t a1 = 0;
        if (a >= 128) {
            a1 = a - 128;
        }
        else {
            a1 = 128 - a;
        }
        
        uint16_t c1 = (a1 * 298) / 100;
        
        sensorData::currentRaw(c1);
    }
    static inline void update3() {
        const auto upm1 = rpm1::rpm();
        sensorData::rpm1(upm1);
        Storage::rpms[0] = rpm1::rpm();
        
#ifdef USE_RPM2_ON_OPTO2
        const auto upm2 = rpm2::rpm();
        Storage::rpms[1] = upm2;
#endif
    }
    static inline void update4() {
#ifdef USE_TC0_AS_SWUSART
        GPS::VTG::speedRaw(decimalBuffer);
        auto s = StringConverter<FixedPoint<int16_t, 4>>::parse(decimalBuffer);
        sensorData::speedRaw(s.raw());
#endif
        
    }
    inline static StringBuffer<GPS::Sentence::DecimalMaxWidth> decimalBuffer;
    
    inline static uint_ranged_circular<uint8_t, 0, 4> part;
    inline static uint16_t zmin = std::numeric_limits<uint16_t>::max();
    inline static uint_NaN<uint8_t> zminNum;
};

inline void updateActors() {
    static uint_ranged_circular<uint8_t, 0, 3> part;
    switch(part) {
    case 0: 
    {
        auto v1 = Hott::SumDProtocollAdapter<0>::value(1);
#ifdef USE_RPM2_ON_OPTO2
        softPpm::ppm(v1, 0);
#else
        hardPpm::ppm<hardPpm::A>(v1);
#endif
    }
        break;
    case 1:
    {
        auto v3 = Hott::SumDProtocollAdapter<0>::value(3);
#ifdef USE_RPM2_ON_OPTO2
        softPpm::ppm(v3, 1);
#else
        hardPpm::ppm<hardPpm::B>(v3);
#endif
    }
        break;
    case 2:
    {
        auto v0 = Hott::SumDProtocollAdapter<0>::value(0);
#ifndef USE_TC0_AS_SWUSART
        hbridge1::pwm(v0);
#endif
    }
        break;
    case 3:
    {
        auto v0 = Hott::SumDProtocollAdapter<0>::value(0);
#ifndef USE_TC0_AS_SWUSART
        hbridge2::pwm(v0);
#endif
    }
        break;
    default:
        assert(false);
    }
    ++part;
}

inline void updateMultiChannel() {
    static uint8_t part = 0;
    static std::byte d{0};
    if (Hott::SumDProtocollAdapter<0>::hasMultiChannel()) {
        if (part < Hott::MultiChannel::size) {
            if (Hott::SumDProtocollAdapter<0>::mChannel(part) == Hott::MultiChannel::State::Up) {
                d |= std::byte(1 << part);
            }
            ++part;
        }
        else {
            mcp23008::startWrite(0x09, d);
            d = 0_B;
            part = 0;            
        }
    }
    else {
        //        mcp23008::startWrite(0x09, std::byte{++part});
    }
}

int main() {
    constexpr std::hertz fCr = 1 / Hott::hottDelayBetweenBytes;
    static_assert(fCr == Config::Timer::frequency);
    
    testPin1::dir<AVR::Output>();
    testPin1::on();
    
    testPin2::dir<AVR::Output>();
    testPin2::on();
    
    testPin3::dir<AVR::Output>();
    testPin3::on();
    
    eeprom::init();
    adcController::init();
    
    isrRegistrar::init();
    
    alarmTimer::init(AVR::TimerMode::CTCNoInt); 
    
#ifndef USE_RPM2_ON_OPTO2
    //    ppmDecoder::init();
#endif
    
    leds::init();
    leds::off();
    
    Util::delay(1_ms);    
    leds::set(0, Constants::cGreen);
    
    ledFSM::init();
    
    testPin1::dir<AVR::Output>();
    
    crWriterSensorBinary::init();
    crWriterSensorText::init();
    
#ifdef USE_TC1_AS_HARDPPM
    hardPpm::init();
#else 
    softPpm::init();    
#endif
    rpm1::init();
#ifdef USE_RPM2_ON_OPTO2
    rpm2::init();
#endif
    
#ifndef USE_TC0_AS_SWUSART
    hardPwm::init<Constants::pwmFrequency>();
#else
    gpsUart::init();
#endif
    
    ds18b20::init();
    
    TwiMaster::init<Constants::fSCL>();
    sensorUsart::init<19200>();
    rcUsart::init<115200>();
    
    {
        Scoped<EnableInterrupt<>> ei;
        
        std::outl<terminal>("Test21"_pgm);
        
        {
            std::array<OneWire::ow_rom_t, Storage::dsIds.capacity> ids;
            oneWireMaster::findDevices(ids, ds18b20::family);
            for(const auto& id : ids) {
                if (id) {
                    Storage::dsIds.push_back(id);
                    std::outl<terminal>(id);
                    Util::delay(10_ms);
                }
            }
        }
        {
            std::array<TWI::Address, Storage::i2cDevices.capacity> i2cAddresses;
            TwiMaster::findDevices(i2cAddresses);
            for(const auto& d : i2cAddresses) {
                if (d) {
                    Storage::i2cDevices.push_back(d);
                    std::outl<terminal>(d);
                    Util::delay(10_ms);
                }
            }
        }
        
        mcp23008::startWrite(0x00, std::byte{0x00}); // output
        mcp23008::startWrite(0x09, std::byte{0x00}); // output
        
        if (!oled::init()) {
            std::outl<terminal>("oled error"_pgm);
        }
        
        oled::clear();
        
        tempFSM::init();
        
        const auto periodicTimer = alarmTimer::create(500_ms, AlarmFlags::Periodic);
        
        while(true){
            testPin3::toggle(); // max 127 uS ein Intervall
            TwiMasterAsync::periodic();
            menu::periodic();
            rpm1::periodic();
#ifdef USE_RPM2_ON_OPTO2
            rpm2::periodic();
#endif
            adcController::periodic();
            systemClock::periodic<systemClock::flags_type::ocfa>([&](){
                crWriterSensorBinary::rateProcess();
                crWriterSensorText::rateProcess();
                oneWireMasterAsync::rateProcess();
                Measurements::update();
                updateActors();
                updateMultiChannel();
                
                alarmTimer::periodic([&](uint7_t timer){
                    if (timer == *periodicTimer) {
                        oled::put('.');
                        if (Hott::SumDProtocollAdapter<0>::hasMultiChannel()) {
                            std::out<terminal>("Multi["_pgm, Hott::SumDProtocollAdapter<0>::mChannelForMultiChannel, "]: "_pgm);
                            for(uint8_t i = 0; i < Hott::MultiChannel::size; ++i) {
                                std::out<terminal>(Char{' '}, (uint8_t)Hott::SumDProtocollAdapter<0>::mChannel(i));
                            }
                            std::outl<terminal>();
                        }
                        rpm1::check();
                        uint16_t a = adcController::value(6);
                        std::outl<terminal>("acs: "_pgm, a);
#ifdef USE_RPM2_ON_OPTO2
                        rpm2::check();
#endif
#ifdef USE_PPM_ON_OPTO2
                        std::outl<terminal>("ppm: "_pgm, ppmDecoder::value(0));
#endif
#ifdef USE_TC0_AS_SWUSART
#endif
#ifdef MEM
                        std::outl<terminal>("mem: "_pgm, Util::Memory::getUnusedMemory());
#endif
                    }
                    else {
                        tempFSM::tick(timer);
                        ledFSM::tick(timer);
                    }
                });
                appData.expire();
            });
            tempFSM::periodic();
            
            while(eeprom::saveIfNeeded()) {
                std::outl<terminal>("."_pgm);
            }
        }
    }    
}
ISR(INT1_vect) {
    isrRegistrar::isr<AVR::ISR::Int<1>>();
}
// SumD
ISR(USART1_RX_vect) {
    isrRegistrar::isr<AVR::ISR::Usart<1>::RX>();
}
ISR(USART1_UDRE_vect){
    isrRegistrar::isr<AVR::ISR::Usart<1>::UDREmpty>();
}
// Sensor
ISR(USART0_RX_vect) {
    isrRegistrar::isr<AVR::ISR::Usart<0>::RX>();
}
ISR(USART0_UDRE_vect){
    isrRegistrar::isr<AVR::ISR::Usart<0>::UDREmpty>();
}
#ifdef USE_TC0_AS_SWUSART
ISR(TIMER0_COMPA_vect) {
    testPin3::on();
    isrRegistrar::isr<AVR::ISR::Timer<0>::CompareA>();
    testPin3::off();
}
ISR(TIMER0_COMPB_vect) {
    testPin2::on();
    isrRegistrar::isr<AVR::ISR::Timer<0>::CompareB>();
    testPin2::off();
}
ISR(INT0_vect) {
    testPin1::on();
    isrRegistrar::isr<AVR::ISR::Int<0>>();
    testPin1::off();
}
#endif
// Timer 4
// softPpm
#ifdef USE_RPM2_ON_OPTO2
ISR(TIMER4_COMPA_vect) {
    isrRegistrar::isr<AVR::ISR::Timer<4>::CompareA>();
}
ISR(TIMER4_COMPB_vect) {
    isrRegistrar::isr<AVR::ISR::Timer<4>::CompareB>();
}
#endif

#ifdef USE_PPM_ON_OPTO2
# ifdef USE_ICP1
ISR(TIMER1_CAPT_vect) {
    isrRegistrar::isr<AVR::ISR::Timer<1>::Capture>();
}
# else
ISR(PCINT0_vect) {
    isrRegistrar::isr<AVR::ISR::PcInt<0>>();
}
# endif
#endif
#ifndef NDEBUG
void assertFunction(const PgmStringView& expr, const PgmStringView& file, unsigned int line) noexcept {
    std::outl<terminal>("Assertion failed: "_pgm, expr, Char{','}, file, Char{','}, line);
    while(true) {}
}
#endif

