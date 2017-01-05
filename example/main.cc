#include "main.h"
#include "std/limits.h"
#include "mcu/avr8.h"
#include "mcu/avr/mcutimer.h"
#include "mcu/avr/usart.h"
#include "hal/event.h"
#include "mcu/ports.h"
#include "units/physical.h"
#include "hal/alarmtimer.h"
#include "std/literals.h"
#include "mcu/avr/isr.h"
#include "mcu/avr/spi.h"
#include "mcu/avr/swusart.h"
#include "external/ws2812.h"
#include "mcu/avr/pinchange.h"
#include "mcu/avr/ppm.h"
#include "mcu/avr/adc.h"
#include "hal/ppmswitch.h"
#include "util/fsm.h"
#include "util/delay.h"
#include "external/hott/hott.h"
#include "console.h"
#include "hal/softspimaster.h"
#include "hal/button.h"
#include "external/dcf77.h"
#include "hal/softppm.h"
#include "mcu/avr/mcupwm.h"
#include "hal/softpwm.h"
#include "hal/constantrate.h"
#include "hal/variablerate.h"
#include "hal/bufferedstream.h"
#include "hal/adccontroller.h"
#include "external/onewire.h"
#include "external/ds18b20.h"
#include "external/ds1307.h"
#include "external/lm35.h"
#include "external/i2cram.h"
#include "external/rpm.h"
#include "mcu/avr/twimaster.h"
#include "std/array.h"
#include "util/memory.h"

// 20MHz full-swing
// sudo avrdude -p atmega1284P -P usb -c avrisp2 -U lfuse:w:0xf7:m -U hfuse:w:0xd1:m -U efuse:w:0xfc:m

using PortA = AVR::Port<DefaultMcuType::PortRegister, AVR::A>;
using PortB = AVR::Port<DefaultMcuType::PortRegister, AVR::B>;
using PortC = AVR::Port<DefaultMcuType::PortRegister, AVR::C>;
using PortD = AVR::Port<DefaultMcuType::PortRegister, AVR::D>;

using SoftSPIData = AVR::Pin<PortA, 0>;
using SoftSPIClock = AVR::Pin<PortA, 1>;
using SoftSPISS = AVR::Pin<PortA, 2>;
using SSpi0 = SoftSpiMaster<SoftSPIData, SoftSPIClock, SoftSPISS>;

using oneWirePin = AVR::Pin<PortA, 5>;
using oneWireMaster = OneWire::Master<oneWirePin, OneWire::Normal>;
using oneWireMasterAsync = OneWire::MasterAsync<oneWireMaster, Hott::hottDelayBetweenBytes>;
using ds18b20 = DS18B20<oneWireMasterAsync>;

std::array<OneWire::ow_rom_t, 5> dsIds;

//using terminal = SSpi0;
using bufferedTerminal = BufferedStream<SSpi0, 512>;

namespace std {
    std::basic_ostream<bufferedTerminal> cout;
//    std::basic_ostream<terminal> cout;
    std::lineTerminator<CRLF> endl;
}

using button0 = Button<0, AVR::Pin<PortA, 3>>;
using button1 = Button<1, AVR::Pin<PortA, 4>>;
using buttonController = ButtonController<button0, button1>;

using dcfPin = AVR::Pin<PortA, 7>;
using dcfDecoder = DCF77<dcfPin, Config::Timer::frequency, EventManager, true>;

using adc = AVR::Adc<0>;
using adcController = AdcController<adc, 6>;
using lm35 = LM35<adcController, 0>;

using TwiMaster = TWI::Master<0>;
using TwiMasterAsync = TWI::MasterAsync<TwiMaster>;
using ds1307 = DS1307<TwiMasterAsync>;

static constexpr TWI::Address i2cramAddress{0x53};
using i2cram = I2CRam<TwiMasterAsync, i2cramAddress>;

static constexpr TWI::Address i2cledAddress{0x54};
using i2cled = I2CRam<TwiMasterAsync, i2cledAddress, I2CLedParameter>;

static constexpr TWI::Address i2crpmAddress{0x55};
using i2crpm = I2CRam<TwiMasterAsync, i2crpmAddress, I2CRpmParameter>;

using vrAdapter = VariableRateAdapter<bufferedTerminal, adcController>;

using ws2812_A = AVR::Pin<PortC, 2>;
//using ws2812_B = AVR::Pin<PortC, 2>;

