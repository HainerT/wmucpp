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

#include "config.h"
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

template<typename MCUTimer>
constexpr TimerSetupData<typename MCUTimer::value_type> calculate(const std::hertz& ftimer) {
//    static_assert(MCUTimer::hasOcrA || MCUTimer::hasOcrB, "need ocra or ocrb");
    using pRow = typename MCUTimer::mcu_timer_type::template PrescalerRow<MCUTimer::number>;
    auto p = std::to_array(pRow::values);
    auto sortedPRow = ::Util::sort(p);
    // todo: use right order: aufsteigend (s.u.)
    for(const auto& p : sortedPRow) {
        const auto tv = (Config::fMcu / ftimer) / p;
        if (tv < std::numeric_limits<typename MCUTimer::value_type>::max()) {
            return {p, static_cast<typename MCUTimer::value_type>(tv)};
        }
    }
    return {};
}


template<typename MCUTimer>
constexpr uint16_t prescalerForAbove(const std::hertz& ftimer) {
    using pRow = typename MCUTimer::mcu_timer_type::template PrescalerRow<MCUTimer::number>;
    auto p = std::to_array(pRow::values);
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
