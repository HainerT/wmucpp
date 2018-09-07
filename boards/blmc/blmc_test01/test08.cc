#define NDEBUG

#include <stdlib.h>
#include <util/delay_basic.h>

#include "mcu/avr8.h"
#include "mcu/ports.h"
#include "mcu/avr/usart.h"
#include "mcu/avr/mcupwm.h"
#include "mcu/avr/adcomparator.h"
#include "mcu/avr/adc.h"
#include "mcu/avr/watchdog.h"
#include "container/fixedvector.h"
#include "util/fixedpoint.h"
#include "console.h"

using PortB = AVR::Port<DefaultMcuType::PortRegister, AVR::B>;
using PortC = AVR::Port<DefaultMcuType::PortRegister, AVR::C>;
using PortD = AVR::Port<DefaultMcuType::PortRegister, AVR::D>;

using pinLow0 = AVR::Pin<PortD, 3>;
using pinHigh0 = AVR::Pin<PortB, 4>; // mod
using pinLow1 = AVR::Pin<PortD, 4>;
using pinHigh1 = AVR::Pin<PortB, 2>;
using pinLow2 = AVR::Pin<PortD, 5>;
using pinHigh2 = AVR::Pin<PortD, 2>;

using led =  AVR::Pin<PortD, 7>;

using db0 =  AVR::Pin<PortB, 0>;
using db1 =  AVR::Pin<PortB, 6>;
using db2 =  AVR::Pin<PortB, 7>;

namespace Constants {
    static constexpr std::hertz pwmFrequency = 20000_Hz * 256; 
    static constexpr std::hertz fSystem = 100_Hz;
}

struct CommandAdapter {
    enum class Command : uint8_t {Undefined, Off, Start, Info, Reset, 
                                  IncPwm, DecPwm, IncDelay, DecDelay};
    
    static inline bool process(std::byte v) {
        switch (v) {
        case std::byte{'s'}:
            mCommand = Command::Start;
            break;
        case std::byte{'o'}:
            mCommand = Command::Off;
            break;
        case std::byte{'i'}:
            mCommand = Command::Info;
            break;
        case std::byte{'r'}:
            mCommand = Command::Reset;
            break;
        case std::byte{'p'}:
            mCommand = Command::DecPwm;
            break;
        case std::byte{'P'}:
            mCommand = Command::IncPwm;
            break;
        case std::byte{'d'}:
            mCommand = Command::DecDelay;
            break;
        case std::byte{'D'}:
            mCommand = Command::IncDelay;
            break;
        default:
            break;
        }        
        return true;
    }
    
    static inline Command get() {
        Command c = Command::Undefined;
        {
            Scoped<DisbaleInterrupt<>> di;
            c = mCommand;
            mCommand = Command::Undefined;
        }
        return c;
    }
    
private:
    inline static volatile Command mCommand = Command::Undefined;
};

using uart = AVR::Usart<0, CommandAdapter, MCU::UseInterrupts<true>, UseEvents<false>, 
                       AVR::ReceiveQueueLength<0>, AVR::SendQueueLength<128>>;
using terminalDevice = uart;
using terminal = std::basic_ostream<terminalDevice>;

template<typename AC, typename Timer, typename... PP>
struct Communter {
    using pin_list = Meta::List<PP...>;
    using h0 = Meta::nth_element<0, pin_list>;
    using h1 = Meta::nth_element<1, pin_list>;
    using h2 = Meta::nth_element<2, pin_list>;
    using l0 = Meta::nth_element<3, pin_list>;
    using l1 = Meta::nth_element<4, pin_list>;
    using l2 = Meta::nth_element<5, pin_list>;
    
    using highSides = Meta::List<h0, h1, h2>;
    using lowSides  = Meta::List<l0, l1, l2>;
    
    template<typename Pin>
    requires Meta::contains<highSides, Pin>::value
    inline static void pwm() {
        Pin::template dir<AVR::Input>();
    }
    template<typename Pin>
    requires Meta::contains<highSides, Pin>::value
    inline static void off() {
        Pin::template dir<AVR::Output>();
    }
    template<typename Pin>
    requires Meta::contains<lowSides, Pin>::value
    inline static void floating() {
        Pin::off();
    }
    template<typename Pin>
    requires Meta::contains<lowSides, Pin>::value
    inline static void on() {
        Pin::on();
    }
    
