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

#define MEM
//#define NDEBUG

#include "local02_debug.h"
#include "rcsensorled01.h"
#include "console.h"
#include "util/meta.h"
#include "appl/blink.h"
#include "hal/eeprom.h"
#include "external/hott/menu.h"

namespace Storage {
    enum class AVKey : uint8_t {TSensor1 = 0, TSensor2, RpmSensor1, RpmSensor2, Spannung1, Spannung2, Strom, PWM, Leds1, Leds2, _Number};
    
    class ApplData : public EEProm::DataBase<ApplData> {
    public:
        uint_NaN<uint8_t>& operator[](AVKey key) {
            return AValues[static_cast<uint8_t>(key)];
        }
    private:
        std::array<uint_NaN<uint8_t>, static_cast<uint8_t>(AVKey::_Number)> AValues;
    };
    std::array<OneWire::ow_rom_t, 4> dsIds;
}

using eeprom = EEProm::Controller<Storage::ApplData>;
auto& appData = eeprom::data();

namespace {
    constexpr bool useTerminal = true;
}

namespace Constants {
    static constexpr std::hertz pwmFrequency = 100_Hz * 256;
    static constexpr Color cOff{0};
    static constexpr Color cRed{Red{32}};
    static constexpr Color cBlue{Blue{32}};
    static constexpr Color cGreen{Green{32}};
    static constexpr auto title = "Test 24"_pgm;
    static constexpr const std::hertz fSCL = 100000_Hz;
}

using terminalDevice = std::conditional<useTerminal, rcUsart, void>::type;
using terminal = std::basic_ostream<terminalDevice>;

using statusLed = Blinker<led, Constants::cGreen>;

template<typename... T>
struct Distributor {
    using Items = Meta::filter<Meta::nonVoid, Meta::List<T...>>;
    template<typename U> struct NonVoidDistributor;
    template<template<typename...> typename L, typename... U>
    struct NonVoidDistributor<L<U...>> {
        inline static void init() {
            (U::init(), ...);
        }
    };
    inline static void init() {
        NonVoidDistributor<Items>::init();
    }
};

using sensorData = Hott::SensorProtocollBuffer<0>;
using menuData = Hott::SensorTextProtocollBuffer<0>;
using crWriterSensorBinary = ConstanteRateWriter<sensorData, sensorUsart>;
using crWriterSensorText = ConstanteRateWriter<menuData, sensorUsart>;

using isrRegistrar = IsrRegistrar<sensorUsart::RxHandler, sensorUsart::TxHandler, rcUsart::RxHandler, rcUsart::TxHandler>;

