//#define NDEBUG

#define USE_IBUS

//#define USE_SBUS
//#define USE_SPORT

#define LEARN_DOWN

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

#include <external/hal/adccontroller.h>
#include <external/hal/alarmtimer.h>
#include <external/bluetooth/roboremo.h>
#include <external/bluetooth/qtrobo.h>
#include <external/sbus/sbus.h>
#include <external/sbus/sport.h>
#include <external/ibus/ibus.h>

#include <external/solutions/button.h>
#include <external/solutions/blinker.h>
#include <external/solutions/analogsensor.h>
#include <external/solutions/series01/swuart.h>

#include <std/chrono>

#include <etl/output.h>
#include <etl/meta.h>

using namespace AVR;
using namespace std::literals::chrono;
using namespace External::Units::literals;

namespace IBus::Switch {
    template<typename PA, typename ActorList, typename NVM>
    struct GeneralSwitch;
    
    template<typename PA, typename... Actors, typename NVM>
    struct GeneralSwitch<PA, Meta::List<Actors...>, NVM> {
        using actor_list = Meta::List<Actors...>;
        using channel_t = PA::channel_t;
        using value_t = PA::value_type;
        
        using protocol_t = Protocol1;
        
        using addr_t = Protocol1::addr_t;
        using index_t = Protocol1::index_t;
        using mode_t = Protocol1::mode_t;
        using param_t = Protocol1::param_t;
        using pvalue_t = Protocol1::pvalue_t;
        
        static inline void init(const channel_t c = 0) {
            mChannel = c;
        }

        static inline void channel(const channel_t c) {
            if (c) {
                mChannel = c;
            }
        }

        static inline void address(const addr_t a) {
            mAddr = a;
        }
        
        template<typename T>
        inline static constexpr bool isLearnCode(const T& v) {
            return Protocol1::isLearnCode(v);
        }
        static inline bool ratePeriodic() {
            const auto cv = PA::valueMapped(mChannel);

            if (!cv) return true;
            
            const addr_t addr = Protocol1::toAddress(cv);
            const index_t index = Protocol1::toIndex(cv);
            const mode_t mode = Protocol1::toMode(cv);
                            
            if (Protocol1::isControlMessage(cv)) { // control
                const param_t param = Protocol1::toParameter(cv);
                const pvalue_t value = Protocol1::toParameterValue(cv);
                
//                lpv = value;
//                lpp = param;
                
                if (param == Protocol1::broadCast) {
                    if (value == Protocol1::bCastOff) {
                        lastOnIndex = lastindex_t{};
//                        (Actors::moveToPosition(Protocol1::off), ...);
                    }
                }
                else if (lastOnIndex) {
                    mReceivedControl = false;
                    index_t lastOn{lastOnIndex.toInt()};
                    if (param == Protocol1::reset) {
                        if (value == Protocol1::bCastReset) {
                            Meta::visitAt<actor_list>(lastOn.toInt(), [&]<typename A>(const Meta::Wrapper<A>&) {
                                                          A::reset();
                                                      });
                                          
                        }
                    }
                    else if (param == Protocol1::motorRampTime) {
                        Meta::visitAt<actor_list>(lastOn.toInt(), [&]<typename A>(const Meta::Wrapper<A>&) {
                                                      A::setRamp(value);
                                                  });
                    }
                    else if (param == Protocol1::pwm1) {
                        Meta::visitAt<actor_list>(lastOn.toInt(), [&]<typename A>(const Meta::Wrapper<A>&) {
                                                      A::setPwmForward(value);
                                                  });
                    }
                    else if (param == Protocol1::pwm2) {
                        Meta::visitAt<actor_list>(lastOn.toInt(), [&]<typename A>(const Meta::Wrapper<A>&) {
                                                      A::setPwmBackward(value);
                                                  });
                    }
                    else if (param == Protocol1::offCurr1) {
                        Meta::visitAt<actor_list>(lastOn.toInt(), [&]<typename A>(const Meta::Wrapper<A>&) {
                                                      A::setOffCurrentForward(value);
                                                  });
                    }
                    else if (param == Protocol1::offCurr2) {
                        Meta::visitAt<actor_list>(lastOn.toInt(), [&]<typename A>(const Meta::Wrapper<A>&) {
                                                      A::setOffCurrentBackward(value);
                                                  });
                    }
                    else if (param == Protocol1::passThruChannel) {
                        Meta::visitAt<actor_list>(lastOn.toInt(), [&]<typename A>(const Meta::Wrapper<A>&) {
                                                      A::setPassThru(value);
                                                  });
                    }
                    else if (param == Protocol1::testMode) {
                        if ((value >= 1) && (value <= 8)) {
                            Meta::visit<actor_list>([&]<typename A>(const Meta::Wrapper<A>&) {
//                                                        A::setTest(mode_t(value - 1));
                                                      });
                        }   
                    }
                    else if (param == Protocol1::adcSensitivity) {
                        
                    }
                    else if (param == Protocol1::cutOffSensitivity) {
                        if ((value >= 1) && (value <= 10)) {
                            Meta::visitAt<actor_list>(lastOn.toInt(), [&]<typename A>(const Meta::Wrapper<A>&) {
                                                          A::setStartOffCurrentFactor(value);
                                                      });
                        }
                    }
                }
            }
            else { // command
                mReceivedControl = false;
                if (addr != mAddr) {
                    return false;
                }
//                lmv = mode;
                if (mode != Protocol1::off) {
                    lastOnIndex = index.toInt();
                }
                else {
                    lastOnIndex = lastindex_t{};
                }
                Meta::visitAt<actor_list>(index.toInt(), [&]<typename A>(const Meta::Wrapper<A>&) {
                                              if (mode == Protocol1::off) {
                                                  A::stop();
                                              }
                                              else if (mode == Protocol1::forward) {
                                                  A::startForward();
                                              }
                                              else if (mode == Protocol1::backward) {
                                                  A::startBackward();
                                              }
                                          });
            }
            return true;                
        }
        inline static bool receivedControl() {
            return mReceivedControl;
        }
//        static inline PA::value_type lc0;
//        static inline mode_t lmv;
//        static inline pvalue_t lpv;
//        static inline param_t lpp;
        private: 
        static inline bool mReceivedControl{false};
        using lastindex_t = etl::uint_ranged_NaN<uint8_t, 0, Meta::size_v<actor_list> - 1>; 
        static inline lastindex_t lastOnIndex;
        static inline channel_t mChannel{14}; // ch 16
        static inline addr_t    mAddr{0};
    };
}

