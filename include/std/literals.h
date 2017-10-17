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

#include "units/duration.h"
#include "units/physical.h"
#include "std/ratio.h"

namespace std {
    namespace literals {
        namespace chrono {
            
            constexpr std::milliseconds operator"" _ms(unsigned long long v) {
                return std::milliseconds{static_cast<uint16_t>(v)};
            }
            
            constexpr std::microseconds operator"" _us(unsigned long long v) {
                return std::microseconds{static_cast<uint16_t>(v)};
            }
            
            constexpr std::seconds operator"" _s(unsigned long long v) {
                return std::seconds{static_cast<uint16_t>(v)};
            }
            
        }
        
        namespace physical {
            
            constexpr std::hertz operator"" _Hz(unsigned long long v) {
                return std::hertz{static_cast<uint32_t>(v)};
            }
            
            constexpr std::megahertz operator"" _MHz(unsigned long long v) {
                return std::megahertz{static_cast<uint8_t>(v)};
            }
            
        }
        
    }
}