class TSensorId : public UI::MenuItem<Hott::BufferString, Hott::key_t> {
public:
    TSensorId(uint8_t number) : mNumber{number} {
        assert(number < Storage::dsIds.size);
    }
    virtual void putTextInto(Hott::BufferString& buffer) const override {
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
    uint8_t mNumber = 0;
};

class TSensorMenu : public Hott::NumberedMenu {
public:
    TSensorMenu(Menu* parent, uint8_t n) : NumberedMenu(parent, n, "Sensor"_pgm, &mID), mID{n} {}
private:
    TSensorId mID{0};
};

class TemperaturSensorenMenu : public Hott::Menu {
public:
    TemperaturSensorenMenu(Menu* parent) : Menu(parent, "Temp. Sensoren"_pgm, &mTS1, &mTS2, &mTS3, &mTS4) {}
private:
    TSensorMenu mTS1{this, 0};
    TSensorMenu mTS2{this, 1};
    TSensorMenu mTS3{this, 2};
    TSensorMenu mTS4{this, 3};
};

class TemperaturMenu : public Hott::Menu {
public:
    TemperaturMenu(Hott::Menu* parent) : Hott::Menu(parent, "Temperatur"_pgm, &mSensor1, &mSensor2, &mSensoren) {}

private:
    Hott::TextWithValue<Storage::AVKey, Storage::dsIds.size - 1, Storage::ApplData> mSensor1{"Anzeige 1"_pgm, appData, Storage::AVKey::TSensor1};
    Hott::TextWithValue<Storage::AVKey, Storage::dsIds.size - 1, Storage::ApplData> mSensor2{"Anzeige 2"_pgm, appData, Storage::AVKey::TSensor2};
    TemperaturSensorenMenu mSensoren{this};
};

class SpannungMenu : public Hott::Menu {
public:
    SpannungMenu(Hott::Menu* parent) : Menu(parent, "Spannung"_pgm, &mSensor1) {}
private:
    Hott::TextWithValue<Storage::AVKey, 1, Storage::ApplData> mSensor1{"Sensor 1"_pgm, appData, Storage::AVKey::Spannung1};
};

class RCMenu : public Hott::Menu {
public:
    RCMenu() : Menu(this, "WM SensorLed"_pgm, &mTemperatur, &mSpannung) {}
private:
    TemperaturMenu mTemperatur{this};                        
    SpannungMenu   mSpannung{this};                        
};

template<typename PA, typename MenuType>
class HottMenu final {
    HottMenu() = delete;
public:
    static void periodic() {
        mMenu->textTo(PA::text());
    }
    static void processKey(Hott::key_t key) {
        assert(mMenu);
        if (auto m = mMenu->processKey(key); m != mMenu) {
            mMenu = m;
            for(auto& line : PA::text()) {
                line.clear();
            }
        }
    }
private:
    inline static MenuType mTop;
    inline static Hott::Menu* mMenu = &mTop;
};

using menu = HottMenu<menuData, RCMenu>;

struct HottBinaryHandler : public EventHandler<EventType::HottBinaryRequest> {
    inline static bool process(std::byte) {
        crWriterSensorBinary::enable<true>();
        crWriterSensorText::enable<false>();
        return true;
    }
};

struct HottKeyHandler : public EventHandler<EventType::HottAsciiKey> {
    inline static bool process(std::byte v) {
        std::outl<terminal>("key: "_pgm, v);
        menu::processKey(Hott::key_t(v));
        return true;
    }
};

struct HottBroadcastHandler : public EventHandler<EventType::HottSensorBroadcast> {
    inline static bool process(std::byte) {
        std::outl<terminal>("hbr"_pgm);
        crWriterSensorBinary::enable<true>();
        crWriterSensorText::enable<false>();
        return true;
    }
};

struct HottTextHandler : public EventHandler<EventType::HottAsciiRequest> {
    inline static bool process(std::byte) {
        crWriterSensorBinary::enable<false>();
        crWriterSensorText::enable<true>();
        return true;
    }
};
struct Usart0Handler : public EventHandler<EventType::UsartRecv0> {
    inline static bool process(std::byte) {
        statusLed::blink(Blue{32}, 2);
        return true;
    }
};
struct Usart1Handler : public EventHandler<EventType::UsartRecv1> {
    static bool process(std::byte) {
        statusLed::blink(Blue{32}, 3);
        return true;
    }
};
struct UsartFeHandler : public EventHandler<EventType::UsartFe> {
    inline static bool process(std::byte) {
        return true;
    }
};
struct UsartUpeHandler : public EventHandler<EventType::UsartUpe> {
    inline static bool process(std::byte) {
        return true;
    }
};
struct UsartDorHandler : public EventHandler<EventType::UsartDor> {
    inline static bool process(std::byte) {
        return true;
    }
};

struct TWIHandlerError: public EventHandler<EventType::TWIError> {
    static bool process(std::byte) {
        statusLed::blink(Color{Red{32}, Green{0}, Blue{32}}, 5);
        return true;
    }  
};

const auto tempTimer = alarmTimer::create(1000_ms, AlarmFlags::Periodic);
const auto measureTimer = alarmTimer::create(750_ms, AlarmFlags::Disabled | AlarmFlags::OneShot);

struct TempFSM {
    enum class State : uint8_t {Start, Wait, WaitRead};
    enum class Event : uint8_t {Measure, WaitOver, Read};
    static void process(Event e) {
        switch (mState) {
        case State::Start:
            if (e == Event::Measure) {
                ds18b20::convert();
                alarmTimer::start(*measureTimer);
                mState = State::Wait;
            }
            break;
        case State::Wait:
            if (e == Event::WaitOver) {
                if (Storage::dsIds[mSensorNumber]) {
                    ds18b20::startGet(Storage::dsIds[mSensorNumber]);
                    mState = State::WaitRead;
                }
                else {
                    mSensorNumber = 0;
                    mState = State::Start;
                }
            }            
            break;
        case State::WaitRead:
            if (e == Event::Read) {
                mState = State::Start;
            }
        }
    }
    inline static State mState = State::Start;
    inline static uint8_t mSensorNumber = 0;
};

using tempFSM = TempFSM;


struct DS18B20MeasurementHandler: public EventHandler<EventType::DS18B20Measurement> {
    static bool process(std::byte) {
        auto t = ds18b20::temperature();
        sensorData::temp2(t);
        tempFSM::process(tempFSM::Event::Read);
        return true;
    }
};
struct DS18B20ErrorHandler: public EventHandler<EventType::DS18B20Error> {
    static bool process(std::byte) {
        tempFSM::process(tempFSM::Event::Read);
        return true;
    }
};


const auto periodicTimer = alarmTimer::create(500_ms, AlarmFlags::Periodic);

struct TimerHandler : public EventHandler<EventType::Timer> {
    inline static bool process(std::byte b) {
        auto timer = std::to_integer<uint7_t>(b);
        if (timer == *periodicTimer) {
            appData.expire();
            
            ++mCounter;
            statusLed::tick();

            std::outl<terminal>("pr: "_pgm, Hott::SumDProtocollAdapter<0>::value8Bit(0).toInt());
            std::percent pv1 = std::scale(Hott::SumDProtocollAdapter<0>::value8Bit(0));
            std::outl<terminal>("pv: "_pgm, pv1);
            hbridge::pwm(pv1);
            
            
            auto v1 = Hott::SumDProtocollAdapter<0>::value(2);
            hardPpm::ppm<hardPpm::A>(v1);
            auto v2 = Hott::SumDProtocollAdapter<0>::value(3);
            hardPpm::ppm<hardPpm::B>(v2);

            rpm::check();
            
            if (Hott::SumDProtocollAdapter<0>::hasMultiChannel()) {
                std::byte d{0};
                std::out<terminal>("Multi:"_pgm);
                for(uint8_t i = 0; i < Hott::MultiChannel::size; ++i) {
                    if (Hott::SumDProtocollAdapter<0>::mChannel(i) == Hott::MultiChannel::State::Up) {
                        d |= std::byte(1 << i);
                    }
                    std::out<terminal>(Char{' '}, (uint8_t)Hott::SumDProtocollAdapter<0>::mChannel(i));
                }
                std::outl<terminal>(Char{'-'});

                mcp23008::startWrite(0x09, d);
            }
            
            return true;
        }
        else if (timer == *measureTimer) {
            tempFSM::process(tempFSM::Event::WaitOver);
            return true;
        }
        else if (timer == *tempTimer) {
            tempFSM::process(tempFSM::Event::Measure);
            return true;
        }
        return false;
    }
    inline static uint8_t mCounter = 0;
};

using distributor = Distributor<isrRegistrar, statusLed, crWriterSensorBinary, crWriterSensorText, adcController, ds18b20, 
rpm, hardPpm, eeprom>;

void updateMeasurements() {
    constexpr uint8_t hottScale = adcController::mcu_adc_type::VRef / 0.02;
    
    constexpr auto rawMax = adcController::value_type::Upper;
    
    uint16_t v1 = adcController::value(1) * 4;
    uint16_t v2 = adcController::value(2) * 8;
    uint16_t v3 = adcController::value(0) * 12;
    
    uint16_t z1 = (v1 * hottScale) / rawMax;
    uint16_t z2 = ((v2 - v1) * hottScale) / rawMax;
    uint16_t z3 = ((v3 - v2) * hottScale) / rawMax;
    
    sensorData::cellVoltageRaw(0, z1);
    sensorData::cellVoltageRaw(1, z2);
    sensorData::cellVoltageRaw(2, z3);

    constexpr uint8_t battScale = adcController::mcu_adc_type::VRef / 0.1;
    
    uint16_t b1 = (v3 * battScale) / rawMax;
    sensorData::batteryVoltageRaw(0, b1);
    
    constexpr uint8_t tempScale = adcController::mcu_adc_type::VRef / 0.314;
    
    uint8_t t1 = (adcController::value(4) * tempScale * 25) / rawMax + 20;
    sensorData::temperatureRaw(0, t1);
    
    constexpr uint16_t currentScale = 10 * 4.7 * adcController::mcu_adc_type::VRef / 0.066; // (ACS 712 = 66mV/A, Hott: 0,1A Steps)
    constexpr uint16_t currentOffset = currentScale * 105; // experimentelle O-A-Wert --> EEProm
    
    uint16_t c1 = (adcController::value(3) * currentScale - currentOffset) / rawMax;
    
    sensorData::currentRaw(c1);
    
    const auto upm = rpm::rpm();
    sensorData::rpm1(upm);
}

int main() {
    using namespace std::literals::quantity;
    distributor::init();
    alarmTimer::init(AVR::TimerMode::CTCNoInt); 

    leds1Pin::dir<AVR::Output>();
    leds2Pin::dir<AVR::Output>();
    
    hbridge::init<Constants::pwmFrequency>();
    hbridge::pwm(0_ppc);
    hbridge::direction() = hbridge::Direction{false};
    
    hardPpm::ppm<hardPpm::A>(50_ppc);
    hardPpm::ppm<hardPpm::A>(50_ppc);
    
    constexpr std::hertz fCr = 1 / Hott::hottDelayBetweenBytes;
    static_assert(fCr == Config::Timer::frequency);
    
    sensorUsart::init<19200>();
    rcUsart::init<115200>();

    TwiMaster::init<Constants::fSCL>();

    mcp23008::startWrite(0x00, std::byte{0x00}); // output

    using allEventHandler = EventHandlerGroup<
    TimerHandler, UsartFeHandler, UsartUpeHandler, UsartDorHandler, Usart0Handler, Usart1Handler,
    HottBinaryHandler, HottBroadcastHandler, HottTextHandler, HottKeyHandler,
    ds18b20, DS18B20ErrorHandler, DS18B20MeasurementHandler,
    TWIHandlerError>;

    {
        Scoped<EnableInterrupt<>> ei;
        
        std::outl<terminal>(Constants::title);

        oneWireMaster::findDevices(Storage::dsIds, ds18b20::family);
        for(const auto& id : Storage::dsIds) {
            if (id) {
                std::outl<terminal>(id);
            }
        }

        EventManager::run2<allEventHandler>([](){
            leds1Pin::on();   
            menu::periodic();
            leds1Pin::off();   
            
            TwiMasterAsync::periodic();
            
            adcController::periodic();
            systemClock::periodic<systemClock::flags_type::ocfa>([](){
                leds2Pin::toggle();
                alarmTimer::rateProcess();
                crWriterSensorBinary::rateProcess();
                crWriterSensorText::rateProcess();
                oneWireMasterAsync::rateProcess();
            });
            
            updateMeasurements();
            
            rpm::periodic();
            
            if (EventManager::unprocessedEvent()) {
                std::outl<terminal>("+++ Unprocessd"_pgm);
                statusLed::blink(Constants::cRed, 10);
            }
            if (EventManager::leakedEvent()) {
                std::outl<terminal>("+++ Leaked"_pgm);
                statusLed::blink(Constants::cBlue, 10);
            }
            
            while(eeprom::saveIfNeeded()) {
                std::outl<terminal>("."_pgm);
            }
            
        });
    }    
}

#ifndef NDEBUG
void assertFunction(const PgmStringView& expr, const PgmStringView& file, unsigned int line) noexcept {
    std::outl<terminal>("Assertion failed: "_pgm, expr, Char{','}, file, Char{','}, line);
//    while(true) {}
}
#endif

// Sensor
ISR(USART0_RX_vect) {
    isrRegistrar::isr<AVR::ISR::Usart<0>::RX>();
}
ISR(USART0_UDRE_vect){
    isrRegistrar::isr<AVR::ISR::Usart<0>::UDREmpty>();
}
// SumD
ISR(USART1_RX_vect) {
    isrRegistrar::isr<AVR::ISR::Usart<1>::RX>();
}
ISR(USART1_UDRE_vect){
    isrRegistrar::isr<AVR::ISR::Usart<1>::UDREmpty>();
}
