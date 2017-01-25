/*
 * WMuCpp - Bare Metal C++
 * Copyright (C) 2013, 2014, 2015, 2016, 2016, 2017 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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

#include <stdint.h>
#include <stdbool.h>

volatile uint8_t global = 3;
volatile uint8_t global2 = 0;
volatile bool b = false;

//[pointer
bool foo1(uint8_t* x) {
    if (b) {
        *x = global;
        return true;
    }
    return false;
}//]
//[main
int main()
{
    uint8_t x = 0;
    if (foo1(&x)) {
        global2 = x;
    }

    while(true);
}
//]
