#pragma once

#include "output.h"
#include "iservo.h"

template<typename Devs>
struct ServoOutputs {
    using devs = Devs;
    using debug = devs::debug;
    using servo1_ws = devs::srv1_waveshare;
    using servo2_ws = devs::srv2_waveshare;
    using servo1_ft = devs::srv1_feetech;
    using servo2_ft = devs::srv2_feetech;

    template<uint8_t N>
    static inline void offset(const uint16_t o) {
        static_assert(N <= 1);
        if (servos[N]) {
            servos[N]->offset(o);
        }
    }
    template<uint8_t N>
    static inline void speed(const uint16_t s) {
        static_assert(N <= 1);
        if (servos[N]) {
            servos[N]->speed(s);
        }
    }
    static inline void update(/*const uint8_t n*/) {
        for(const auto& s : servos) {
            if (s) {
                s->update();
            }
        }
        // if ((n < servos.size()) && servos[n]) {
        //     servos[n]->update();
        // }
    }
    static inline int8_t turns(const uint8_t n) {
        if ((n < servos.size()) && servos[n]) {
            return servos[n]->turns();
        }
        return 0;
    }
    static inline uint16_t actualPos(const uint8_t n) {
        if ((n < servos.size()) && servos[n]) {
            return servos[n]->actualPos();
        }
        return 0;
    }
    template<uint8_t N>
    static inline void servo(const uint8_t s) {
        IO::outl<debug>("# servo", N, ": ", s);
        static_assert(N <= 1);
        using srv_ws_t = std::conditional_t<(N == 0), Servo<servo1_ws>, Servo<servo2_ws>>;
        using srv_ft_t = std::conditional_t<(N == 0), Servo<servo1_ft>, Servo<servo2_ft>>;
        switch(s) {
        case 0: // analog FB
            servos[N] = nullptr;
            servos[N] = std::make_unique<srv_ft_t>();
            break;
        case 1: // PWM feedback
            servos[N] = nullptr;
            servos[N] = std::make_unique<srv_ft_t>();
            break;
        case 2: // serial
            servos[N] = nullptr;
            servos[N] = std::make_unique<srv_ws_t>();
            break;
        case 3: // none
            servos[N] = nullptr;
            break;
        default:
            break;
        }
    }
    static inline void zero() {
        for(const auto& s : servos) {
            if (s) {
                s->zero();
            }
        }
    }
    static inline void periodic() {
        for(const auto& s : servos) {
            if (s) {
                s->periodic();
            }
        }
    }
    static inline void ratePeriodic() {
        for(const auto& s : servos) {
            if (s) {
                s->ratePeriodic();
            }
        }
    }
    private:
    static inline std::array<std::unique_ptr<IServo>, 2> servos{};
};