    inline static void init() {
        ((PP::off(), ...));
        ((PP::template dir<AVR::Output>(), ...));
        AC::init();
        AC::enableCapture();
    }
    inline static void enable() {
        AC::init();
    }
    inline static void disable() {
        AC::disable();
    }
    inline static void off() {
        ((PP::template dir<AVR::Output>(), ...));
        (PP::off(), ...);
    }
    inline static void startPosition() {
        state = 0;
    }
    inline static void on() {
        switch(state) {
        case 0: // pwm(0) -> 2, ac = 1, rising
            floating<l1>();
            on<l2>();
            AC::channel(1);
            Timer::captureRising(true);
            break;
        case 1: // pwm(1) -> 2, ac = 0
            off<h0>();
            pwm<h1>();
            AC::channel(0);
            Timer::captureRising(false);
            break;
        case 2: // pwm(1) -> 0, ac = 2
            floating<l2>();
            on<l0>();
            AC::channel(2);
            Timer::captureRising(true);
            break;
        case 3: // pwm(2) -> 0, ac = 1
            off<h1>();
            pwm<h2>();
            AC::channel(1);
            Timer::captureRising(false);
            break;
        case 4: // pwm(2) -> 1, ac = 0
            floating<l0>();
            on<l1>();
            AC::channel(0);
            Timer::captureRising(true);
            break;
        case 5: // pwm(0) -> 1, ac = 2
            off<h2>();
            pwm<h0>();
            AC::channel(2);
            Timer::captureRising(false);
            break;
        default:
            break;
        }
    }
    
    inline static void next() {
        ++state;
        on();
    }
    
private:
    inline static uint_ranged_circular<uint8_t, 0, 5> state{0};
};

using namespace std::literals::quantity;

template<typename Timer, typename PWM, typename Com, typename Adc>
struct Controller {
    typedef typename Timer::value_type timer_value_t;
    typedef typename PWM::value_type pwm_value_t;
    
    static inline constexpr uint8_t pwm_max = std::numeric_limits<pwm_value_t>::max();
    
    struct RampValue {
        timer_value_t tv;
        FixedPoint<uint16_t, 8> pwm;
        
        inline RampValue& operator+=(const RampValue& rhs) {
            tv -= rhs.tv;
            pwm += rhs.pwm;
            return *this;
        }
    };
    
    struct Ramp {
        RampValue start;
        RampValue end;
        RampValue delta;
        uint8_t   steps;
    };
    
    struct RampPoint {
        std::microseconds time;
        std::percent pvm;
    };
    
    enum class State : uint8_t {Off, Align, RampUp, ClosedLoop, ClosedLoopComDelay, ClosedLoopCommute, ClosedLoopBeforeDetectDelay, 
                                ClosedLoopAdcSample, ClosedLoopError};
    
    static inline constexpr uint16_t prescaler = 8;
    static inline constexpr auto fTimer = Config::fMcu / prescaler;
    
    static inline RampPoint ramp_start = {20000_us, 3_ppc};
    static inline RampPoint ramp_end   = {3000_us, 6_ppc};
    static inline uint8_t ramp_steps = 50;
    
    static inline auto pwmStart = FixedPoint<uint16_t, 8>{(double)std::expand(5_ppc, uint8_t{0}, pwm_max)};
    
    inline static RampValue convert(const RampPoint& p) {
        RampValue v;
        v.tv = p.time * fTimer;
        v.pwm = FixedPoint<uint16_t, 8>{(double)std::expand(p.pvm, uint8_t{0}, pwm_max)};
        return v;
    } 
    
    inline static Ramp make_ramp(const RampPoint& start, const RampPoint& end, uint8_t steps) {
        Ramp ramp;
        ramp.start = convert(start);
        ramp.end = convert(end);
        ramp.steps = steps;
        ramp.delta.tv = (ramp.start.tv - ramp.end.tv) / steps;
        ramp.delta.pwm = (ramp.end.pwm - ramp.start.pwm) / steps;
        return ramp;
    }
    
    inline static Ramp ramp = make_ramp(ramp_start, ramp_end, ramp_steps);
    
    inline static RampValue actualRV = ramp.start;
    
    static inline constexpr uint16_t alignValue = 50_ms * fTimer;
    