namespace Storage {
    inline static constexpr uint8_t NChannels = 4;
    
    template<typename Timer, typename Channel, typename PWM_T>
    struct Output {
        inline static constexpr auto defaultCurrentCheckTimeout = 200_ms;
        
        void currentCheckTO(const auto& t){
            currentCheckTimeout = t;
            rampCyles = ccTimeoutToRampCycles(External::Tick<Timer>{t});
        }
        
        External::Tick<Timer> currentCheckTimeout{defaultCurrentCheckTimeout};
        
        External::Tick<Timer> ccTimeoutToRampCycles(const External::Tick<Timer>& ccTo) {
            return ccTo / (PWM_T::Upper - PWM_T::Lower + 1);
        }
        
        External::Tick<Timer> rampCyles = ccTimeoutToRampCycles(currentCheckTimeout);
//        static_assert(rampCyles.value >= 1);
//        std::integral_constant<uint16_t, rampCyles.value>::_;

        PWM_T pwmForward{PWM_T::Upper / 4};
        PWM_T pwmBackward{PWM_T::Upper / 4};
        
        uint16_t forwardRefC{std::numeric_limits<uint16_t>::max()};
        uint16_t backwardRefC{std::numeric_limits<uint16_t>::max()};

        uint16_t forwardOffC{std::numeric_limits<uint16_t>::max()};
        uint16_t backwardOffC{std::numeric_limits<uint16_t>::max()};

        etl::uint_ranged<uint8_t, 1, 10> startOffCFactor{1};
        
        Channel passThru{};
    };
     
    template<typename Timer, typename Channel, typename AddressR, typename PWM_T>
    struct ApplData final : public EEProm::DataBase<ApplData<Timer, Channel, AddressR, PWM_T>> {
//        Channel::_;
        using Address = etl::uint_ranged_NaN<typename AddressR::value_type, AddressR::Lower, AddressR::Upper>;
//        Address::_;
        using value_type = Output<Timer, Channel, PWM_T>;
        
        enum class AdcSensitivity : uint8_t {Low, Mid, High};

        uint8_t& magic() {
            return mMagic;
        }

        void clear() {
            for(value_type& v : mData) {
                v = value_type{};
            }
            mChannel = 14;
            mAddress = 0;
            mAdcSens = AdcSensitivity::Low;
        }

        Channel& channel() {
            return mChannel;
        }
        Address& address() {
            return mAddress;
        }
        
        using index_type = etl::uint_ranged<uint8_t, 0, NChannels - 1>;
        
        value_type& output(const index_type i) {
            return mData[i];
        }
    private:
        uint8_t mMagic;
        Channel mChannel;
        Address mAddress;
        AdcSensitivity mAdcSens{AdcSensitivity::Low};
        std::array<value_type, NChannels> mData;
    };
}

namespace  {
#ifdef USE_HOTT
    constexpr auto fRtc = 500_Hz;
#endif
#ifdef USE_SBUS
    constexpr auto fRtc = 1000_Hz;
#endif
#ifdef USE_IBUS
    constexpr auto fRtc = 2000_Hz;
#endif
#ifdef USE_PPM
    constexpr auto fRtc = 128_Hz;
#endif
//    constexpr uint16_t Ri = 2700;
    constexpr uint16_t Ri = 3000;
}

#ifdef USE_IBUS
template<typename Sensor>
struct CProvider {
    inline static constexpr auto ibus_type = IBus::Type::type::BAT_CURR;
    inline static void init() {
    }
    inline static uint16_t value() {
        return Sensor::value();
    }
};
#endif
#ifdef USE_SPORT
template<typename Sensor>
struct CProvider {
    inline static constexpr auto valueId = External::SPort::ValueId::Current;
    inline static uint32_t value() {
        return Sensor::value();
    }
};
#endif

template<typename Timer, uint8_t N, typename PWM, typename ADC, 
         typename InA, typename InB, typename EndPin, typename PA, typename NVM, typename Term>
struct ChannelFsm {
    using protocol_t = IBus::Switch::Protocol1;
    using pvalue_t = protocol_t::pvalue_t;

    using adc_index_t = typename ADC::index_type;
    static inline constexpr adc_index_t adci{N};
    
    using pwm_value_t = typename PWM::value_type;
    
    using pa_value_t = typename PA::value_type;
    using channel_t = typename PA::channel_t;
    
    using storage_t = std::remove_cvref_t<decltype(NVM::data())>;
    using data_t = storage_t::value_type;
    using data_index_t = storage_t::index_type;
    
    using endButton = External::Button<EndPin, Timer, External::Tick<Timer>{100_ms}, External::Tick<Timer>{300_ms}>;
    
    inline static constexpr data_index_t data_index{N};
    
    enum class State : uint8_t {Init = 0, 
                                Off = 1, OffWait, 
                                Forward = 10, Backward, ForwardWait, BackwardWait, 
                                SetForwardPwm = 20, SetForwardPwmOff, SetBackwardPwm, SetBackwardPwmOff, SetForwardOC, SetBackwardOC, 
                                OffForwardOC = 30, OffBackwardOC, OffForwardEnd, OffBackwardEnd,
                                ForwardPassThru = 40, BackwardPassThru, ForwardPassThruWait, BackwardPassThruWait};

