/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2019 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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

#define NDEBUG

#include <mcu/avr.h>
#include <mcu/internals/systemclock.h>

#include <mcu/internals/cppm.h>
#include <mcu/internals/constantrate.h>
#include <mcu/internals/pwm.h>
#include <mcu/internals/eeprom.h>

#include <etl/output.h>
#include <etl/fixedpoint.h>
#include <etl/scoped.h>
#include <etl/meta.h>

#include <external/hal/alarmtimer.h>
#include <external/hott/hott.h>
#include <external/hott/menu.h>

#include <external/bluetooth/roboremo.h>
#include <external/bluetooth/qtrobo.h>
#include <external/solutions/ifx007.h>
#include <external/solutions/rpm.h>

#include <external/hal/adccontroller.h>

using PortB = AVR::Port<AVR::B>;
using PortC = AVR::Port<AVR::C>;
using PortD = AVR::Port<AVR::D>;
using PortE = AVR::Port<AVR::E>;

using pc1 = AVR::Pin<PortC, 3>;
using pc0 = AVR::Pin<PortC, 4>;

using txd0Enable = AVR::Pin<PortD, 3>;

using inh1 = AVR::Pin<PortD, 5>;
using inh2 = AVR::Pin<PortB, 0>;

using ppm = AVR::Pin<PortE, 0>;

using qtPA = External::QtRobo::ProtocollAdapter<0, 16>;

using sumd = Hott::SumDProtocollAdapter<0, AVR::UseInterrupts<false>>;
struct AsciiHandler;
struct BinaryHandler;
struct BCastHandler;

using sensorPA = Hott::SensorProtocollAdapter<0, Hott::esc_id, AsciiHandler, BinaryHandler, BCastHandler>;

using sensorUsart = AVR::Usart<0, sensorPA, AVR::UseInterrupts<false>, AVR::ReceiveQueueLength<0>> ;

using rcUsart = AVR::Usart<1, sumd, AVR::UseInterrupts<false>, AVR::ReceiveQueueLength<0>>;
//using rcUsart = AVR::Usart<1, External::Hal::NullProtocollAdapter, AVR::UseInterrupts<false>, AVR::ReceiveQueueLength<0>>;
//using rcUsart = AVR::Usart<1, sumd, AVR::UseInterrupts<true>, AVR::ReceiveQueueLength<0>>;

using terminalDevice = rcUsart;
using terminal = etl::basic_ostream<terminalDevice>;
using log = External::QtRobo::Logging<terminal, 0>;

//using sensorData = Hott::SensorProtocollBuffer<0>;
using escData = Hott::EscProtocollBuffer<0>;
using crWriterSensorBinary = ConstanteRateWriter<escData, sensorUsart>;

using menuData = Hott::SensorTextProtocollBuffer<0>;
using crWriterSensorText = ConstanteRateWriter<menuData, sensorUsart>;

using pwm = AVR::Pwm<1>;
using hbridge = External::IFX007::HBridge<pwm, sumd::value_type>;

using adc = AVR::Adc<0>;
using adcController = External::Hal::AdcController<adc>;

namespace  {
    using namespace std::literals::chrono;
    using namespace External::Units;
//    constexpr auto interval = 10_ms;
    constexpr auto interval = External::Units::duration_cast<std::chrono::milliseconds>(Hott::hottDelayBetweenBytes);
//    constexpr auto fpwm = hertz{1000};
    constexpr auto fpwm = Project::Config::fMcu;
    
    constexpr RPM MaximumRpm{12000};
    constexpr RPM MinimumRpm{100};
}

using rpm= External::RpmWithIcp<3, MinimumRpm, MaximumRpm>;

using systemClock = AVR::SystemTimer<0, interval>;
using alarmTimer = External::Hal::AlarmTimer<systemClock>;

struct Storage {
    enum class AVKey : uint8_t {Input = 0, Channel, PWMF1, PWMF2, PWMMode, PWMThr1, PWMThr2, SplitMode, _Number};
    
    class ApplData : public EEProm::DataBase<ApplData> {
    public:
        etl::uint_NaN<uint8_t>& operator[](AVKey key) {
            return AValues[static_cast<uint8_t>(key)];
        }
    private:
        std::array<etl::uint_NaN<uint8_t>, static_cast<uint8_t>(AVKey::_Number)> AValues{};
    };
};

