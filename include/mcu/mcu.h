/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2013, 2014, 2015, 2016 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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

#include <stdint.h>
#include "std/limits.h"
#include "units/physical.h"

#if __has_include(<avr/io.h>)
# include <avr/io.h>
#endif

namespace AVR {
struct A {
    static constexpr char letter = 'A';
};
struct B {
    static constexpr char letter = 'B';
};
struct C {
    static constexpr char letter = 'C';
};
struct D {
    static constexpr char letter = 'D';
};
struct E {
    static constexpr char letter = 'E';
};

struct ATMegaNone;
struct ATMega8;
struct ATMega1284P;
struct ATMega328P;

}

#include "mcu/avr/atmega8.h"
#include "mcu/avr/atmega328p.h"
#include "mcu/avr/atmega328pb.h"
#include "mcu/avr/atmega1284p.h"

namespace AVR {

template<typename Component>
constexpr Component* getBaseAddr() {
    return reinterpret_cast<Component*>(Component::address);
}

template<typename Component, int Number>
constexpr Component* getBaseAddr() {
    return reinterpret_cast<Component*>(Component::template Address<Number>::value);
}

template<typename Component, typename Letter>
constexpr Component* getBaseAddr() {
    return reinterpret_cast<Component*>(Component::template Address<Letter>::value);
}

}

#if defined(__AVR_ATmega1284P__)
typedef AVR::ATMega1284P DefaultMcuType;
#elif defined(__AVR_ATmega328P__)
typedef AVR::ATMega328P DefaultMcuType;
#elif defined(__AVR_ATmega8__)
typedef AVR::ATMega8 DefaultMcuType;
#elif defined(__AVR_ATtiny85__)
typedef AVR::ATTiny85 DefaultMcuType;
#else
typedef AVR::ATMegaNone DefaultMcuType;
#endif


namespace AVR {
namespace Util {

template<typename T>
struct TimerSetupData final {
    const uint16_t prescaler;
    const T ocr;
};

template<typename MCUTimer, typename T>
constexpr TimerSetupData<T> calculate(const std::hertz& ftimer) {
    using pRow = typename MCUTimer::mcu_timer_type::template PrescalerRow<MCUTimer::number>;
    for(const auto& p : pRow::values) {
        const auto tv = (Config::fMcu / ftimer) / p;
        if (tv < std::numerical_limits<T>::max()) {
            return {p, static_cast<T>(tv)};
        }
    }
    return {0, 0};
}

}
}