    enum class Event : uint8_t {None, Stop, 
                                Forward, Backward, 
                                SetFowardPwm, SetBackwardPwm, SetForwardOC, SetBackwardOC,
                                SetStartOffCFactor
                               };
#ifdef USE_IBUS
    struct StateProvider {
        inline static constexpr auto ibus_type = IBus::Type::type::FLIGHT_MODE;
        static inline void init() {}
        static inline uint16_t value() {
#ifdef FULL_STATE
            return 10000 + uint16_t(mValue) + (mOcf ? 1000 : 0) + (mOcb ? 100 : 0);
#else
            return uint16_t(mValue) + 1000;
#endif
        }
        static inline void setOcF(const bool on) {
            mOcf = on;
        }
        static inline void setOcB(const bool on) {
            mOcb = on;
        }
        static inline void setFwEnd(const bool on) {
            mOfe = on;
        }
        static inline void setBwEnd(const bool on) {
            mObe = on;
        }
        static inline void set(const State s) {
            mValue = s;
        }
        static inline bool oc() {
            return mOcf || mOcb;
        }
        static inline bool end() {
            return mOfe || mObe;
        }
    private:
        static inline bool mOfe{false};
        static inline bool mObe{false};
        static inline bool mOcf{false};
        static inline bool mOcb{false};
        static inline State mValue{};
    };
#endif
#ifdef USE_SBUS
    struct StateProvider {
        inline static constexpr auto valueId = External::SPort::ValueId::DIY;
        static inline void init() {}
        static inline uint32_t value() {
#ifdef FULL_STATE
            return 10000 + uint16_t(mValue) + (mOcf ? 1000 : 0) + (mOcb ? 100 : 0);
#else
            return uint16_t(mValue) + 1000;
#endif
        }
        static inline void setOcF(const bool on) {
            mOcf = on;
        }
        static inline void setOcB(const bool on) {
            mOcb = on;
        }
        static inline void setFwEnd(const bool on) {
            mOfe = on;
        }
        static inline void setBwEnd(const bool on) {
            mObe = on;
        }
        static inline void set(const State s) {
            mValue = s;
        }
        static inline bool oc() {
            return mOcf || mOcb;
        }
        static inline bool end() {
            return mOfe || mObe;
        }
    private:
        static inline bool mOcf{false};
        static inline bool mOcb{false};
        static inline bool mOfe{false};
        static inline bool mObe{false};
        static inline State mValue{};
    };
#endif
    static inline constexpr uint16_t pa_mid = (pa_value_t::Upper + pa_value_t::Lower) / 2;
    static inline constexpr uint16_t pa_half = (pa_value_t::Upper - pa_value_t::Lower) / 2;
    static inline constexpr uint16_t pa_hysterese = 10;
    
    static inline constexpr External::Tick<Timer> offTimeout{300_ms};
    static inline constexpr External::Tick<Timer> setPwmTimeout{2000_ms};
    
    static inline auto& pwmForward() {
        return data().pwmForward;        
    }
    static inline auto& pwmBackward() {
        return data().pwmBackward;        
    }
    static inline auto& forwardOffCurr() {
        return data().forwardOffC;        
    }
    static inline auto forwardOffCurr(const uint16_t pwm) {
        if (forwardOffCurr() == std::numeric_limits<uint16_t>::max()) {
            return forwardOffCurr();
        }
        else {
            if (forwardOffCurr() > forwardStartOffCurr()) {
                const uint16_t cv1 = forwardOffCurr() - forwardStartOffCurr();
                const uint16_t cv2 = ((((uint32_t)pwm - pa_mid) * cv1) / pa_half) + forwardStartOffCurr(); 
                return cv2;
            }
            else {
                return forwardOffCurr();
            }
        }
    }
    static inline auto backwardOffCurr(const uint16_t pwm) {
        if (backwardOffCurr() > backwardStartOffCurr()) {
            const uint16_t cv1 = backwardOffCurr() - backwardStartOffCurr();
            const uint16_t cv2 = ((((uint32_t)pa_mid - pwm) * cv1) / pa_half) + backwardStartOffCurr(); 
            return cv2;
        }
        else {
            return backwardOffCurr();
        }
    }
    static inline auto& backwardOffCurr() {
        return data().backwardOffC;        
    }
    static inline auto& forwardRefCurr() {
        return data().forwardRefC;        
    }
    static inline auto& backwardRefCurr() {
        return data().backwardRefC;        
    }
    static inline auto forwardStartOffCurr() {
        return (forwardOffCurr() * data().startOffCFactor) / 10;   
    }
    static inline auto backwardStartOffCurr() {
        return (backwardOffCurr() * data().startOffCFactor) / 10;   
    }
    
    static inline pwm_value_t mRampPwm{};
    static inline Event mEvent{Event::None};
    
    inline static void init() {
        InA::template dir<Output>();
        InB::template dir<Output>();
        off();
        PWM::template on<N>();
        endButton::init();
    }
    
    inline static auto& data() {
        return NVM::data().output(data_index);
    }
    inline static void change() {
        NVM::data().change();
    }
    
    inline static void reset() {
        off();
        data() = data_t{};
        change();
    }

    inline static auto& ccTimeout() {
        return data().currentCheckTimeout;
    }
    
    inline static auto& rampCycles() {
        return data().rampCyles;
    }

    inline static void setRamp(const pvalue_t v) {
        if ((v > 0) && (v <= pvalue_t::Upper)) {
            data().currentCheckTO(data().defaultCurrentCheckTimeout * v.toInt());
        }
        else {
            data().currentCheckTO(data().defaultCurrentCheckTimeout);
        }
        change();
    }
    
    inline static auto& passThru() {
        return data().passThru;
    }
    
    inline static void setPassThru(const auto v) {
        if ((v > 0) && (v <= (channel_t::Upper + 1))) {
            passThru() = v - 1;        
        }           
        else {
            passThru() = channel_t{};
        }
        change();
    }
    
    inline static void setPwmForward(const pvalue_t& v) {
        mEvent = Event::SetFowardPwm;
        mEventValue = v;
    }

    inline static void setPwmBackward(const pvalue_t& v) {
        mEvent = Event::SetBackwardPwm;
        mEventValue = v;
    }

    inline static void setOffCurrentForward(const pvalue_t& v) {
        if (v.isTop()) {
            forwardOffCurr() = std::numeric_limits<uint16_t>::max();
        }
        else {
            forwardOffCurr() = forwardRefCurr()+ ((forwardRefCurr() * v.toInt()) >> 3);
        }
        change();
        mEvent = Event::SetForwardOC;
        mEventValue = v;
    }
    
    inline static void setOffCurrentBackward(const pvalue_t& v) {
        if (v.isTop()) {
            backwardOffCurr() = std::numeric_limits<uint16_t>::max();
        }
        else {
            backwardOffCurr() = backwardRefCurr() + ((backwardRefCurr() * v.toInt()) >> 3);
        }
        change();
        mEvent = Event::SetBackwardOC;
        mEventValue = v;
    }

    inline static void setStartOffCurrentFactor(const pvalue_t& v) {
        data().startOffCFactor.set(v);
        mEvent = Event::SetStartOffCFactor;
    }
    