using ppmInputPin = AVR::Pin<PortC, 3>;
using ppmPinSet = AVR::PinSet<ppmInputPin>;
using pinChangeHandlerPpm = AVR::PinChange<ppmPinSet>;
using ppmTimerInput = AVR::Timer8Bit<2>;
using ppm1 = PpmDecoder<pinChangeHandlerPpm, ppmTimerInput>;
using ppmSwitch = PpmSwitch<0, ppm1>;

using systemClock = AVR::Timer8Bit<0>;
using systemTimer = AlarmTimer<systemClock>;

using sensorUsart = AVR::Usart<0, Hott::SensorProtocollAdapter<0>> ;
using rcUsart = AVR::Usart<1, Hott::SumDProtocollAdapter<0>>;


using ppmTimerOutput = AVR::Timer16Bit<3>;
using ppmPin1 = AVR::Pin<PortC, 4>;
using ppmPin2 = AVR::Pin<PortC, 5>;
using softPpm = SoftPPM<ppmTimerOutput, ppmPin1, ppmPin2>;

// todo: testen, ob das mit PpmChange vereinbar ist
using hardPwm = AVR::PWM<2>;

using crTimer = AVR::Timer16Bit<1>;
using crWriterSensorBinary = ConstanteRateWriter<Hott::SensorProtocollBuffer<0>, sensorUsart>;
using crWriterSensorText = ConstanteRateWriter<Hott::SensorTextProtocollBuffer<0>, sensorUsart>;
//using crAdapter = ConstantRateAdapter<crTimer, AVR::ISR::Timer<1>::CompareA, crWriterSensorBinary, crWriterSensorText, TestBitShifter<crTestPin, 0x55>>;
using crAdapterHott = ConstantRateAdapter<crTimer, AVR::ISR::Timer<1>::CompareA, crWriterSensorBinary, crWriterSensorText>;
using crAdapterOneWire = ConstantRateAdapter<void, AVR::ISR::Timer<1>::CompareA, oneWireMasterAsync>;

using isrDistributor = IsrDistributor<AVR::ISR::Timer<1>::CompareA, crAdapterHott, crAdapterOneWire>;

using softPwmPin1 = AVR::Pin<PortB, 5>;
using softPwmPin2 = AVR::Pin<PortB, 6>;
using softPwm = SoftPWM<softPwmPin1, softPwmPin2>;

using sampler = PeriodicGroup<AVR::ISR::Timer<0>::CompareA, buttonController, systemTimer, dcfDecoder, softPwm>; // werden alle resolution ms aufgerufen

using led0 = AVR::Pin<PortC, 6>;
using led1 = AVR::Pin<PortC, 7>;

//using rpm = RpmFromInterruptSource<0, ppmTimerInput>;

struct EventHandlerParameter {
    std::optional<uint7_t> timerId1;
};

struct I2CRpmHandler: public EventHandler<EventType::I2CRpmValueAvailable> {
    static void process(uint8_t v) {
        std::cout << "i2c rpm value: "_pgm << v << std::endl;
    }  
};

struct I2CRpmErrorHandler: public EventHandler<EventType::I2CRpmError> {
    static void process(uint8_t) {
        std::cout << "i2c rpm error"_pgm << std::endl;
    }
};

struct I2CLedHandler: public EventHandler<EventType::I2CLedValueAvailable> {
    static void process(uint8_t v) {
        std::cout << "i2c led value: "_pgm << v << std::endl;
    }  
};

struct I2CLedErrorHandler: public EventHandler<EventType::I2CLedError> {
    static void process(uint8_t) {
        std::cout << "i2c led error"_pgm << std::endl;
    }
};

struct I2CRamHandler: public EventHandler<EventType::I2CRamValueAvailable> {
    static void process(uint8_t v) {
        std::cout << "i2c ram value: "_pgm << v << std::endl;
    }  
};

struct I2CRamErrorHandler: public EventHandler<EventType::I2CRamError> {
    static void process(uint8_t) {
        std::cout << "i2c ram error"_pgm << std::endl;
    }
};

struct DCFReceive0Handler : public EventHandler<EventType::DCFReceive0> {
    static void process(uint8_t n) {
        std::cout << "dcf 0 : "_pgm << n << std::endl;
    }  
};
struct DCFReceive1Handler : public EventHandler<EventType::DCFReceive1> {
    static void process(uint8_t n) {
        std::cout << "dcf 1 : "_pgm << n << std::endl;
    }  
};
struct DCFSyncHandler : public EventHandler<EventType::DCFSync> {
    static void process(uint8_t) {
        std::cout << "dcf sync  "_pgm << dcfDecoder::dateTime() << std::endl;
    }  
};
struct DCFErrorHandler : public EventHandler<EventType::DCFError> {
    static void process(uint8_t) {
        std::cout << "dcf error"_pgm << std::endl;
    }  
};
struct DCFParityHandler : public EventHandler<EventType::DCFParityError> {
    static void process(uint8_t) {
        std::cout << "dcf parity error"_pgm << std::endl;
    }  
};