using eeprom = EEProm::Controller<Storage::ApplData>;
auto& appData = eeprom::data();

class Dashboard final : public Hott::Menu {
public:
    Dashboard() : Menu(this, "Dashboard"_pgm
                    ) {}
private:
//    Hott::TextWithValue<Storage::AVKey, 3, Storage::ApplData> mMode{"Mode"_pgm, appData, Storage::AVKey::SplitMode};
};

// todo: Zusammenfassen: DropDown(...)

struct YesNo : public Hott::TextWithValue<Storage::AVKey, Storage::ApplData> {
    YesNo(const PgmStringView& title, Storage::ApplData& data, Storage::AVKey k) :
        TextWithValue(title, data, k, 1) {}

    virtual void valueToText(uint8_t value, etl::span<3, etl::Char> buffer) const override{
        if (value == 0) {
            buffer.insertLeftFill("No"_pgm);
        }
        else {
            buffer.insertLeftFill("Yes"_pgm);
        }
    }
};

struct Connection : public Hott::TextWithValue<Storage::AVKey, Storage::ApplData, 4> {
    Connection(const PgmStringView& title, Storage::ApplData& data, Storage::AVKey k) :
        TextWithValue(title, data, k, 2) {}

    virtual void valueToText(uint8_t value, etl::span<4, etl::Char> buffer) const override{
        if (value == 0) {
            buffer.insertLeftFill("SUMD"_pgm);
        }
        else if (value == 1) {
            buffer.insertLeftFill("CPPM"_pgm);
        }
        else if (value == 2) {
            buffer.insertLeftFill("PPM"_pgm);
        }
        else {
            buffer.insertLeftFill("err"_pgm);
        }
    }
};

struct PWMMode : public Hott::TextWithValue<Storage::AVKey, Storage::ApplData, 5> {
    PWMMode(const PgmStringView& title, Storage::ApplData& data, Storage::AVKey k) :
        TextWithValue(title, data, k, 1) {}

    virtual void valueToText(uint8_t value, etl::span<5, etl::Char> buffer) const override{
        if (value == 0) {
            buffer.insertLeftFill("Dual"_pgm);
        }
        else if (value == 1) {
            buffer.insertLeftFill("Split"_pgm);
        }
        else {
            buffer.insertLeftFill("err"_pgm);
        }
    }
};

class SplitModeMenu final : public Hott::Menu {
public:
    SplitModeMenu() : Menu(this, "SplitMode"_pgm,
                    &mMode
                    ) {}
private:
    Hott::TextWithValue<Storage::AVKey, Storage::ApplData> mMode{"Mode"_pgm, appData, Storage::AVKey::SplitMode, 2};
};

class PWMMenu final : public Hott::Menu {
public:
    PWMMenu() : Menu(this, "PWM"_pgm,
                    &mPWMF1, &mPWMF2, &mPWMThr1, &mPWMThr2, &mPWMMode
                    ) {}
private:
    Hott::TextWithValue<Storage::AVKey, Storage::ApplData> mPWMF1{"PWM Freq 1"_pgm, appData, Storage::AVKey::PWMF1, 20};
    Hott::TextWithValue<Storage::AVKey, Storage::ApplData> mPWMF2{"PWM Freq 2"_pgm, appData, Storage::AVKey::PWMF2, 20};
    Hott::TextWithValue<Storage::AVKey, Storage::ApplData> mPWMThr1{"PWM Thr 1"_pgm, appData, Storage::AVKey::PWMThr1, 100};
    Hott::TextWithValue<Storage::AVKey, Storage::ApplData> mPWMThr2{"PWM Thr 2"_pgm, appData, Storage::AVKey::PWMThr2, 100};
    PWMMode mPWMMode{"PWM Mode"_pgm, appData, Storage::AVKey::PWMMode};
};


class RCMenu final : public Hott::Menu {
public:
    RCMenu() : Menu(this, "WM BDC ESC 0.2"_pgm,
                    &mSource, &mChannel, &mPwm, &mSplitMode, &mDashboard
                    ) {}
private:
    Connection mSource{"Input Conn."_pgm, appData, Storage::AVKey::Input};
    Hott::TextWithValue<Storage::AVKey, Storage::ApplData> mChannel{"Channel"_pgm, appData, Storage::AVKey::Channel, 8};
    PWMMenu mPwm;
    SplitModeMenu mSplitMode;
    Dashboard mDashboard;
};