    inline static void stop() {
        mEvent = Event::Stop;
    }

    inline static void startForward() {
        mEvent = Event::Forward;
    }

    inline static void startBackward() {
        mEvent = Event::Backward;
    }
    
    inline static pvalue_t mEventValue{};
    
    inline static Event event() {
        Event e{Event::None};
        std::swap(e, mEvent);
        return e;
    }
    
    inline static void periodic() {
    }

    inline static void ratePeriodic() {
        const auto oldState = mState;
        ++mStateTick;
        ++mRampTick;
        endButton::periodic();
        const auto e = event();
        const auto buttenEvent = endButton::event();
        switch(mState) {
        case State::Init:
            mState = State::Off;
            break;
        case State::OffWait:
            mStateTick.on(offTimeout, []{
                mState = State::Off;
            });
            break;
        case State::Off:
            if (auto pt = passThru()) {
                if (auto pv = PA::value(pt)) {
                    if (auto rv = pv.toInt(); rv > (pa_mid + pa_hysterese)) {
                        mState = State::ForwardPassThruWait;
                    } 
                    else if (rv < (pa_mid - pa_hysterese)) {
                        mState = State::BackwardPassThruWait;
                    }
                }         
            }
            else {
                if (e == Event::Forward) {
                    mState = State::ForwardWait;
                }
                else if (e == Event::Backward) {
                    mState = State::BackwardWait;
                }
                else if (e == Event::SetFowardPwm) {
                    mState = State::SetForwardPwm;
                }
                else if (e == Event::SetBackwardPwm) {
                    mState = State::SetBackwardPwm;
                }
                else if (e == Event::SetForwardOC) {
                    mState = State::SetForwardOC;
                }
                else if (e == Event::SetBackwardOC) {
                    mState = State::SetBackwardOC;
                }
            }
            break;
        case State::SetForwardPwm:
            if (e != Event::None) {
                if (e == Event::SetFowardPwm) {
                    updateForwardPwm();
                    mStateTick.on(setPwmTimeout, []{
                        mState = State::SetForwardPwmOff;
                    });
                }
                else {
                    mState = State::OffWait;                
                }
            }
            break;
        case State::SetForwardPwmOff:
            if (e != Event::None) {
                if (e == Event::Stop) {
                    mState = State::OffWait;
                } 
                else if (e == Event::Forward) {
                    mState = State::ForwardWait;
                } 
                else if (e == Event::Backward) {
                    mState = State::BackwardWait;
                } 
            }
            break;
        case State::SetBackwardPwm:
            if (e != Event::None) {
                if (e == Event::SetBackwardPwm) {
                    updateBackwardPwm();
                    mStateTick.on(setPwmTimeout, []{
                        mState = State::SetBackwardPwmOff;
                    });
                }
                else {
                    mState = State::OffWait;                
                }
            }
            break;
        case State::SetBackwardPwmOff:
            if (e != Event::None) {
                if (e == Event::Stop) {
                    mState = State::OffWait;
                } 
                else if (e == Event::Forward) {
                    mState = State::ForwardWait;
                } 
                else if (e == Event::Backward) {
                    mState = State::BackwardWait;
                } 
            }
            break;
        case State::SetForwardOC:
            if (e != Event::None) {
                if (e == Event::SetForwardOC) {
                }
                else {
                    mState = State::OffWait;                
                }
            }
            break;
        case State::SetBackwardOC:
            if (e != Event::None) {
                if (e == Event::SetBackwardOC) {
                }
                else {
                    mState = State::OffWait;                
                }
            }
            break;
        case State::ForwardWait:
            mRampTick.on(rampCycles(), []{
                if (++mRampPwm <= pwmForward()) {
                    PWM::template pwm<N>(mRampPwm);
                }
            });
            mStateTick.on(ccTimeout(), []{
               mState = State::Forward; 
            });
            if (e == Event::Stop) {
                mState = State::OffWait;
            }
            break;
        case State::Forward:
            if (passThru()) {
                mState = State::ForwardPassThru;
            }
            lcv = ADC::value(adci, pwmForward());
            lcv2 = ADC::value(adci);
            if (const auto cv = ADC::value(adci); cv >= forwardOffCurr()) {
                mState = State::OffForwardOC;
            }
            if (buttenEvent == endButton::Press::Long) {
                mState = State::OffForwardEnd;
            }
            if (e == Event::Forward) {
            }
            else if (e == Event::SetFowardPwm) {
                mState = State::SetForwardPwm;
            }
            else if (e == Event::SetForwardOC) {
                mState = State::SetForwardOC;
            }
            else if (e != Event::None) {
                mState = State::OffWait;
            }
            break;
        case State::ForwardPassThruWait:
            if (auto pt = passThru()) {
                if (auto pv = PA::value(pt)) {
                    if (auto rv = pv.toInt(); rv < (pa_mid + pa_hysterese)) {
                        mState = State::OffWait;
                    } 
                    else {
                        pwm_value_t pvs(((uint32_t)rv - pa_mid) * pwmForward() / pa_half); 
                        PWM::template pwm<N>(pvs);
                    }
                    mStateTick.on(ccTimeout(), []{
                        mState = State::ForwardPassThru;
                    });
                }         
            }
            else {
                mState = State::OffWait;
            }
            break;
        case State::ForwardPassThru:
            if (auto pt = passThru()) {
                if (auto pv = PA::value(pt)) {
                    if (auto rv = pv.toInt(); rv < (pa_mid + pa_hysterese)) {
                        mState = State::OffWait;
                    } 
                    else {
                        if (buttenEvent == endButton::Press::Long) {
                            mState = State::OffForwardEnd;
                        }
                        else {
//                            pwm_value_t::_;
                            pwm_value_t pvs(((uint32_t)rv - pa_mid) * pwmForward() / pa_half); 
                            lcv = ADC::value(adci, pvs);
                            if (const auto cv = ADC::value(adci, pvs); cv >= forwardOffCurr(rv)) {
                                lcv = cv;
                                lcv2 = forwardOffCurr(rv);
                                mState = State::OffForwardOC;
                            }
                            PWM::template pwm<N>(pvs);
                        }
                    }
                }         
                else {
                    mState = State::OffWait;
                }
            }
            else {
                mState = State::OffWait;
            }
            break;
        case State::OffForwardOC:
            if (auto pt = passThru()) {
                if (auto pv = PA::value(pt)) {
                    if (auto rv = pv.toInt(); (rv > (pa_mid - pa_hysterese)) && (rv < (pa_mid + pa_hysterese))) {
                        mState = State::OffWait;
                    } 
                }
            }
            else {
                if (e == Event::Stop) {
                    mState = State::OffWait;
                }
            }
            break;
        case State::OffForwardEnd:
            if (e == Event::Backward) {
                mState = State::BackwardWait;
            }
            break;
        case State::BackwardWait:
            mRampTick.on(rampCycles(), []{
                if (++mRampPwm <= pwmBackward()) {
                    PWM::template pwm<N>(mRampPwm);
                }
            });
            mStateTick.on(ccTimeout(), []{
               mState = State::Backward; 
            });
            if (e == Event::Stop) {
                mState = State::OffWait;
            }
            break;
        case State::Backward:
            if (const auto cv = ADC::value(adci); cv >= backwardOffCurr()) {
                mState = State::OffBackwardOC;
            }
            if (buttenEvent == endButton::Press::Long) {
                mState = State::OffBackwardEnd;
            }
            if (e == Event::Backward) {
            }
            else if (e == Event::SetBackwardPwm) {
                mState = State::SetBackwardPwm;
            }
            else if (e == Event::SetBackwardOC) {
                mState = State::SetBackwardOC;
            }
            else if (e != Event::None) {
                mState = State::OffWait;
            }
            break;
        case State::BackwardPassThruWait:
            if (auto pt = passThru()) {
                if (auto pv = PA::value(pt)) {
                    if (auto rv = pv.toInt(); rv > (pa_mid - pa_hysterese)) {
                        mState = State::OffWait;
                    } 
                    else {
                        pwm_value_t pvs(((uint32_t)pa_mid - rv) * pwmBackward() / pa_half); 
                        PWM::template pwm<N>(pvs);
                    }
                }         
                mStateTick.on(ccTimeout(), []{
                    mState = State::BackwardPassThru;
                });
            }
            else {
                mState = State::OffWait;
            }
            break;
        case State::BackwardPassThru:
            if (auto pt = passThru()) {
                if (auto pv = PA::value(pt)) {
                    if (auto rv = pv.toInt(); rv > (pa_mid - pa_hysterese)) {
                        mState = State::OffWait;
                    } 
                    else {
                        if (buttenEvent == endButton::Press::Long) {
                            mState = State::OffBackwardEnd;
                        }
                        else {
                            if (const auto cv = ADC::value(adci); cv >= backwardOffCurr(rv)) {
                                mState = State::OffBackwardOC;
                            }
                            pwm_value_t pvs(((uint32_t)pa_mid - rv) * pwmBackward() / pa_half); 
                            PWM::template pwm<N>(pvs);
                        }
                    }
                }         
            }
            else {
                mState = State::OffWait;
            }
            break;
        case State::OffBackwardOC:
            if (auto pt = passThru()) {
                if (auto pv = PA::value(pt)) {
                    if (auto rv = pv.toInt(); (rv > (pa_mid - pa_hysterese)) && (rv < (pa_mid + pa_hysterese))) {
                        mState = State::OffWait;
                    } 
                }
            }
            else {
                if (e == Event::Stop) {
                    mState = State::OffWait;
                }
            }
            break;
        case State::OffBackwardEnd:
            if (e == Event::Forward) {
                mState = State::ForwardWait;
            }
            break;
        }
        if (oldState != mState) {
            StateProvider::set(mState);
            mStateTick.reset();
            mRampTick.reset();
            switch(mState) {
            case State::Init:
                etl::outl<Term>("S i"_pgm);
                break;
            case State::OffWait:
                etl::outl<Term>("S ow"_pgm);
                StateProvider::setOcF(false);
                StateProvider::setOcB(false);
                StateProvider::setFwEnd(false);
                StateProvider::setBwEnd(false);
                off();
                break;
            case State::Off:
                etl::outl<Term>("S o"_pgm);
                off();
                break;
            case State::SetForwardPwm:
                forward();
                updateForwardPwm();
                etl::outl<Term>("S sf"_pgm);
                break;
            case State::SetForwardPwmOff:
                etl::outl<Term>("S sfo"_pgm);
                off();
                break;
            case State::SetBackwardPwm:
                backward();
                updateBackwardPwm();
                etl::outl<Term>("S sb"_pgm);
                break;
            case State::SetBackwardPwmOff:
                etl::outl<Term>("S sbo"_pgm);
                off();
                break;
            case State::SetForwardOC:
                off();
                etl::outl<Term>("S of"_pgm);
                break;
            case State::SetBackwardOC:
                off();
                etl::outl<Term>("S ob"_pgm);
                break;
            case State::ForwardWait:
                etl::outl<Term>("S fw"_pgm);
                StateProvider::setBwEnd(false);
                PWM::template pwm<N>(0);
                mRampPwm.set(0);
                forward();
                break;
            case State::Forward:
                etl::outl<Term>("S f"_pgm);
                forward();
                break;
            case State::ForwardPassThruWait:
                etl::outl<Term>("S ftw"_pgm);
                forward();
                break;
            case State::ForwardPassThru:
                etl::outl<Term>("S ft"_pgm);
                forward();
                break;
            case State::OffForwardOC:
                etl::outl<Term>("S ocf: "_pgm, lcv, ","_pgm, lcv2);
                StateProvider::setOcF(true);
                off();
                break;
            case State::OffForwardEnd:
                etl::outl<Term>("S ofe: "_pgm, lcv);
                StateProvider::setFwEnd(true);
                off();
                break;
            case State::BackwardWait:
                etl::outl<Term>("S bw"_pgm);
                StateProvider::setFwEnd(false);
                PWM::template pwm<N>(0);
                mRampPwm.set(0);
                backward();
                break;
            case State::Backward:
                etl::outl<Term>("S b"_pgm);
                backward();
                break;
            case State::BackwardPassThruWait:
                etl::outl<Term>("S btw"_pgm);
                backward();
                break;
            case State::BackwardPassThru:
                etl::outl<Term>("S bt"_pgm);
                backward();
                break;
            case State::OffBackwardOC:
                etl::outl<Term>("S ocb"_pgm);
                StateProvider::setOcB(true);
                off();
                break;
            case State::OffBackwardEnd:
                etl::outl<Term>("S obe: "_pgm, lcv);
                StateProvider::setBwEnd(true);
                off();
                break;
            }
        }
    }
    
