#define USE_MCU_STM_V2
#define NDEBUG

#include "devices.h"

#include <chrono>
#include <cassert>
 
using namespace std::literals::chrono_literals;

struct Config {
    Config() = delete;
};

struct Data {
    static inline std::array<uint16_t, 64> mChannels{}; // sbus [172, 1812], center = 992
    static inline std::array<uint8_t, 256> mAltData{};    
};

template<typename Devices>
struct GFSM {
    using devs = Devices;
    using trace = devs::trace;
    using systemTimer = devs::systemTimer;
    
    using pwm1 = devs::pwm1;
    using crsf = devs::crsf;
    using crsf_pa = devs::crsf_pa;
    using crsf_out = devs::crsf_out;
    using crsfTelemetry = devs::crsfTelemetry;
    
    using sport = devs::sport;
    using sport_pa = devs::sport_pa;
    using sport_uart = devs::sport_uart;
    
    using sbus = devs::sbus;
    
    using bt_uart = devs::bt_uart;
    using robo_pa = devs::robo_pa;
    
    using led1 = devs::led1;
    using led2 = devs::led2;
    
    using btpwr = devs::btpwr;
    using bten = devs::bten;
    
    using tp1 = devs::tp1;
    using tp2 = devs::tp2;
    
    static inline constexpr External::Tick<systemTimer> debugTicks{500ms};
    static inline constexpr External::Tick<systemTimer> initTicks{500ms};
    
    enum class State : uint8_t {Undefined, Init, Run};
    
    static inline void init() {
        devs::init();
        devs::led1::set();
        
        for(auto& v : Data::mChannels) {
            v = 992;
        }
    }   
    static inline void periodic() {
        tp1::set();
        trace::periodic();
        crsf::periodic();
        sport::periodic();
        sport_uart::periodic();
        sbus::periodic();
        bt_uart::periodic();
        
        // if (LPUART1->ISR & USART_ISR_RXNE_RXFNE) {
        //     uint8_t b = LPUART1->RDR;
        //     IO::outl<trace>("LPUART: ", b);
        // }
        
        tp1::reset();
    }
    static inline void ratePeriodic() {
        tp2::set();        
        
        crsf_pa::copyChangedChannels(Data::mChannels);

        robo_pa::whenTargetChanged([&](const robo_pa::Target t, const robo_pa::index_type::nan_type i){
            IO::outl<trace>("Robo");
            switch(t) {
            case robo_pa::Target::Prop:
                if (i < Data::mChannels.size()) {
                    Data::mChannels[i] = robo_pa::propValues[i];                    
                }
            break;
            case robo_pa::Target::Switch:
                if (i < Data::mAltData.size()) {
                    Data::mAltData[i] = robo_pa::switchValues[i];                    
                }
            break;
            case robo_pa::Target::Toggle:
                if (i < Data::mAltData.size()) {
                    Data::mAltData[i] = robo_pa::toggleValues[i];                    
                }
            break;
            default:
            break;
            }
        });

        
        sbus::set(std::span{&Data::mChannels[0], 16});;
        
        sport::ratePeriodic();
        crsf_out::ratePeriodic();
        crsfTelemetry::ratePeriodic();
        sbus::ratePeriodic();
        
        const auto oldState = mState;
        ++mStateTick;
        switch(mState) {
        case State::Undefined:
            mStateTick.on(initTicks, []{
                mState = State::Init;
                btpwr::reset(); // on
            });
        break;
        case State::Init:
            mStateTick.on(initTicks, []{
                mState = State::Run;
            });
        break;
        case State::Run:
            (++mDebugTick).on(debugTicks, []{
                led1::toggle();
                // IO::outl<trace>(
                //             "b: ", crsf_pa::mBytesCounter
                //             );
                //             " p: ", crsf_pa::mPackagesCounter, 
                //             " l: ", crsf_pa::mLinkPackagesCounter, 
                //             " ch: ", crsf_pa::mChannelsPackagesCounter, 
                //             " pg: ", crsf_pa::mPingPackagesCounter, 
                //             " pe: ", crsf_pa::mParameterEntryPackagesCounter, 
                //             " pr: ", crsf_pa::mParameterReadPackagesCounter, 
                //             " pw: ", crsf_pa::mParameterWritePackagesCounter, 
                //             " c: ", crsf_pa::mCommandPackagesCounter, 
                //             " d: ", crsf_pa::mDataPackagesCounter); 
                IO::outl<trace>(
                            "ch0: ", Data::mChannels[0],
                            " ch1: ", Data::mChannels[1],
                            " ch2: ", Data::mChannels[2],
                            " ch3: ", Data::mChannels[3]
                        );
                IO::outl<trace>(
                            " ro0: ", robo_pa::propValues[0],
                            " ro1: ", robo_pa::propValues[1],
                            " ro2: ", robo_pa::propValues[2],
                            " ro3: ", robo_pa::propValues[3]
                        );
                
            });
        break;
        }
        if (oldState != mState) {
            mStateTick.reset();
        }
        tp2::reset();        
    }
private:
    static inline External::Tick<systemTimer> mStateTick;
    static inline External::Tick<systemTimer> mDebugTick;
    static inline State mState{State::Undefined};
};

using devs = Devices<CC01, Config, Mcu::Stm::Stm32G431>;

int main() {
    using gfsm = GFSM<devs>;
    gfsm::init();

    // NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    // __enable_irq();
    
    while(true) {
        gfsm::periodic();
        devs::systemTimer::periodic([]{
            gfsm::ratePeriodic();
        });
    }
}
void __assert_func (const char *, int, const char *, const char *){
    while(true) {
        devs::tp1::set();
        devs::tp1::reset();
    }
}

extern "C" {

void DMA1_Channel1_IRQHandler() {
    DMA1->IFCR = DMA_IFCR_CTCIF1;
}

}