struct DS1307handler: public EventHandler<EventType::DS1307TimeAvailable> {
    static void process(uint8_t) {
        std::cout << "ds1307 time"_pgm << std::endl;
    }  
};

struct DS1307handlerError: public EventHandler<EventType::DS1307Error> {
    static void process(uint8_t) {
        std::cout << "ds1307 error"_pgm << std::endl;
    }  
};

struct TWIHandlerError: public EventHandler<EventType::TWIError> {
    static void process(uint8_t) {
        std::cout << "twi error"_pgm << std::endl;
    }  
};

class DS18B20MeasurementHandler: public EventHandler<EventType::DS18B20Measurement> {
public:
    static void process(uint8_t) {
        std::cout << "t: " << ds18b20::temperature() << std::endl;
    }
};
class DS18B20ErrorHandler: public EventHandler<EventType::DS18B20Error> {
public:
    static void process(uint8_t) {
        std::cout << "t: error" << std::endl;
    }
};

class Button0Handler: public EventHandler<EventType::ButtonPress0> {
public:
    static void process(uint8_t) {
        std::cout << "button 0 press"_pgm << std::endl;
    }
};

class HottBinaryHandler : public EventHandler<EventType::HottBinaryRequest> {
public:
    static void process(uint8_t) {
//        std::cout << "hbb"_pgm << std::endl;
        crWriterSensorBinary::enable<true>();
        crWriterSensorText::enable<false>();
        crAdapterHott::start();
    }
};

class HottKeyHandler : public EventHandler<EventType::HottAsciiKey> {
public:
    static void process(uint8_t v) {
        std::cout << "k: "_pgm << v << std::endl;
//        Hott::SensorProtocoll<sensorUsart>::key(v);
    }
};

class HottBroadcastHandler : public EventHandler<EventType::HottSensorBroadcast> {
public:
    static void process(uint8_t) {
        std::cout << "hbr"_pgm << std::endl;
        crWriterSensorBinary::enable<true>();
        crWriterSensorText::enable<false>();
        crAdapterHott::start();
    }
};

class HottTextHandler : public EventHandler<EventType::HottAsciiRequest> {
public:
    static void process(uint8_t) {
        std::cout << "hba"_pgm << std::endl;
        crWriterSensorBinary::enable<false>();
        crWriterSensorText::enable<true>();
        crAdapterHott::start();
    }
};

decltype(systemTimer::create(1000_ms, AlarmFlags::Periodic)) pTimer;
decltype(systemTimer::create(1000_ms, AlarmFlags::Periodic)) mTimer;
decltype(systemTimer::create(1000_ms, AlarmFlags::Periodic)) tTimer;

class TimerHandler : public EventHandler<EventType::Timer> {
public:
    static void process(uint8_t timer) {
        if (timer == *pTimer) {

            std::cout << "unused: "_pgm << Util::Memory::getUnusedMemory() << std::endl;   

            static uint8_t count = 0;
            WS2812<2, ws2812_A>::set({16, (uint8_t)((count++ % 2) * 16), 16});
            
            if (count % 2) {
                i2cram::startWrite(0, count);
                i2cled::startWrite(0, count * 64);
//                i2crpm::startWrite(0, 0);
            }
            else {
                if (!i2cram::startRead(0)) {
                    std::cout << "start read ram error"_pgm << std::endl;
                }
                if (!i2cled::startRead(0)) {
                    std::cout << "start read led error"_pgm << std::endl;
                }
                if (!i2crpm::startRead(0)) {
                    std::cout << "start read rpm error"_pgm << std::endl;
                }
            }
            
            ds1307::startReadTimeInfo();
            
            std::cout << "Temp lm35: "_pgm << lm35::temperature() << std::endl;
            std::cout << "ppm:"_pgm << ppm1::value<0>() << std::endl;
            std::cout << "c0: "_pgm << Hott::SumDProtocollAdapter<0>::value8Bit(0) << std::endl;
    //        std::cout << "c1: "_pgm << Hott::SumDProtocollAdapter<0>::value8Bit(1) << std::endl;
    //        std::cout << "c2: "_pgm << Hott::SumDProtocollAdapter<0>::value8Bit(2) << std::endl;
    //        std::cout << "c3: "_pgm << Hott::SumDProtocollAdapter<0>::value8Bit(3) << std::endl;
    //        std::cout << "c4: "_pgm << Hott::SumDProtocollAdapter<0>::value8Bit(4) << std::endl;
    //        std::cout << "c5: "_pgm << Hott::SumDProtocollAdapter<0>::value8Bit(5) << std::endl;
    //        std::cout << "nn: "_pgm << Hott::SumDProtocollAdapter<0>::numberOfChannels() << std::endl;
    
            std::percent pv = std::scale(Hott::SumDProtocollAdapter<0>::value8Bit(0),
                                   Hott::SumDMsg::Low8Bit, Hott::SumDMsg::High8Bit);
            std::cout << "pv: " << pv.value << std::endl;
            softPpm::ppm(pv, 0);
    
            softPwm::pwm(pv, 0);
            softPwm::pwm(pv, 1);
            std::cout << "spwm period: "_pgm << softPwm::period() << std::endl;
        }
        else if (timer == *tTimer) {
            if (ds18b20::convert()) {
                systemTimer::start(*mTimer);
            }
        }
        else if (timer == *mTimer) {
            ds18b20::startGet(dsIds[0]);
        }
    }
};