    static inline void debug() {
        if constexpr(N == 0) {
            etl::out<Term>("Ch"_pgm, N, " St: "_pgm, uint8_t(mState), " c: "_pgm, lcv, " c2: "_pgm, lcv2, " mr: "_pgm, mRampPwm);
//            etl::out<Term>(" to:"_pgm, ccTimeout().value, " rc: "_pgm, rampCycles().value);
//            etl::out<Term>(" fpw: "_pgm, pwmForward(), " bpw: "_pgm, pwmBackward());
//            etl::out<Term>(" frc: "_pgm, forwardRefCurr(), " foc: "_pgm, forwardOffCurr(), " sfoc: "_pgm, forwardStartOffCurr());
            etl::outl<Term>(" foc: "_pgm, forwardOffCurr(), " sfoc: "_pgm, forwardStartOffCurr());
//            etl::outl<Term>(" brc: "_pgm, backwardRefCurr(), " boc: "_pgm, backwardOffCurr());
        }
    }
    
private:
    inline static uint16_t lcv{};
    inline static uint16_t lcv2{};
    
    inline static void updateForwardPwm() {
        const auto cv = ADC::value(adci);                
        pwmForward().set(((uint16_t)pwm_value_t::Upper * (mEventValue - pvalue_t::Lower)) / (pvalue_t::Upper - pvalue_t::Lower));
        PWM::template pwm<N>(pwmForward());
        forwardOffCurr() = defaultOC(cv);
        forwardRefCurr() = cv;
        change();
    }
    inline static void updateBackwardPwm() {
        const auto cv = ADC::value(adci);    
        pwmBackward().set(((uint16_t)pwm_value_t::Upper * (mEventValue - pvalue_t::Lower)) / (pvalue_t::Upper - pvalue_t::Lower));
        PWM::template pwm<N>(pwmBackward());
        backwardOffCurr() = defaultOC(cv);
        backwardRefCurr() = cv;
        change();
    }
    
