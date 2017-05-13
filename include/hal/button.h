/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2016, 2017 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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

#pragma once

#include "config.h"
#include "mcu/ports.h"
#include "hal/event.h"

template<uint8_t N>
struct ButtonEvent;

template<>
struct ButtonEvent<0> {
    ButtonEvent() = delete;
    static constexpr EventType event = EventType::ButtonPress0;
};
template<>
struct ButtonEvent<1> {
    ButtonEvent() = delete;
    static constexpr EventType event = EventType::ButtonPress1;
};
template<>
struct ButtonEvent<2> {
    ButtonEvent() = delete;
    static constexpr EventType event = EventType::ButtonPress2;
};
template<>
struct ButtonEvent<3> {
    ButtonEvent() = delete;
    static constexpr EventType event = EventType::ButtonPress3;
};
template<>
struct ButtonEvent<4> {
    ButtonEvent() = delete;
    static constexpr EventType event = EventType::ButtonPress4;
};
template<>
struct ButtonEvent<5> {
    ButtonEvent() = delete;
    static constexpr EventType event = EventType::ButtonPress5;
};
template<>
struct ButtonEvent<6> {
    ButtonEvent() = delete;
    static constexpr EventType event = EventType::ButtonPress6;
};
template<>
struct ButtonEvent<7> {
    ButtonEvent() = delete;
    static constexpr EventType event = EventType::ButtonPress7;
};

template<typename... Buttons>
class ButtonController final {
public:
    ButtonController() = delete;
    static void init() {
        (Buttons::init(), ...); // fold expression
    }
    static void periodic() {
        (Buttons::sample(), ...);
    }
    static void start() {}
    static constexpr auto rateProcess = periodic;
};

template<uint8_t N, typename Pin, bool UseEvent = true, int Thresh = Config::Button::buttonTicksForPressed> // call sample every 1ms -> 50ms Threshold
class Button final {
    static_assert(N < 8, "wrong number of buttons");
    template<typename... Buttons> friend class ButtonController;
public:
    Button() = delete;
    struct Data {
        uint8_t state : 1, count : 7;
    };
    static void init() {
        Pin::template dir<AVR::Input>();
        Pin::pullup();
    }
    inline static Data mData {0, 0};
private:
    static void sample() {
        if (!Pin::isHigh()) { // pressed (active low)
            if (!mData.state) { // not pressed
                if (++mData.count >= Thresh) {
                    mData.state = 1;
                    mData.count = 0;
                    if (UseEvent) {
                        EventManager::enqueue({ButtonEvent<N>::event, std::byte{N}});
                    }
                }
            }
        }
        else { // not pressed
            if (mData.state) {
                mData.state = 0;
                mData.count = 0;
            }
        }
    }
};