RCMenu topMenu; // note: global object instead of static in Hottmenu -> no guards are created (is this a g++ error)

template<typename PA>
class HottMenu final {
    HottMenu() = delete;
public:
    inline static void init() {
        clear();
    }
    inline static void periodic() {
        Hott::key_t k = Hott::key_t::nokey;
        {
            etl::Scoped<etl::DisbaleInterrupt<>> di;
            k = mKey;
            mKey = Hott::key_t::nokey;
        }
        if (k != Hott::key_t::nokey) {
            processKey(k);
        }
        mMenu->textTo(PA::text());
    }
    inline static void processKey(Hott::key_t key) {
        assert(mMenu);
        if (auto m = mMenu->processKey(key); m != mMenu) {
            mMenu = m;
            clear();
        }
    }
    inline static void isrKey(std::byte b) {
        mKey = Hott::key_t{b};
    }
private:
    inline static void clear() {
        for(auto& line : PA::text()) {
            line.clear();
        }
    }
    inline static /*volatile*/ Hott::key_t mKey = Hott::key_t::nokey;
    inline static Hott::Menu* mMenu = &topMenu;
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
#ifdef OUTPUT
        std::outl<terminal>("hbr start"_pgm);
#endif
        crWriterSensorBinary::enable<true>();
    }    
    static void stop() {
    }    
};

//using isrRegistrar = AVR::IsrRegistrar<rcUsart::RxHandler, rcUsart::TxHandler>;

template<typename... CC>
struct StaticInitializer {
    StaticInitializer() {
        (CC::init(), ...);
    }
};

using Initializer = StaticInitializer<systemClock, eeprom, hbridge, menu>;

namespace {
    Initializer initializer;
}

int main() {
    using namespace etl;
    using namespace std;
    using namespace AVR;
    
    pc0::dir<Output>();
    pc1::dir<Output>();

    txd0Enable::dir<Output>();
    txd0Enable::off();

    inh1::dir<Output>();
    inh1::on();

    inh2::dir<Output>();
    inh2::on();

    ppm::dir<Input>();
    ppm::pullup();
    
//    systemClock::init();
    
//    eeprom::init();
    
    sensorUsart::init<19200>();
    rcUsart::init<115200>();
    
    pwm::init<fpwm>();
    
//    hbridge::init();

//    menu::init();    

    const auto t = alarmTimer::create(1000_ms, External::Hal::AlarmFlags::Periodic);

    rpm::init();

    
    {
        Scoped<EnableInterrupt<>> ei;

        hbridge::duty(Hott::SumDMsg::Mid);
        
        escData::rpmRaw(121);
        escData::rpmMaxRaw(632);
        escData::voltageRaw(134);
        escData::voltageMinRaw(114);
        escData::currentRaw(3);
        escData::currentMaxRaw(27);
        escData::tempRaw(600);
        escData::tempMaxRaw(400);
        
        while(true) {
            pc1::toggle();
            sensorUsart::periodic();
            rcUsart::periodic();
            hbridge::periodic();
            menu::periodic();
            rpm::period();
            
            systemClock::periodic([&](){
                crWriterSensorBinary::rateProcess();
                crWriterSensorText::rateProcess();
                pc0::toggle();
                auto v = sumd::value(0);
                hbridge::duty(v);
                alarmTimer::periodic([&](alarmTimer::index_type timer){
                    if (timer == t) {
                        rpm::check();
//                        etl::outl<terminal>("p: "_pgm, hbridge::x1);
//                        etl::outl<terminal>("t: "_pgm, hbridge::x2);
                        
                        etl::outl<terminal>("v: "_pgm, v.toInt());
                        appData.expire();
                    }
                });
            });
            if(eeprom::saveIfNeeded()) {
                etl::out<terminal>("."_pgm);
            }    
        }
    }
}


//// SumD
//ISR(USART1_RX_vect) {
//    isrRegistrar::isr<AVR::ISR::Usart<1>::RX>();
//}
//ISR(USART1_UDRE_vect){
//    isrRegistrar::isr<AVR::ISR::Usart<1>::UDREmpty>();
//}