    inline static uint16_t defaultOC(const uint16_t& i) {
        return (3 * i) / 2;
    }
    
    inline static pwm_value_t v0;
    inline static void forward() {
        InA::on();
        InB::off();
    }
    inline static void backward() {
        InA::off();
        InB::on();
    }
    inline static void off() {
        InA::off();
        InB::off();
    }
    inline static External::Tick<Timer> mRampTick;
    inline static External::Tick<Timer> mStateTick;
    inline static State mState{State::Init};
};

template<typename Timer, typename PWM, typename NVM, typename ChList, typename Led, typename Adc, 
         typename Servo, typename Baud, 
         typename Sensor, 
         typename SW, 
         typename Term = void>
struct GlobalFsm;

template<typename Timer, typename PWM, typename NVM, typename Led, typename... Chs, typename Adc, 
         typename Servo, auto baud, 
         typename Sensor, 
         typename SW, 
         typename Term>
struct GlobalFsm<Timer, PWM, NVM, Meta::List<Chs...>, Led, Adc, Servo, BaudRate<baud>, Sensor, SW, Term> {
    using channel_list = Meta::List<Chs...>;
    static_assert(Meta::size_v<channel_list> <= 4, "too much channels");
    static_assert(Meta::is_set_v<channel_list>, "channels must be different");

    using adi_t = Adc::index_type;

    using protocoll_adapter_t = Servo::protocoll_adapter_type;
    
    using ch_t = protocoll_adapter_t::channel_t;
    
    enum class State : uint8_t {Undefined, StartWait, SearchChannel, AfterSearch, InitRun, Run, 
                                ShowAddress, ShowAddressWait, LearnTimeout,
                                Debug1};

    static constexpr External::Tick<Timer> learnTimeoutTicks{4000_ms};
    static constexpr External::Tick<Timer> scanTimeoutTicks{50_ms};
    static constexpr External::Tick<Timer> waitTimeoutTicks{3000_ms};
    static constexpr External::Tick<Timer> signalTimeoutTicks{500_ms};
    static constexpr External::Tick<Timer> eepromTimeout{2000_ms};
    static constexpr External::Tick<Timer> debugTimeout{500_ms};
    
    using blinker = External::SimpleBlinker<Led, Timer, 300_ms>;
    
