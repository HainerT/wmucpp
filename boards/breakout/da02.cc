#define NDEBUG

#include <mcu/avr.h>
#include <mcu/common/delay.h>

#include <mcu/internals/ccp.h>
#include <mcu/internals/clock.h>
#include <mcu/internals/systemclock.h>
#include <mcu/internals/usart.h>
#include <mcu/internals/portmux.h>
#include <mcu/internals/adc.h>
#include <mcu/internals/pwm.h>
#include <mcu/internals/eeprom.h>
#include <mcu/internals/spi.h>
#include <mcu/internals/ccl.h>
#include <mcu/internals/sigrow.h>
#include <mcu/internals/event.h>
#include <mcu/internals/syscfg.h>
#include <mcu/internals/twi.h>

#include <external/hal/adccontroller.h>
#include <external/hal/alarmtimer.h>
#include <external/bluetooth/roboremo.h>
#include <external/bluetooth/qtrobo.h>
#include <external/sbus/sbus.h>
#include <external/sbus/sport.h>
#include <external/ibus/ibus2.h>
#include <external/hott/sumdprotocolladapter.h>
#include <external/hott/experimental/sensor.h>
#include <external/hott/experimental/adapter.h>
#include <external/hott/menu.h>

#include <external/solutions/button.h>
#include <external/solutions/blinker.h>
#include <external/solutions/analogsensor.h>
#include <external/solutions/series01/swuart.h>
#include <external/solutions/series01/rpm.h>
#include <external/solutions/apa102.h>
#include <external/solutions/series01/sppm_in.h>
#include <external/solutions/rc/busscan.h>
#include <external/solutions/series01/cppm_out.h>
#include <external/solutions/rc/rf.h>
#include <external/solutions/ad9851.h>
#include <external/solutions/si5351.h>

#include <mcu/pgm/pgmarray.h>

#include <std/chrono>
#include <std/bit>

#include <etl/output.h>
#include <etl/meta.h>

using namespace AVR;
using namespace std::literals::chrono;
using namespace External::Units::literals;

#ifndef NDEBUG
namespace xassert {
    etl::StringBuffer<160> ab;
    etl::StringBuffer<10> aline;
    bool on{false};
}
#endif

namespace  {
    constexpr auto fRtc = 1000_Hz;
    constexpr Twi::Address mcp23008{39};
    constexpr Twi::Address si5351{0x60};
}

using ccp = Cpu::Ccp<>;
using clock = Clock<>;
using sigrow = SigRow<>;

using systemTimer = SystemTimer<Component::Rtc<0>, fRtc>;

using dbgPin = Pin<Port<A>, 6>;

//using a0Pin = Pin<Port<A>, 0>; // usart0
//using a1Pin = Pin<Port<A>, 1>;
//using a2Pin = Pin<Port<A>, 2>;

using a5Pin = Pin<Port<A>, 5>;

using dataPin = Pin<Port<D>, 7>;
using dataAct = ActiveHigh<dataPin, Output>;

using clkPin = Pin<Port<C>, 0>;
using clkAct = ActiveHigh<clkPin, Output>;
using clkSig = SinglePulse<clkAct>;

using fupPin = Pin<Port<C>, 1>;
using fupAct = ActiveHigh<fupPin, Output>;
using fupSig = SinglePulse<fupAct>;

using rstPin = Pin<Port<C>, 2>;
using rstAct = ActiveHigh<rstPin, Output>;
using rstSig = SinglePulse<rstAct>;

using usart0Position = Portmux::Position<Component::Usart<0>, Portmux::Default>; // Sensor
using tdev = Usart<usart0Position, External::Hal::NullProtocollAdapter, AVR::UseInterrupts<false>, AVR::ReceiveQueueLength<16>, AVR::SendQueueLength<256>>;
using terminal = etl::basic_ostream<tdev>;

using tca0Position = AVR::Portmux::Position<AVR::Component::Tca<0>, Portmux::Default>;
using cppm = External::Ppm::Cppm<tca0Position, std::integral_constant<uint8_t, 16>, AVR::UseInterrupts<true>>;
using lut0 = Ccl::SimpleLut<0, Ccl::Input::Mask, Ccl::Input::Tca0<1>, Ccl::Input::Mask>;    

using twi0Position = Portmux::Position<Component::Twi<0>, Portmux::Default>;
using twi = AVR::Twi::Master<twi0Position>;

using portmux = Portmux::StaticMapper<Meta::List<tca0Position, twi0Position>>;

//using ad9851 = External::DDS::AD9851<systemTimer, dataAct, clkSig, fupSig, rstSig>;
using si = External::SI5351::Clock<twi, si5351>;

