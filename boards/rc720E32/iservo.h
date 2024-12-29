#pragma once

struct IDevice {
    virtual void periodic() = 0;
    virtual void ratePeriodic() = 0;
    virtual ~IDevice() {}
};
struct IServo : IDevice {
    virtual std::pair<uint8_t, uint8_t> fwVersion() = 0;
    virtual std::pair<uint8_t, uint8_t> hwVersion() = 0;
    virtual int8_t turns() = 0;
    virtual uint16_t actualPos() = 0;
    virtual void speed(uint16_t) = 0;
    virtual void offset(uint16_t) = 0;
    virtual void zero() = 0;
    virtual void update() = 0;
    virtual ~IServo(){}
};
struct IEsc : IDevice {
    virtual std::pair<uint8_t, uint8_t> fwVersion() = 0;
    virtual std::pair<uint8_t, uint8_t> hwVersion() = 0;
    virtual uint16_t current() = 0;
    virtual uint16_t rpm() = 0;
    virtual void set(uint16_t) = 0;
    virtual void update() = 0;
    virtual ~IEsc(){}
};
struct IRelay : IDevice {
#ifdef USE_EXTRA_FORWARDS
    virtual void ping() = 0;
    virtual void forwardPacket(const std::byte type, const std::array<uint8_t, 64>& data, const uint16_t length) = 0;
    virtual void command(const std::array<uint8_t, 64>& data, const uint16_t length) = 0;
#else
    virtual void forwardPacket(const volatile uint8_t* data, const uint16_t length) = 0;
#endif
    virtual void setChannel(const uint8_t ch, const uint16_t v) = 0;
    virtual uint16_t value(const uint8_t ch) = 0;
    virtual void update() = 0;
    virtual ~IRelay(){}
};

template<typename T>
concept hasActivateSBus2 = requires(T) {
    T::activateSBus2(true);
};
template<typename T>
concept hasForward = requires(T) {
    T::forwardPacket(nullptr, 0);
};
template<typename T>
concept hasPing = requires(T) {
    T::ping();
};
template<typename T>
concept hasSet = requires(T) {
    T::set(0, 0);
};
template<typename T>
concept hasValue = requires(T) {
    T::value(0);
};
template<typename T>
concept hasPositive = requires(T) {
    T::positive(true);
};
// template<typename T>
// concept hasForwardPacket = requires(T) {
//     T::ping();
// };

template<typename R>
struct Relay : IRelay {
    Relay() {
        R::init();
    }
    ~Relay() {
        R::reset();
    }
    virtual void positive(const bool pos, const bool per = true) {
        if constexpr(hasPositive<R>) {
            R::positive(pos, per);
        }
    }
    virtual void activateSBus2(const bool b) {
        if constexpr(hasActivateSBus2<R>) {
            R::activateSBus2(b);
        }
    }
#ifdef USE_EXTRA_FORWARDS
    virtual void forwardPacket(const std::byte type, const std::array<uint8_t, 64>& data, const uint16_t length) {
        if constexpr(hasPing<R>) {
            R::forwardPacket(type, data, length);
        }
    }
    virtual void ping() {
        if constexpr(hasPing<R>) {
            R::ping();
        }
    }
    virtual void command(const std::array<uint8_t, 64>& data, const uint16_t length) {
        if constexpr(hasPing<R>) {
            R::command(data, length);
        }
    }
#else
    virtual void forwardPacket(const volatile uint8_t* data, const uint16_t length) {
        if constexpr(hasForward<R>) {
            R::forwardPacket(data, length);
        }
    }
#endif
    virtual void setChannel(const uint8_t ch, const uint16_t v) {
        if constexpr(hasSet<R>) {
            R::set(ch, v);
        }
    }
    virtual uint16_t value(const uint8_t ch) {
        if constexpr(hasValue<R>) {
            return R::value(ch);
        }
        else {
            return 992;
        }
    }
    virtual void update() {
        R::update();
    }
    virtual void periodic() {
        R::periodic();
    }
    virtual void ratePeriodic() {
        R::ratePeriodic();
    }
};

template<typename S>
struct Servo : IServo {
    Servo() {
        S::init();
    }
    ~Servo() {
        S::reset();
    }
    virtual std::pair<uint8_t, uint8_t> fwVersion() {
        return S::fwVersion();
    }
    virtual std::pair<uint8_t, uint8_t> hwVersion() {
        return S::hwVersion();
    }
    virtual int8_t turns() {
        return S::turns();
    }
    virtual uint16_t actualPos() {
        return S::actualPos();
    }
    virtual void speed( const uint16_t s) {
        S::speed(s);
    }
    virtual void offset( const uint16_t o) {
        S::offset(o);
    }
    virtual void zero() {
        S::zero();
    }
    virtual void update() {
        S::update();
    }
    virtual void periodic() {
        S::periodic();
    }
    virtual void ratePeriodic() {
        S::ratePeriodic();
    }
};

template<typename E>
struct Esc: IEsc {
    Esc() {
        E::init();
    }
    ~Esc() {
        E::reset();
    }
    virtual std::pair<uint8_t, uint8_t> fwVersion() {
        return E::fwVersion();
    }
    virtual std::pair<uint8_t, uint8_t> hwVersion() {
        return E::hwVersion();
    }
    virtual uint16_t current() {
        return E::current();
    }
    virtual uint16_t rpm() {
        return E::rpm();
    }
    virtual void set(const uint16_t s) {
        E::set(s);
    }
    virtual void update() {
        E::update();
    }
    virtual void periodic() {
        E::periodic();
    }
    virtual void ratePeriodic() {
        E::ratePeriodic();
    }
};