    inline static void init() {
        NVM::init();
        if (NVM::data().magic() != 42) {
            NVM::data().magic() = 42;
            NVM::data().clear();
            NVM::data().change();
        }
        blinker::init();
#ifdef USE_IBUS
        Servo::template init<BaudRate<baud>>();
#endif
#ifdef USE_SBUS
        Servo::template init<AVR::BaudRate<baud>, FullDuplex, true, 1>(); // 8E2
#endif
        Sensor::init();
        Adc::mcu_adc_type::nsamples(6); 
        Adc::init();
        PWM::init();
        PWM::template prescale<6>();
        (Chs::init(), ...);
    }
    inline static void periodic() {
        NVM::saveIfNeeded([&]{
            etl::outl<Term>("ep s"_pgm);
        });
        Adc::periodic();
        Servo::periodic();
        Sensor::periodic();
        (Chs::periodic(), ...);
    }
    inline static void ratePeriodic() {
        const auto oldState = mState;
        ++mStateTick;
        ++mEepromTick;
        Sensor::ratePeriodic();
        SW::ratePeriodic();
        (Chs::ratePeriodic(), ...);
        blinker::ratePeriodic();
        switch(mState) {
        case State::Undefined:
            mState = State::StartWait;
            blinker::steady();
            break;
        case State::StartWait:
            mStateTick.on(waitTimeoutTicks, []{
                blinker::off();
                mState = State::SearchChannel;
            });
            break;
        case State::SearchChannel:
            if (search()) {
                mState = State::AfterSearch;
            }
            mStateTick.on(learnTimeoutTicks, []{
                mState = State::LearnTimeout;
            });
            break;
        case State::AfterSearch:
            mStateTick.on(signalTimeoutTicks, []{
                mState = State::ShowAddress;
            });
            break;
        case State::LearnTimeout:
            etl::outl<Term>("timeout ch: "_pgm, NVM::data().channel().toInt(), " adr: "_pgm, NVM::data().address().toInt());
            if (NVM::data().channel() && NVM::data().address()) {
                SW::channel(NVM::data().channel());
                SW::address(typename SW::addr_t{NVM::data().address().toInt()});
            }
            mState = State::InitRun;
            break;
        case State::ShowAddress:
            etl::outl<Term>("learned ch: "_pgm, NVM::data().channel().toInt(), " adr: "_pgm, NVM::data().address().toInt());
            blinker::blink(NVM::data().address().toInt() + 1);
            mState = State::ShowAddressWait;
            break;
        case State::ShowAddressWait:
            if (!blinker::isActive()) {
                mState = State::InitRun;
            }
            break;
        case State::InitRun:
            mState = State::Run;
            break;
        case State::Run:
            if ((Chs::StateProvider::oc() || ...)) {
                blinker::steady();
            }
            else if ((Chs::StateProvider::end() || ...)) {
                blinker::blink(1);
            }
            else {
                blinker::off();
            }
            mEepromTick.on(eepromTimeout, []{
//                etl::outl<Term>("exp"_pgm);
                NVM::data().expire();
            });
            mStateTick.match(debugTimeout, []{
                mState = State::Debug1;
            });
            break;
        case State::Debug1:
            mState = State::Run;
            break;
        }
        if (oldState != mState) {
            mStateTick.reset();
            switch(mState) {
            case State::Undefined:
                break;
            case State::StartWait:
                etl::outl<Term>("rcQ test11"_pgm);
                break;
            case State::SearchChannel:
            case State::AfterSearch:
            case State::ShowAddress:
            case State::ShowAddressWait:
            case State::LearnTimeout:
            case State::InitRun:
                break;
            case State::Run:
                break;
            case State::Debug1:
                etl::circular_call(Chs::debug...);
                break;
            }
        }
    }
private:
    using protocol_t = typename SW::protocol_t;
    using addr_t = typename protocol_t::addr_t;
    
    static inline bool search() {
        if (learnChannel < 16) {
            if (const auto lc = protocoll_adapter_t::valueMapped(learnChannel.toRangedNaN()); lc && SW::isLearnCode(lc)) {
                etl::outl<Term>("ch: "_pgm, learnChannel.toInt(), " : "_pgm, lc.toInt());
                if (const auto pv = protocol_t::toParameterValue(lc).toInt(); (pv >= 1) && ((pv - 1) <= SW::protocol_t::addr_t::Upper)) {
                    const uint8_t addr = pv - 1;
                    SW::channel(learnChannel.toRangedNaN());
                    SW::address(addr_t(addr));
                    NVM::data().channel() = learnChannel;
                    NVM::data().address() = addr;
                    NVM::data().change();
                    return true;
                }
            }   
        }
#ifdef LEARN_DOWN
        --learnChannel;
#else
        ++learnChannel;
#endif
        return false;
    }
#ifdef LEARN_DOWN
    static inline etl::uint_ranged_circular<uint8_t, ch_t::Lower, ch_t::Upper> learnChannel{ch_t::Upper};
#else
    static inline etl::uint_ranged_circular<uint8_t, ch_t::Lower, ch_t::Upper> learnChannel{0};
#endif
    static inline External::Tick<Timer> mStateTick;
    static inline External::Tick<Timer> mEepromTick;
    static inline State mState{State::Undefined};
};

using ccp = Cpu::Ccp<>;
using clock = Clock<>;

using usart1Position = Portmux::Position<Component::Usart<1>, Portmux::Default>; // Servo / DBG
#ifdef USE_IBUS
using usart2Position = Portmux::Position<Component::Usart<2>, Portmux::Default>; // Sensor
#endif

using tcaPosition = Portmux::Position<Component::Tca<0>, Portmux::Default>;

using pwm = PWM::DynamicPwm8Bit<tcaPosition>;
#ifdef USE_IBUS
using portmux = Portmux::StaticMapper<Meta::List<usart1Position, usart2Position>>;
#endif
#ifdef USE_SBUS
using portmux = Portmux::StaticMapper<Meta::List<usart1Position>>;
#endif

using systemTimer = SystemTimer<Component::Rtc<0>, fRtc>;
using alarmTimer = External::Hal::AlarmTimer<systemTimer>;


#ifdef USE_SBUS
using servo_pa = External::SBus::Servo::ProtocollAdapter<0, systemTimer>;
#endif
#ifdef USE_IBUS
using servo_pa = IBus::Servo::ProtocollAdapter<0>;
#endif


using servo = Usart<usart1Position, servo_pa, AVR::UseInterrupts<false>, 
                    AVR::ReceiveQueueLength<0>, AVR::SendQueueLength<256>>;

#ifndef NDEBUG
using terminal = etl::basic_ostream<servo>;
#else
using terminal = etl::basic_ostream<void>;
#endif

using ledPin  = Pin<Port<D>, 2>; 

using pwmPin1 = Pin<Port<A>, 0>; 
using pwmPin2 = Pin<Port<A>, 1>; 
using pwmPin3 = Pin<Port<A>, 2>; 
using pwmPin4 = Pin<Port<A>, 3>; 

using end1Pin = Pin<Port<F>, 5>; 
using end2Pin = Pin<Port<D>, 1>; 
using end3Pin = Pin<Port<A>, 4>; 
using end4Pin = Pin<Port<A>, 5>; 

using end1 = ActiveLow<end1Pin, Input>;
using end2 = ActiveLow<end2Pin, Input>;
using end3 = ActiveLow<end3Pin, Input>;
using end4 = ActiveLow<end4Pin, Input>;

using inA1Pin = Pin<Port<F>, 3>; 
using inB1Pin = Pin<Port<F>, 1>; 
using inA2Pin = Pin<Port<D>, 3>; 
using inB2Pin = Pin<Port<D>, 6>; 
using inA3Pin = Pin<Port<C>, 3>; 
using inB3Pin = Pin<Port<C>, 2>; 
using inA4Pin = Pin<Port<A>, 6>; 
using inB4Pin = Pin<Port<A>, 7>; 

using dgbPin = Pin<Port<C>, 0>; // tx 
using ibusPin = Pin<Port<C>, 1>; // rx 

