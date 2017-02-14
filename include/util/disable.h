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

#include "std/traits.h"

#if __has_include(<avr/interrupt.h>)
# include <avr/interrupt.h>
#else
# define sei()
# define cli()
#endif

struct EnableInterrupt {};
struct DisbaleInterrupt {};

// todo: Restore State (SREG)

template<typename T, bool Active = true>
class Scoped;

template<bool Active>
class Scoped<EnableInterrupt, Active> final
{
public:
    inline Scoped() {
        if constexpr(Active) {
            sei();
        }
    }
    inline ~Scoped() {
        if constexpr(Active) {
            cli();
        }
    }
};

template<bool Active>
class Scoped<DisbaleInterrupt, Active> final
{
public:
    inline Scoped() {
        if constexpr(Active) {
            cli();
        }
    }
    inline ~Scoped() {
        if constexpr(Active) {
            sei();
        }
    }
};

