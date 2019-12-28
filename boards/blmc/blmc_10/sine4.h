#pragma once

#include <mcu/avr.h>
#include <mcu/internals/systemclock.h>
#include <mcu/internals/usart.h>
#include <mcu/internals/port.h>
#include <mcu/internals/adc.h>
#include <mcu/internals/adcomparator.h>
#include <mcu/internals/capture.h>
#include <mcu/internals/pwm.h>

#include <external/units/physical.h>
#include <external/units/percent.h>

#include <etl/fixedpoint.h>
#include <etl/output.h>

#include <std/chrono>

#include <math.h>

namespace BLDC {
    using namespace AVR;
    using namespace External::Units;
    using namespace External::Units::literals;
    using namespace std::literals::chrono;
    
    namespace Sine6 {
        
        template<typename RotTimer, typename ComTimer, typename PWM, typename Commuter, typename AC, typename CSensor>
        struct Controller {
            
            using all_channels = typename PWM::all_channels;
            
            enum class State : uint8_t {Off, Sine, ClosedLoopStart, ClosedLoopStart2, ClosedLoop, ClosedLoopError};
            
            inline static constexpr std::array<uint8_t, 6> sinePhaseLength{100, 50, 25, 12, 6, 3};
            
            static inline constexpr uint16_t PwmMax = 200;
            using value_type = etl::typeForValue_t<PwmMax>;
            
//            using scale_type = etl::ScaledInteger<etl::uint_ranged<value_type, 1, 100>, std::ratio<1,100>>;
            using scale_type = etl::FixedPoint<uint8_t, 7>;
            
            static inline constexpr auto gen = []<uint16_t Size, uint16_t Max = 1000>(std::integral_constant<uint16_t, Size>, 
                                                                                      std::integral_constant<uint16_t, Max> = std::integral_constant<uint16_t, Max>{}){
                                                                                using value_type = etl::typeForValue_t<Max>;        
                                                                                std::array<value_type, Size> t;
                                                                                for(uint16_t i = 0; i < t.size(); ++i) {
                                                                                           t[i] = (Max / 2) * (cos((i * 2 * M_PI) / t.size()) + 1.0);
                                                                                }
                                                                                return t;
                                                                        };
        
        
        struct AcHandler : AVR::IsrBaseHandler<AVR::ISR::AdComparator<0>::Edge>{
            static inline void isr() {
                AC::onInterrupt([&](){
                    uint8_t c = 0;
                    for(uint8_t i = 0; i < mMaxDelay; ++i) {
                        if (!(AC::get() ^ AC::get())) {
                            if(++c > mDelay) {
                                break;
                            }
                        }
                        else {
                            c = 0;
                        }
                    }
                    Commuter::next();
                    Commuter::template onState<0>([&] {
                        mActualPeriodLoop = RotTimer::restart(); 
                    });
                });
            };
            inline static constexpr uint8_t mDelay = 4;
            inline static constexpr uint8_t mMaxDelay = 10;
        };
        
        inline static volatile State mState = State::Off;
        
        
        inline static void closedLoop(bool on) {
            if (on) {
                mState = State::ClosedLoopStart;
            }
            else {
                mState = State::Sine;
            }
        }
        
        struct ComTimerHandler : AVR::IsrBaseHandler<typename ComTimer::interrupt_type>{
            static inline void isr() {
                ComTimer::onInterrupt([&]{
                    isrPeriodic();
                });
            }
        };
        
        inline static void init() {
            CSensor::init();
            AC::init();
            AC::template enableInterrupts<false>();
            Commuter::init();
            RotTimer::init();
            ComTimer::init();
            ComTimer::period(mActualPeriod);
            ComTimer::template enableInterrupts<false>();
        }
        
        template<uint16_t Size>
        inline static auto setSine() {
            static constexpr auto sine_table = gen(std::integral_constant<uint16_t, Size>{}, std::integral_constant<uint16_t, PwmMax>{});
            using size_type = decltype(sine_table)::size_type;
            using value_type = decltype(sine_table)::value_type;
            constexpr size_type shift = sine_table.size() / 3;
            
            static etl::uint_ranged_circular<size_type, 0, Size - 1> index{}; 
            
            std::array<value_type, 3> v{sine_table[index], 
                        sine_table[index.template leftShift<shift>()], 
                        sine_table[index.template leftShift<2*shift>()]};
            
            ++index;
            if (index == 0) {
                if (mState == State::ClosedLoopStart) {
                    
                    ComTimer::template enableInterrupts<false>();
             
                    AC::template enableInterrupts<true>();
                    
                    Commuter::set(typename Commuter::state_type{1});
                    
                    PWM::template duty<all_channels >(mActualPwm);
                    
                    mState = State::ClosedLoopStart2;
                }
                else {
                    mOldActualSineTable = mActualSineTable;
                    ComTimer::period(mActualPeriod);
                }
            }
            return v;
        }
        
        inline static void enableSineMode(bool v) {
            if (v) {
                
            }
            else {
                
            }
        }
        
        template<uint8_t Table>
        struct SSS {
            inline constexpr auto operator()() {
                return setSine<6 * sinePhaseLength[Table]>();
            } 
        };
        
        inline static void isrPeriodic() {
//            using value_type = etl::typeForValue_t<PwmMax>;

            auto v = etl::select_t<SSS>(mOldActualSineTable);
            
            if (mState == State::ClosedLoopStart2) {
                mState = State::ClosedLoop;
                return;
            }
        
            PWM::template duty<Meta::List<AVR::PWM::WO<0>>>(v[0] * mScale);
            PWM::template duty<Meta::List<AVR::PWM::WO<1>>>(v[1] * mScale);
            PWM::template duty<Meta::List<AVR::PWM::WO<2>>>(v[2] * mScale);
        }
        