using csf1Pin = Pin<Port<F>, 2>; // ADC 12
using csf2Pin = Pin<Port<D>, 7>; // ADC 7
using csf3Pin = Pin<Port<D>, 5>; // ADC 5
using csf4Pin = Pin<Port<D>, 0>; // ADC 0 

#ifdef USE_SPORT
using rxtxPin = Pin<Port<F>, 0>; // rxtx 
#else
using sensorUartPin = Pin<Port<F>, 0>; // tx 
#endif

using daisyChain= Pin<Port<F>, 4>; 

#ifdef USE_DAISY
struct IBusThrough {
    inline static void init() {
        daisyChain::template dir<Output>();
    }
    inline static void on() {
        daisyChain::on();
    }
    inline static void off() {
        daisyChain::off();
    }
};
using ibt = IBusThrough;
#else
using ibt = void;
#endif

using eeprom = EEProm::Controller<Storage::ApplData<systemTimer, servo_pa::channel_t, IBus::Switch::Protocol1::addr_t, pwm::value_type>>;

//using adc = Adc<Component::Adc<0>, AVR::Resolution<10>, Vref::V2_5>;
using adc = Adc<Component::Adc<0>, AVR::Resolution<10>, Vref::V4_3>;
using adcController = External::Hal::AdcController<adc, Meta::NList<12, 7, 5, 0, 0x1e>>; // 1e = temp

//using vn1 = External::AnalogSensor<adcController, 0, std::ratio<0,1>, std::ratio<Ri,1900>, std::ratio<100,1>>;
using vn1 = External::AnalogSensor<adcController, 0, std::ratio<0,1>, std::ratio<Ri,1900>, std::ratio<100,1>>;
using vn2 = External::AnalogSensor<adcController, 1, std::ratio<0,1>, std::ratio<Ri,1900>, std::ratio<100,1>>;
using vn3 = External::AnalogSensor<adcController, 2, std::ratio<0,1>, std::ratio<Ri,1900>, std::ratio<100,1>>;
using vn4 = External::AnalogSensor<adcController, 3, std::ratio<0,1>, std::ratio<Ri,1900>, std::ratio<100,1>>;

using cp1 = CProvider<vn1>;
using cp2 = CProvider<vn2>;
using cp3 = CProvider<vn3>;
using cp4 = CProvider<vn4>;

using ch0 = ChannelFsm<systemTimer, 0, pwm, adcController, inA1Pin, inB1Pin, end1, servo_pa, eeprom, terminal>;
using ch1 = ChannelFsm<systemTimer, 1, pwm, adcController, inA2Pin, inB2Pin, end2, servo_pa, eeprom, terminal>;
using ch2 = ChannelFsm<systemTimer, 2, pwm, adcController, inA3Pin, inB3Pin, end3, servo_pa, eeprom, terminal>;
using ch3 = ChannelFsm<systemTimer, 3, pwm, adcController, inA4Pin, inB4Pin, end4, servo_pa, eeprom, terminal>;

using sp1 = ch0::StateProvider;
using sp2 = ch1::StateProvider;
using sp3 = ch2::StateProvider;
using sp4 = ch3::StateProvider;

#ifdef USE_SPORT
template<typename PA>
using sensorUsart = External::SoftSerial::Usart<Meta::List<rxtxPin, rxtxPin>, Component::Tcb<0>, 
PA, AVR::BaudRate<57600>,
AVR::ReceiveQueueLength<0>,
AVR::SendQueueLength<64>,
etl::NamedFlag<true>,
etl::NamedFlag<false>
//                                            ,dbg1
>;
using sensor = External::SPort::Sensor<External::SPort::SensorId::ID3, sensorUsart, systemTimer, 
                                       Meta::List<cp1, cp2, cp3, cp4, sp1, sp2, sp3, sp4>>;

#else
using sensor = IBus::Sensor<usart2Position, AVR::Usart, AVR::BaudRate<115200>, 
                          Meta::List<cp1, cp2, cp3, cp4, sp1, sp2, sp3, sp4>, systemTimer, ibt
//                          , etl::NamedFlag<true>
//                           , etl::NamedFlag<true>
                          >;
#endif

using gswitch = IBus::Switch::GeneralSwitch<servo_pa, Meta::List<ch0, ch1, ch2, ch3>, eeprom>;

using led = AVR::ActiveHigh<ledPin, Output>;
#ifdef USE_IBUS
using gfsm = GlobalFsm<systemTimer, pwm, eeprom, Meta::List<ch0, ch1, ch2, ch3>, 
                        led, adcController, servo, BaudRate<115200>, sensor, gswitch, terminal>;
#endif
#ifdef USE_SBUS
using gfsm = GlobalFsm<systemTimer, pwm, eeprom, Meta::List<ch0, ch1, ch2, ch3>, 
                        led, adcController, servo, BaudRate<100000>, sensor, gswitch, terminal>;
#endif
#ifdef USE_SPORT
using isrRegistrar = IsrRegistrar<sensor::uart::StartBitHandler, sensor::uart::BitHandler>;
#endif

int main() {
    portmux::init();
    ccp::unlock([]{
        clock::prescale<1>();
    });
    systemTimer::init();
    gfsm::init();
    
    {
#ifdef USE_SPORT
        etl::Scoped<etl::EnableInterrupt<>> ei;
#endif
        while(true) {
            gfsm::periodic();
            systemTimer::periodic([&]{
                gfsm::ratePeriodic();
                alarmTimer::periodic([&](const auto&){
                });
            });
        }
    }
}

#ifdef USE_SPORT
ISR(PORTF_PORT_vect) {
    isrRegistrar::isr<AVR::ISR::Port<rxtxPin::name_type>>();
}

ISR(TCB0_INT_vect) {
    isrRegistrar::isr<AVR::ISR::Tcb<0>::Capture>();
}
#endif

#ifndef NDEBUG
[[noreturn]] inline void assertOutput(const AVR::Pgm::StringView& expr [[maybe_unused]], const AVR::Pgm::StringView& file[[maybe_unused]], unsigned int line [[maybe_unused]]) noexcept {
    etl::outl<terminal>("Assertion failed: "_pgm, expr, etl::Char{','}, file, etl::Char{','}, line);
    while(true) {
        led::activate();
    }
}

template<typename String1, typename String2>
[[noreturn]] inline void assertFunction(const String1& s1, const String2& s2, unsigned int l) {
    assertOutput(s1, s2, l);
}
#endif
