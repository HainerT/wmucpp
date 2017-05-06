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

//#define USE_COUT

#include <stdint.h>

volatile uint8_t r;

class Stream {
};

#ifndef USE_COUT
template<typename S, typename... TT> void out(TT... v) __attribute__((noinline));

template<typename S, typename... TT>
void out(TT... v) {
    ((r=v),...);
}
#else
Stream& operator<<(Stream& o, uint8_t v) __attribute__((noinline));
Stream& operator<<(Stream& o, uint8_t v){
    r = v;
    return o;
}
Stream& operator<<(Stream& o, uint16_t v) __attribute__((noinline));
Stream& operator<<(Stream& o, uint16_t v) {
    r = v;
    return o;
}
Stream& operator<<(Stream& o, uint32_t v) __attribute__((noinline));
Stream& operator<<(Stream& o, uint32_t v) {
    r = v;
    return o;
}
Stream cout;
#endif

int main() {
    uint8_t x = 1;
    uint16_t y = 2;
    uint32_t z = 3;

#ifdef USE_COUT    
    cout << x << y << z;
#else
    out<Stream>(x, y, z);
#endif
    while(true) {
    }
}