        inline static void periodic() {
        }

        static inline uint16_t npwm = 0;
        static inline int16_t d = 0;
        static inline int16_t delta = 0;
        
        inline static void ratePeriodic() {
            if ((mActualPeriodLoop < 2 * mActualPeriodEstimate) && (mActualPeriodLoop > mActualPeriodEstimate / 2)) {
                mActualPeriodEstimate = (uint32_t{3} * mActualPeriodEstimate + mActualPeriodLoop) / 4;
            }
            
            switch(mState) {
            case State::Sine:
//            {
//                int16_t delta = (mCurrentMax + mCurrentMin) / 2 - CSensor::value();
//                if ((mScale > scale_type{1.0/64}) && (mScale < scale_type{1.0})) {
//                } 
//            }
                
                if (CSensor::value() > mCurrentMax) {
                    if (mScale > scale_type{0.1}) {
                        mScale -= scale_type{1.0/128};
                    }
                }
                else if (CSensor::value() < mCurrentMin) {
                    if (mScale < scale_type{0.9}) {
                        mScale += scale_type{1.0/128};
                    }
                }

                mActualPeriodEstimate = mDesiredPeriod;
                if (mDesiredPeriod < mPeriodS0) {
                    mState = State::ClosedLoopStart;
                }
                break;
            case State::ClosedLoop:
            {
                delta = mActualPeriodEstimate - mDesiredPeriod;
                d = (delta * scale_type{1.0/8});
                npwm = mActualPwm + d;
                PWM::template duty<all_channels>(npwm);
                
//                if (mActualPeriodEstimate > mPeriodS1) {
//                    mState = State::Sine;
//                }
            }
                break;
            default:
                break;
            }
        }
        
        inline static void currentMax(uint16_t v) {
            mCurrentMax = v;
        }
        inline static void currentMin(uint16_t v) {
            mCurrentMin = v;
        }
        
        inline static void speed(etl::uint_ranged<uint16_t, 0, 320> s) {
            constexpr uint16_t sMin = 10;
            constexpr uint32_t periodBase = 10000;
            uint8_t t = 0;
            
            if (s < sMin) {
                off();
                mState = State::Off;
                return;
            }
            else if ((s >= sMin) && (s < (2 * sMin))) {
                mActualPeriod = periodBase - (periodBase * (s - sMin)) / 20 ;
                t = 0;
            }
            else if ((s >= 2* sMin) && (s < (4 * sMin))) {
                mActualPeriod = periodBase - (periodBase * (s - 2*sMin)) / 40 ;
                t = 1;
            }
            else if ((s >= 4* sMin) && (s < (8 * sMin))) {
                mActualPeriod = periodBase - (periodBase * (s - 4*sMin)) / 80 ;
                t = 2;
            }
            else if ((s >= 8* sMin) && (s <= (16 * sMin))) {
                mActualPeriod = periodBase - (periodBase * (s - 8*sMin)) / 160 ;
                t = 3;
            }
            else if ((s >= 16* sMin) && (s <= (32 * sMin))) {
                mActualPeriod = periodBase - (periodBase * (s - 16*sMin)) / 320;
                t = 4;
            }
            
            if ((mState != State::Off) && (mState != State::Sine)) return; 
            
            mDesiredPeriod = (mActualPeriod.toInt() * uint32_t{sinePhaseLength[t]} * 6) / (Project::Config::fMcu / Project::Config::fRtc);
            
            mState = State::Sine;
            
            AC::template enableInterrupts<false>();
            ComTimer::template enableInterrupts<true>();
            
            mActualSineTable = t;

            Commuter::template pwm<0>();
            Commuter::template pwm<1>();
            Commuter::template pwm<2>();
            
            mActualPwm = PwmMax * mScale * mPwmAdjust;
        }
        
        inline static void setSine(uint16_t p) {
            
        }
        
        inline static void off() {
            Commuter::off();
            AC::template enableInterrupts<false>();
        }
        
        
        inline static void pwmSet(int16_t d) {
            mActualPwm += d;
            PWM::template duty<all_channels >(mActualPwm);
        }
        
        inline static void pwmInc() {
            ++mActualPwm;
            PWM::template duty<all_channels >(mActualPwm);
        }
        inline static void pwmDec() {
            --mActualPwm;
            PWM::template duty<all_channels >(mActualPwm);
        }
        
        static inline volatile scale_type mScale{0.4};
        
        inline static uint16_t mActualPwm = 0;
        inline static volatile etl::uint_ranged<uint16_t, 1000, 40000> mActualPeriod{20000};

        inline static volatile uint16_t mActualPeriodLoop{0};
        inline static volatile uint16_t mActualPeriodEstimate{0};
        
        inline static volatile uint16_t mDesiredPeriod = 0;
        
        inline static volatile etl::uint_ranged<uint8_t, 0, sinePhaseLength.size() - 1> mActualSineTable{};
        inline static etl::uint_ranged<uint8_t, 0, sinePhaseLength.size() - 1> mOldActualSineTable{};
        
        inline static uint16_t mCurrentMax = 130;
        inline static uint16_t mCurrentMin = 120;
        
//        inline static uint16_t mPeriodS0 = 2000;
        inline static uint16_t mPeriodS0 = 600;
        inline static uint16_t mPeriodS1 = mPeriodS0 * etl::FixedPoint<uint8_t, 7>{1.2};
        
        inline static scale_type mPwmAdjust{0.85};
    };
}
}
