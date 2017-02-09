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
#include "avr8defs.h"
#include "std/limits.h"
#include "units/physical.h"
#include "util/algorithm.h"

namespace AVR {
namespace Util {

template<typename T>
struct TimerSetupData final {
    const uint16_t prescaler = 0;
    const T ocr = 0;
    explicit constexpr operator bool() const {
        return (prescaler > 0) && (ocr > 0);
    }
};

template<typename T, uint8_t N>
constexpr std::array<typename AVR::PrescalerPair<T>::scale_type, N> prescalerValues(const std::array<AVR::PrescalerPair<T>, N>& a) {
    std::array<typename AVR::PrescalerPair<T>::scale_type, N> values;
    for(uint8_t i = 0; i < N; ++i) {
        values[i] = a[i].scale;
    }
    return values;
}

template<uint16_t Prescale, typename T, uint8_t N>
constexpr typename AVR::PrescalerPair<T>::bits_type bitsFrom(const std::array<AVR::PrescalerPair<T>, N>& a) {
    for(const auto pair: a) {
        if (pair.scale == Prescale) {
            return pair.bits;
        }
    }
    return static_cast<typename AVR::PrescalerPair<T>::bits_type>(0);
}

template<typename T, uint8_t N>
constexpr uint16_t bitsToPrescale(T bits, const std::array<AVR::PrescalerPair<T>, N>& a) {
    for(const auto& pair : a) {
        if (bits == pair.bits) {
            return pair.scale;
        }
    }
    return 0;
}

template<typename MCUTimer>
constexpr TimerSetupData<typename MCUTimer::value_type> calculate(const std::hertz& ftimer) {
//    static_assert(MCUTimer::hasOcrA || MCUTimer::hasOcrB, "need ocra or ocrb");

    using pBits = typename MCUTimer::mcu_timer_type::template PrescalerBits<MCUTimer::number>;
    auto p = prescalerValues(pBits::values);
    
    auto sortedPRow = ::Util::sort(p); // aufsteigend
    for(const auto& p : sortedPRow) {
        if (p > 0) {
            const auto tv = (Config::fMcu / ftimer) / p;
            if (tv < std::numeric_limits<typename MCUTimer::value_type>::max()) {
                return {p, static_cast<typename MCUTimer::value_type>(tv)};
            }
        }
    }
    return {};
}


template<typename MCUTimer>
constexpr uint16_t prescalerForAbove(const std::hertz& ftimer) {
    using pBits = typename MCUTimer::mcu_timer_type::template PrescalerBits<MCUTimer::number>;
    auto p = prescalerValues(pBits::values);
    auto sortedPRow = ::Util::sort(p, std::greater<uint16_t>());
    for(const auto& p : sortedPRow) {
        auto f = Config::fMcu / (uint32_t)p;
        if (f >= ftimer) {
            return p;
        }
    }
    return 0;
}

}
}