class TestHandler : public EventHandler<EventType::Test> {
public:
    static void process(uint8_t) {
        std::cout << "t"_pgm << std::endl;
    }
};

class PpmUpHandler : public EventHandler<EventType::Ppm1Up> {
public:
    static void process(uint8_t) {
        std::cout << "ppm1up"_pgm << std::endl;
    }
};
class PpmDownHandler : public EventHandler<EventType::Ppm1Down> {
public:
    static void process(uint8_t) {
        std::cout << "ppm1down"_pgm << std::endl;
    }
};
class UsartHandler : public EventHandler<EventType::UsartRecv0> {
public:
    static void process(uint8_t v) {
        std::cout << "u: "_pgm << v << std::endl;
    }
};

using isrRegistrar = IsrRegistrar<ppm1, isrDistributor, sampler, 
                                  sensorUsart::RxHandler, sensorUsart::TxHandler, rcUsart::RxHandler, rcUsart::TxHandler,
                                  softPpm::OCAHandler, softPpm::OCBHandler>;

int main()
{
    isrRegistrar::init();
    Scoped<EnableInterrupt> interruptEnabler;

    systemTimer::init();
//    terminal::init<0>();
    bufferedTerminal::init<0>();

    sensorUsart::init<19200>();
    rcUsart::init<115200>();
    SSpi0::init();
    buttonController::init();
    adcController::init();
    
//    rpm::init();
    
    using namespace std::literals::quantity;
    softPpm::init();
    softPpm::ppm(50_ppc, 0);

    softPwm::init();
    
    hardPwm::init();
    hardPwm::pwm<hardPwm::A>(90_ppc);
    hardPwm::pwm<hardPwm::B>(50_ppc);

//    testPin::dir<AVR::Output>();

    ds1307::init();
    ds1307::squareWave<true>();

    TwiMaster::init<ds1307::fSCL>();
    
    i2cram::init<ds1307::fSCL>();
    i2cled::init<ds1307::fSCL>();
    i2crpm::init<ds1307::fSCL>();
    
    std::array<TWI::Address, 5> i2cAddresses;
    TwiMaster::findDevices(i2cAddresses);
    for(const auto& d : i2cAddresses) {
        std::cout << d << std::endl;
    }
    
    dcfDecoder::init();

    constexpr std::hertz fCr = 1 / Hott::hottDelayBetweenBytes;
    constexpr auto tsd = AVR::Util::calculate<crTimer>(fCr);
    static_assert(tsd, "wrong parameter");
    crTimer::prescale<tsd.prescaler>();
    crTimer::ocra<tsd.ocr>();

    crAdapterHott::init();
    crAdapterOneWire::init();
    
    ds18b20::init();

    oneWireMaster::findDevices(dsIds);
    for(const auto& id : dsIds) {
        std::cout << id << std::endl;
    }
            
    led0::dir<AVR::Output>();
    led1::dir<AVR::Output>();

    pinChangeHandlerPpm::init();
    PpmDecoder<pinChangeHandlerPpm, ppmTimerInput>::init();

    std::cout << "RC Controller 0.1"_pgm << std::endl;

    WS2812<2, ws2812_A>::init();
    WS2812<2, ws2812_A>::off();

//    WS2812<2, ws2812_B>::init();
//    WS2812<2, ws2812_B>::off();

    pTimer = systemTimer::create(1000_ms, AlarmFlags::Periodic);
    tTimer = systemTimer::create(5000_ms, AlarmFlags::Periodic);
    mTimer = systemTimer::create(750_ms, AlarmFlags::OneShot);
    systemTimer::stop(*mTimer);
    
//    std::cout << Config() << std::endl;

    using handler = EventHandlerGroup<TimerHandler,
                                HottBinaryHandler, HottBroadcastHandler, HottTextHandler, TestHandler,
                                PpmDownHandler, PpmUpHandler,
                                UsartHandler, HottKeyHandler,
                                Button0Handler, 
                                ds18b20, DS18B20ErrorHandler, DS18B20MeasurementHandler,
                                TWIHandlerError, ds1307, DS1307handler, DS1307handlerError, i2cram, i2cled, i2crpm,
                                DCFReceive0Handler, DCFReceive1Handler, DCFSyncHandler, DCFErrorHandler, DCFParityHandler,
                                I2CRamHandler, I2CRamErrorHandler,
                                I2CLedHandler, I2CLedErrorHandler,
                                I2CRpmHandler, I2CRpmErrorHandler>;

    EventManager::run<sampler, handler>([](){
//        led0::toggle();
        ppmSwitch::process(ppm1::value<0>());
        softPwm::freeRun();
        crAdapterHott::periodic();
        vrAdapter::periodic();
        crAdapterOneWire::periodic();
        TwiMasterAsync::periodic();
    });

    return 0;
}