    inline static void init() {
        Adc::init();    
        PWM::template init<Constants::pwmFrequency>();
        PWM::template pwm<typename PWM::A>(0);
        Com::init();
        Timer::off();
        Timer::noiseCancel();
        Timer::mode(AVR::TimerMode::IcpNoInt); 
    }
    inline static void run() {
        spin();
        if ((mState != State::Off)) {
            Timer::template periodic<Timer::flags_type::ocfa>([&](){
                periodic();    
            });            
        }
    }
private:  
    enum class Channel : uint8_t {Temp = 3, Current = 6, Voltage = 7};
    
    
    inline static void off() {
        Com::off();
        Timer::off();
        mState = State::Off;
    }
    inline static void start() {
        actualRV = ramp.start;
        Timer::reset();
        Timer::ocra(alignValue);
        Timer::template prescale<prescaler>();
        mState = State::Align;
        mStep = 0;
    }
    
    inline static uint16_t lastCounter = 0;
    inline static uint16_t period = 0;

    inline static void updatePeriod() {
        uint16_t actual = Timer::icr();
        if (actual >= lastCounter) {
            period = actual - lastCounter;
        }
        else {
            period = (std::numeric_limits<uint16_t>::module() - lastCounter) + actual;
        }
        lastCounter = actual;
    }
    
    inline static void enableAdc() {
        Adc::channel(uint8_t(mChannel)); 
        Com::disable();
        Adc::enable();
        Adc::startConversion();
    }
    
    inline static void enableAC() {
        Adc::disable();
        Com::enable();
    }
    
    inline static void spin() {
        switch(mState) {
        case State::ClosedLoop:
            Timer::template periodic<Timer::flags_type::icf>([&](){ // zero crossing
                enableAdc();                
                updatePeriod();   
                uint16_t delay = std::min(period / mDelayFactor, uint16_t{2500}); // todo: Konstante
                Timer::ocra(Timer::counter() + delay);
                Timer::template clearFlag<Timer::flags_type::ocfa>();
                mState = State::ClosedLoopComDelay;
            });
            break;
        case State::ClosedLoopCommute:
            Com::next();
            
        {
            uint16_t delay = std::min(period / 2, uint16_t{100});
            Timer::ocra(Timer::counter() + delay);
            Timer::template clearFlag<Timer::flags_type::ocfa>();
            mState = State::ClosedLoopBeforeDetectDelay;
        }
            break;
        case State::ClosedLoopAdcSample:
            Timer::template clearFlag<Timer::flags_type::icf>();
            mState = State::ClosedLoop;
            
            Adc::whenConversionReady([](auto v) {
                ++mAdcConverions;
                switch(mChannel) {
                case Channel::Temp:
                    mTemp = v;
                    mChannel = Channel::Current;
                    break;
                case Channel::Current:
                    mCurrent= v;
                    mChannel = Channel::Voltage;
                    break;
                case Channel::Voltage:
                    mVoltage= v;
                    mChannel = Channel::Temp;
                    break;
                }
            });

            enableAC();
            
            break;
            
        case State::Off:
            if (Adc::conversionReady()) {
//                Adc::startConversion();
            }
            break;
        default:
            break;
        }
    }
    inline static void periodic() {
        switch(mState) {
        case State::Align:
            Com::startPosition();
            Timer::reset();
            Timer::ocra(alignValue);
            PWM::template pwm<typename PWM::A>(actualRV.pwm.integer());
            Com::on();
            mState = State::RampUp;
            break;
        case State::RampUp:
            Com::next();
            Timer::reset();
            Timer::ocra(actualRV.tv);
            PWM::template pwm<typename PWM::A>(actualRV.pwm.integer());
            if (mStep < ramp_steps) {
                ++mStep;
                actualRV += ramp.delta;
            }
            else {
                mState = State::ClosedLoop;
                actualRV.pwm = pwmStart;
            }
            break;
        case State::ClosedLoop:
            PWM::template pwm<typename PWM::A>(actualRV.pwm.integer());
            break;
        case State::ClosedLoopComDelay:
            PWM::template pwm<typename PWM::A>(actualRV.pwm.integer());
            mState = State::ClosedLoopCommute;
            break;
        case State::ClosedLoopBeforeDetectDelay:
            mState = State::ClosedLoopAdcSample;
            break;
        case State::ClosedLoopError:
            off();
            break;
        default: 
            assert(false);
            break;
        }
    }
    inline static void delayInc() {
        mDelayFactor += 1;
    }
    inline static void delayDec() {
        mDelayFactor = std::max(4, mDelayFactor - 1);
    }
    inline static void tvInc() {
        ramp.end.tv += 100;
    }
    inline static void tvDec() {
        ramp.end.tv -= 100;
    }
    inline static void pwmInc() {
        actualRV.pwm += FixedPoint<uint16_t, 8>(1);
    }
    inline static void pwmDec() {
        actualRV.pwm -= FixedPoint<uint16_t, 8>(1);
    }
    inline static void reset() {
        ramp = make_ramp(ramp_start, ramp_end, ramp_steps);
    }
    inline static State mState = State::Off;
    inline static  uint8_t mStep = 0;
    inline static uint8_t mDelayFactor = 4;
    
