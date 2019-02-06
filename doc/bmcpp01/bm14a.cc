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

#include <cstdint>
#include <cstddef> 
#include <cassert> 
#include <std/utility>
#include <avr/pgmspace.h>

template<auto Size>
struct PgmArray {
    template<typename> struct Generator;
    using mapper = Generator<std::make_index_sequence<Size>>;
    inline static char value(size_t index) {
        assert(index < Size);
        return pgm_read_byte(&mapper::data[index]);
    }
    template<auto... Index>
    struct Generator<std::index_sequence<Index...>> {
        inline static constexpr char data[Size] PROGMEM = {[](auto v){return v * 2;}(Index)... }; 
    };
};

using a1 = PgmArray<10> ;

int main() {
    return a1::value(2);
}