#ifndef NDEBUG
void assertFunction(const PgmStringView& expr, const PgmStringView& file, unsigned int line) {
    std::cout << "Assertion failed: "_pgm << expr << ',' << file << ',' << line << std::endl;
    while(true) {}
}
#endif

ISR(PCINT0_vect) {
//    isrRegistrar::isr<AVR::ISR::PcInt<0>>();
}
ISR(PCINT1_vect) {
//    isrRegistrar::isr<AVR::ISR::PcInt<1>>();
}
ISR(PCINT2_vect) {
    led1::toggle();
    isrRegistrar::isr<AVR::ISR::PcInt<2>>();
}
ISR(PCINT3_vect) {
//    isrRegistrar::isr<AVR::ISR::PcInt<3>>();
}
ISR(TIMER1_COMPA_vect) {
    isrRegistrar::isr<AVR::ISR::Timer<1>::CompareA>();
//    SWUsart<0>::isr_compa();
}
ISR(TIMER1_COMPB_vect) {
//    isrRegistrar::isr<AVR::ISR::Timer<1>::CompareB>();
//    SWUsart<0>::isr_compb();
}
ISR(TIMER1_CAPT_vect) {
//    isrRegistrar::isr<AVR::ISR::Timer<1>::Capture>();
//    SWUsart<0>::isr_icp();
}
ISR(SPI_STC_vect) {
//    isrRegistrar::isr<AVR::ISR::Spi<0>::Stc>();
//    AVR::Spi<0>::isr();
}
ISR(TIMER0_COMPA_vect) {
    // todo: auf isrRegistrar umbauen
    isrRegistrar::isr<AVR::ISR::Timer<0>::CompareA>();
//    sampler::isr();
}

// todo: isrReg

ISR(TIMER3_COMPA_vect) {
    isrRegistrar::isr<AVR::ISR::Timer<3>::CompareA>();
//    softPpm::isrA();
}
ISR(TIMER3_COMPB_vect) {
    isrRegistrar::isr<AVR::ISR::Timer<3>::CompareB>();
//    softPpm::isrB();
}

ISR(USART0_RX_vect) {
    isrRegistrar::isr<AVR::ISR::Usart<0>::RX>();
//    sensorUsart::rx_isr();
}
ISR(USART0_UDRE_vect){
    isrRegistrar::isr<AVR::ISR::Usart<0>::UDREmpty>();
//    sensorUsart::tx_isr();
}
ISR(USART1_RX_vect) {
    isrRegistrar::isr<AVR::ISR::Usart<1>::RX>();
//    rcUsart::rx_isr();
}
ISR(USART1_UDRE_vect){
    isrRegistrar::isr<AVR::ISR::Usart<1>::UDREmpty>();
//    rcUsart::tx_isr();
}
ISR(ANALOG_COMP_vect) {
//    led1::toggle();
//    isrRegistrar::isr<AVR::ISR::AdComparator<0>::Edge>();
}

