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

#include <stdint.h>
#include <stdlib.h>

#if __has_include(<avr/io.h>)
# include <avr/io.h>
#endif
#if __has_include(<avr/avr_mcu_section.h>)
# include <avr/avr_mcu_section.h>
#endif

#ifndef GPIOR0
# define GPIOR0 _SFR_IO8(0x5c)
#endif

AVR_MCU_SIMAVR_CONSOLE(&GPIOR0);

int main() {
    uint8_t x = 42;
    uint8_t base = 10;
    const uint8_t size = 4;    
    
    char data[size];
    itoa(x, data, base);
    
    for(uint8_t i = 0; i < size; ++i) {
        if (data[i] == '\0') {
            break;
        }
        GPIOR0 = data[i];
    }
    GPIOR0 = '\r';
    while(1) {}
    
}