template<typename MCU = DefaultMcuType>
struct Fsm {
    using ch_t = cppm::channel_t;
    using rv_t = cppm::ranged_type;
    
    enum class State : uint8_t {Undefined, Init, Set, Set2, Run};
    
    static inline constexpr External::Tick<systemTimer> mInitTicks{500_ms};
    static inline constexpr External::Tick<systemTimer> mChangeTicks{100_ms};
    static inline constexpr External::Tick<systemTimer> mDebugTicks{500_ms};
    
    static inline void init() {
        dbgPin::dir<Output>();
        a5Pin::dir<Output>();
//        ad9851::init();
        twi::init();
        tdev::init<BaudRate<9600>>();
        
        cppm::set(ch_t{0}, rv_t{cppm::ocMedium});
        cppm::set(ch_t{1}, rv_t{cppm::ocMedium});
        cppm::set(ch_t{2}, rv_t{cppm::ocMedium});
        cppm::set(ch_t{3}, rv_t{cppm::ocMedium});
        cppm::set(ch_t{4}, rv_t{cppm::ocMedium});
        cppm::set(ch_t{5}, rv_t{cppm::ocMedium});
        cppm::set(ch_t{6}, rv_t{cppm::ocMedium});
        cppm::set(ch_t{7}, rv_t{cppm::ocMedium});
        
        etl::outl<terminal>("dds10"_pgm);
    }
    static inline void periodic() {
        dbgPin::toggle();
        tdev::periodic();
//        ad9851::periodic();
        twi::periodic();
        si::periodic();
    }
    static inline void ratePeriodic() {
//        ad9851::ratePeriodic();
        const auto oldState = mState;
        ++mStateTick;
        ++mChangeTick;
        (++mDebugTick).on(mDebugTicks, []{
            etl::outl<terminal>("v: "_pgm, vv.toInt());
        });
        switch(mState) {
        case State::Undefined:
            mStateTick.on(mInitTicks, []{
               mState = State::Init; 
            });
            break;
        case State::Init:
            mStateTick.on(mInitTicks, []{
               mState = State::Set; 
            });
            break;
        case State::Set:
            if (si::setChannel(0)) {
                si::setOutput(1);
                mState = State::Set2;
            }
//            if (si::setupWithClockBuilderData()) {
//                mState = State::Run;
//            }
            break;
        case State::Set2:
            if (si::setChannelUpperFreq(0)) {
                mState = State::Run;
            }
            break;
//        case State::Set:
////            if (ad9851::ready()) {
////                mState = State::Run;
////            }
//            break;
        case State::Run:
            mChangeTick.on(mChangeTicks, []{
                cppm::set(ch_t{0}, vv);
                vv += 10;
                if (vv.isTop()) {
                    vv.setToBottom();
                }
            });
            break;
        }
        if (oldState != mState) {
            mStateTick.reset();
            switch(mState) {
            case State::Undefined:
                break;
            case State::Init:
                break;
            case State::Set:
//                ad9851::channel(ad9851::index_type{0});
                break;
            case State::Run:
                cppm::init();
                lut0::init(0x33_B); // invert
        //        lut0::enable(); // nicht notwendig
                break;
            }
        } 
    }
private:
    static inline rv_t vv{cppm::ocMedium};    
    static inline State mState{State::Undefined};
    static inline External::Tick<systemTimer> mStateTick;
    static inline External::Tick<systemTimer> mDebugTick;
    static inline External::Tick<systemTimer> mChangeTick;
};

using fsm = Fsm<>;

struct Dummy {
    inline static void once() {}
};

//using isrRegistrar = IsrRegistrar<cppm::CmpHandler<ad9851::SetBaseFrequency>, cppm::OvfHandler<ad9851::SetUpperFrequency>>;
using isrRegistrar = IsrRegistrar<cppm::CmpHandler<Dummy>, cppm::OvfHandler<Dummy>>;

int main() {
    portmux::init();
    ccp::unlock([]{
         clock::template init<Project::Config::fMcuMhz>();
    });
    systemTimer::init();
    
    fsm::init();
    {
        etl::Scoped<etl::EnableInterrupt<>> ei;
        while(true) {
            fsm::periodic();                        
            systemTimer::periodic([&]{
                fsm::ratePeriodic();                        
            });
        }
    }    
}

ISR(TCA0_OVF_vect) {
    a5Pin::on();
    isrRegistrar::isr<AVR::ISR::Tca<0>::Ovf>();
}
ISR(TCA0_CMP1_vect) {
    a5Pin::off();
    isrRegistrar::isr<AVR::ISR::Tca<0>::Cmp<1>>();
}
