/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2016, 2017, 2018 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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

#include <cstdint>
#include <std/utility>

namespace AVR {
    namespace Series0 {
        struct Sleep final {
            enum class CtrlA_t: uint8_t {
                enable     = 0x01,
                idle       = (0x00 << 1),
                standby    = (0x01 << 1),
                power_down = (0x02 << 1),
            };
            ControlRegister<Cpu, CtrlA_t> ctrla;
            
            static inline constexpr uintptr_t address = 0x0050;
        };
       static_assert(sizeof(Sleep) == 1);

    }
}