    inline static uint8_t mVoltage = 0;
    inline static uint8_t mCurrent = 0;
    inline static uint8_t mTemp = 0;
    
    inline static Channel mChannel = Channel::Temp;
    inline static uint16_t mAdcConverions = 0;
};

using adc = AVR::Adc<0, AVR::Resolution<8>>;

using commutationTimer = AVR::Timer16Bit<1>; // timer 1

using hardPwm = AVR::PWM<2, AVR::NonInverting>; // timer 0

using adcomp = AVR::AdComparator<0>;
using commuter = Communter<adcomp, commutationTimer, pinHigh0, pinHigh1, pinHigh2, pinLow0, pinLow1, pinLow2>;

using controller = Controller<commutationTimer, hardPwm, commuter, adc>;

using isrRegistrar = IsrRegistrar<uart::RxHandler, uart::TxHandler>;

//using systemClock = AVR::Timer8Bit<2>; // timer 2

int main() {
    OSCCAL = 0xff; // 16 MHz
    
    db0::dir<AVR::Output>();
    db1::dir<AVR::Output>();
    db2::dir<AVR::Output>();
    
    //    systemClock::template setup<Constants::fSystem>(AVR::TimerMode::CTCNoInt);
    
    isrRegistrar::init();
    
    uart::init<9600>();
    
    controller::init();  
    
    // watchdog: alles aus
    {
        Scoped<EnableInterrupt<>> ei;
        
        std::outl<terminal>("BL-Ctrl (mod) V08"_pgm);
        
        while(true) {
//            db0::toggle();
            
            controller::run();
            
            if (auto c = CommandAdapter::get(); c != CommandAdapter::Command::Undefined) {
                switch(c) {
                case CommandAdapter::Command::Off:
                    std::outl<terminal>("Off"_pgm);
                    controller::off();
                    break;
                case CommandAdapter::Command::Start:
                    std::outl<terminal>("Start"_pgm);
                    controller::start();
                    break;
                case CommandAdapter::Command::IncPwm:
                    std::outl<terminal>("P"_pgm);
                    controller::pwmInc();
                    break;
                case CommandAdapter::Command::DecPwm:
                    std::outl<terminal>("p"_pgm);
                    controller::pwmDec();
                    break;
                case CommandAdapter::Command::IncDelay:
                    std::outl<terminal>("D"_pgm);
                    controller::delayInc();
                    break;
                case CommandAdapter::Command::DecDelay:
                    std::outl<terminal>("d"_pgm);
                    controller::delayDec();
                    break;
                case CommandAdapter::Command::Reset:
                    std::outl<terminal>("Reset"_pgm);
                    controller::reset();
                    break;
                case CommandAdapter::Command::Info:
//                    std::outl<terminal>("d:"_pgm, controller::mDelayFactor);
//                    std::outl<terminal>("p:"_pgm, controller::actualRV.pwm);
//                    std::outl<terminal>("f:"_pgm, controller::period);
                    std::outl<terminal>("v:"_pgm, controller::mVoltage);
                    std::outl<terminal>("t:"_pgm, controller::mTemp);
                    std::outl<terminal>("c:"_pgm, controller::mCurrent);
                    std::outl<terminal>("n:"_pgm, controller::mAdcConverions);
                    break;
                default:
                    break;
                }
                
            }
        }
    }    
}

ISR(USART_RX_vect) {
    isrRegistrar::isr<AVR::ISR::Usart<0>::RX>();
}
ISR(USART_UDRE_vect){
    isrRegistrar::isr<AVR::ISR::Usart<0>::UDREmpty>();
}
