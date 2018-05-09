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
#include <cstddef>

#if __has_include(<avr/pgmspace.h>)
# include <avr/pgmspace.h>
#endif

template<uint8_t Width, uint8_t Height>
class Font;

template<>
class Font<5, 7> {
public:
    static constexpr uint8_t Width = 5;
    static constexpr uint8_t Height = 7;
    class Char {
        friend class Font<5,7>;
    public:
        uint8_t operator[](uint8_t column) const {
            return pgm_read_byte(&data[mOffset + column]);
        }
    private:
        Char(uint16_t offset) : mOffset(offset){}
        const uint16_t mOffset = 0;
    };
    
    Char operator[](uint8_t c) const {
        assert(c >= ' ');
        uint16_t o = (c - ' ') * Width;
        assert(o < sizeof(data));
        return Char{o};
    }
    
private:
    static constexpr uint8_t data[] PROGMEM = {
        0x00, 0x00, 0x00, 0x00, 0x00,// (space)
        0x00, 0x00, 0x5F, 0x00, 0x00,// !
        0x00, 0x07, 0x00, 0x07, 0x00,// "
        0x14, 0x7F, 0x14, 0x7F, 0x14,// #
        0x24, 0x2A, 0x7F, 0x2A, 0x12,// $
        0x23, 0x13, 0x08, 0x64, 0x62,// %
        0x36, 0x49, 0x55, 0x22, 0x50,// &
        0x00, 0x05, 0x03, 0x00, 0x00,// '
        0x00, 0x1C, 0x22, 0x41, 0x00,// (
        0x00, 0x41, 0x22, 0x1C, 0x00,// )
        0x08, 0x2A, 0x1C, 0x2A, 0x08,// *
        0x08, 0x08, 0x3E, 0x08, 0x08,// +
        0x00, 0x50, 0x30, 0x00, 0x00,// ,
        0x08, 0x08, 0x08, 0x08, 0x08,// -
        0x00, 0x60, 0x60, 0x00, 0x00,// .
        0x20, 0x10, 0x08, 0x04, 0x02,// /
        0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
        0x00, 0x42, 0x7F, 0x40, 0x00,// 1
        0x42, 0x61, 0x51, 0x49, 0x46,// 2
        0x21, 0x41, 0x45, 0x4B, 0x31,// 3
        0x18, 0x14, 0x12, 0x7F, 0x10,// 4
        0x27, 0x45, 0x45, 0x45, 0x39,// 5
        0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
        0x01, 0x71, 0x09, 0x05, 0x03,// 7
        0x36, 0x49, 0x49, 0x49, 0x36,// 8
        0x06, 0x49, 0x49, 0x29, 0x1E,// 9
        0x00, 0x36, 0x36, 0x00, 0x00,// :
        0x00, 0x56, 0x36, 0x00, 0x00,// ;
        0x00, 0x08, 0x14, 0x22, 0x41,// <
        0x14, 0x14, 0x14, 0x14, 0x14,// =
        0x41, 0x22, 0x14, 0x08, 0x00,// >
        0x02, 0x01, 0x51, 0x09, 0x06,// ?
        0x32, 0x49, 0x79, 0x41, 0x3E,// @
        0x7E, 0x11, 0x11, 0x11, 0x7E,// A
        0x7F, 0x49, 0x49, 0x49, 0x36,// B
        0x3E, 0x41, 0x41, 0x41, 0x22,// C
        0x7F, 0x41, 0x41, 0x22, 0x1C,// D
        0x7F, 0x49, 0x49, 0x49, 0x41,// E
        0x7F, 0x09, 0x09, 0x01, 0x01,// F
        0x3E, 0x41, 0x41, 0x51, 0x32,// G
        0x7F, 0x08, 0x08, 0x08, 0x7F,// H
        0x00, 0x41, 0x7F, 0x41, 0x00,// I
        0x20, 0x40, 0x41, 0x3F, 0x01,// J
        0x7F, 0x08, 0x14, 0x22, 0x41,// K
        0x7F, 0x40, 0x40, 0x40, 0x40,// L
        0x7F, 0x02, 0x04, 0x02, 0x7F,// M
        0x7F, 0x04, 0x08, 0x10, 0x7F,// N
        0x3E, 0x41, 0x41, 0x41, 0x3E,// O
        0x7F, 0x09, 0x09, 0x09, 0x06,// P
        0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
        0x7F, 0x09, 0x19, 0x29, 0x46,// R
        0x46, 0x49, 0x49, 0x49, 0x31,// S
        0x01, 0x01, 0x7F, 0x01, 0x01,// T
        0x3F, 0x40, 0x40, 0x40, 0x3F,// U
        0x1F, 0x20, 0x40, 0x20, 0x1F,// V
        0x7F, 0x20, 0x18, 0x20, 0x7F,// W
        0x63, 0x14, 0x08, 0x14, 0x63,// X
        0x03, 0x04, 0x78, 0x04, 0x03,// Y
        0x61, 0x51, 0x49, 0x45, 0x43,// Z
        0x00, 0x00, 0x7F, 0x41, 0x41,// [
        0x02, 0x04, 0x08, 0x10, 0x20,// "\"
        0x41, 0x41, 0x7F, 0x00, 0x00,// ]
        0x04, 0x02, 0x01, 0x02, 0x04,// ^
        0x40, 0x40, 0x40, 0x40, 0x40,// _
        0x00, 0x01, 0x02, 0x04, 0x00,// `
        0x20, 0x54, 0x54, 0x54, 0x78,// a
        0x7F, 0x48, 0x44, 0x44, 0x38,// b
        0x38, 0x44, 0x44, 0x44, 0x20,// c
        0x38, 0x44, 0x44, 0x48, 0x7F,// d
        0x38, 0x54, 0x54, 0x54, 0x18,// e
        0x08, 0x7E, 0x09, 0x01, 0x02,// f
        0x08, 0x14, 0x54, 0x54, 0x3C,// g
        0x7F, 0x08, 0x04, 0x04, 0x78,// h
        0x00, 0x44, 0x7D, 0x40, 0x00,// i
        0x20, 0x40, 0x44, 0x3D, 0x00,// j
        0x00, 0x7F, 0x10, 0x28, 0x44,// k
        0x00, 0x41, 0x7F, 0x40, 0x00,// l
        0x7C, 0x04, 0x18, 0x04, 0x78,// m
        0x7C, 0x08, 0x04, 0x04, 0x78,// n
        0x38, 0x44, 0x44, 0x44, 0x38,// o
        0x7C, 0x14, 0x14, 0x14, 0x08,// p
        0x08, 0x14, 0x14, 0x18, 0x7C,// q
        0x7C, 0x08, 0x04, 0x04, 0x08,// r
        0x48, 0x54, 0x54, 0x54, 0x20,// s
        0x04, 0x3F, 0x44, 0x40, 0x20,// t
        0x3C, 0x40, 0x40, 0x20, 0x7C,// u
        0x1C, 0x20, 0x40, 0x20, 0x1C,// v
        0x3C, 0x40, 0x30, 0x40, 0x3C,// w
        0x44, 0x28, 0x10, 0x28, 0x44,// x
        0x0C, 0x50, 0x50, 0x50, 0x3C,// y
        0x44, 0x64, 0x54, 0x4C, 0x44,// z
        0x00, 0x08, 0x36, 0x41, 0x00,// {
        0x00, 0x00, 0x7F, 0x00, 0x00,// |
        0x00, 0x41, 0x36, 0x08, 0x00,// }
        0x08, 0x08, 0x2A, 0x1C, 0x08,// ->
        0x08, 0x1C, 0x2A, 0x08, 0x08 // <-
    };
};

template<>
class Font<6, 8> {
public:
    static constexpr uint8_t Width = 6;
    static constexpr uint8_t Height = 8;
    class Char {
        friend class Font<6,8>;
    public:
        inline static constexpr uint8_t size = Width;
        typedef std::byte value_type;
        std::byte operator[](uint8_t column) const {
            return std::byte{pgm_read_byte(&data[mOffset + column])};
        }
        class Iterator {
        public:
            constexpr Iterator(const Char& cc, uint8_t index = 0) : mChar(cc), mIndex(index) {}
            inline std::byte operator*() {
                return mChar[mIndex];
            }
            inline void operator++() {
                ++mIndex;
            }
            inline bool operator!=(const Iterator& rhs) {
                return mIndex != rhs.mIndex;
            }
        private:
            const Char& mChar;
            uint8_t mIndex = 0;
        };
        constexpr Iterator begin() const {
            return Iterator(*this);
        }
        constexpr Iterator end() const {
            return Iterator(*this, size);
        }
    private:
        Char(uint16_t offset) : mOffset(offset){}
        const uint16_t mOffset = 0;
    };
    
    Char operator[](uint8_t c) const {
        assert(c >= ' ');
        uint16_t o = (c - ' ') * Width;
        assert(o < sizeof(data));
        return Char{o};
    }
    
private:
    static constexpr uint8_t data [] PROGMEM = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sp
        0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, // !
        0x00, 0x00, 0x07, 0x00, 0x07, 0x00, // "
        0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14, // #
        0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12, // $
        0x00, 0x62, 0x64, 0x08, 0x13, 0x23, // %
        0x00, 0x36, 0x49, 0x55, 0x22, 0x50, // &
        0x00, 0x00, 0x05, 0x03, 0x00, 0x00, // '
        0x00, 0x00, 0x1c, 0x22, 0x41, 0x00, // (
        0x00, 0x00, 0x41, 0x22, 0x1c, 0x00, // )
        0x00, 0x14, 0x08, 0x3E, 0x08, 0x14, // *
        0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, // +
        0x00, 0x00, 0x00, 0xA0, 0x60, 0x00, // ,
        0x00, 0x08, 0x08, 0x08, 0x08, 0x08, // -
        0x00, 0x00, 0x60, 0x60, 0x00, 0x00, // .
        0x00, 0x20, 0x10, 0x08, 0x04, 0x02, // /
        0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
        0x00, 0x00, 0x42, 0x7F, 0x40, 0x00, // 1
        0x00, 0x42, 0x61, 0x51, 0x49, 0x46, // 2
        0x00, 0x21, 0x41, 0x45, 0x4B, 0x31, // 3
        0x00, 0x18, 0x14, 0x12, 0x7F, 0x10, // 4
        0x00, 0x27, 0x45, 0x45, 0x45, 0x39, // 5
        0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
        0x00, 0x01, 0x71, 0x09, 0x05, 0x03, // 7
        0x00, 0x36, 0x49, 0x49, 0x49, 0x36, // 8
        0x00, 0x06, 0x49, 0x49, 0x29, 0x1E, // 9
        0x00, 0x00, 0x36, 0x36, 0x00, 0x00, // :
        0x00, 0x00, 0x56, 0x36, 0x00, 0x00, // ;
        0x00, 0x08, 0x14, 0x22, 0x41, 0x00, // <
        0x00, 0x14, 0x14, 0x14, 0x14, 0x14, // =
        0x00, 0x00, 0x41, 0x22, 0x14, 0x08, // >
        0x00, 0x02, 0x01, 0x51, 0x09, 0x06, // ?
        0x00, 0x32, 0x49, 0x59, 0x51, 0x3E, // @
        0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C, // A
        0x00, 0x7F, 0x49, 0x49, 0x49, 0x36, // B
        0x00, 0x3E, 0x41, 0x41, 0x41, 0x22, // C
        0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C, // D
        0x00, 0x7F, 0x49, 0x49, 0x49, 0x41, // E
        0x00, 0x7F, 0x09, 0x09, 0x09, 0x01, // F
        0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A, // G
        0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F, // H
        0x00, 0x00, 0x41, 0x7F, 0x41, 0x00, // I
        0x00, 0x20, 0x40, 0x41, 0x3F, 0x01, // J
        0x00, 0x7F, 0x08, 0x14, 0x22, 0x41, // K
        0x00, 0x7F, 0x40, 0x40, 0x40, 0x40, // L
        0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
        0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F, // N
        0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E, // O
        0x00, 0x7F, 0x09, 0x09, 0x09, 0x06, // P
        0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
        0x00, 0x7F, 0x09, 0x19, 0x29, 0x46, // R
        0x00, 0x46, 0x49, 0x49, 0x49, 0x31, // S
        0x00, 0x01, 0x01, 0x7F, 0x01, 0x01, // T
        0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F, // U
        0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F, // V
        0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F, // W
        0x00, 0x63, 0x14, 0x08, 0x14, 0x63, // X
        0x00, 0x07, 0x08, 0x70, 0x08, 0x07, // Y
        0x00, 0x61, 0x51, 0x49, 0x45, 0x43, // Z
        0x00, 0x00, 0x7F, 0x41, 0x41, 0x00, // [
        0x00, 0x55, 0x2A, 0x55, 0x2A, 0x55, // backslash
        0x00, 0x00, 0x41, 0x41, 0x7F, 0x00, // ]
        0x00, 0x04, 0x02, 0x01, 0x02, 0x04, // ^
        0x00, 0x40, 0x40, 0x40, 0x40, 0x40, // _
        0x00, 0x00, 0x01, 0x02, 0x04, 0x00, // '
        0x00, 0x20, 0x54, 0x54, 0x54, 0x78, // a
        0x00, 0x7F, 0x48, 0x44, 0x44, 0x38, // b
        0x00, 0x38, 0x44, 0x44, 0x44, 0x20, // c
        0x00, 0x38, 0x44, 0x44, 0x48, 0x7F, // d
        0x00, 0x38, 0x54, 0x54, 0x54, 0x18, // e
        0x00, 0x08, 0x7E, 0x09, 0x01, 0x02, // f
        0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C, // g
        0x00, 0x7F, 0x08, 0x04, 0x04, 0x78, // h
        0x00, 0x00, 0x44, 0x7D, 0x40, 0x00, // i
        0x00, 0x40, 0x80, 0x84, 0x7D, 0x00, // j
        0x00, 0x7F, 0x10, 0x28, 0x44, 0x00, // k
        0x00, 0x00, 0x41, 0x7F, 0x40, 0x00, // l
        0x00, 0x7C, 0x04, 0x18, 0x04, 0x78, // m
        0x00, 0x7C, 0x08, 0x04, 0x04, 0x78, // n
        0x00, 0x38, 0x44, 0x44, 0x44, 0x38, // o
        0x00, 0xFC, 0x24, 0x24, 0x24, 0x18, // p
        0x00, 0x18, 0x24, 0x24, 0x18, 0xFC, // q
        0x00, 0x7C, 0x08, 0x04, 0x04, 0x08, // r
        0x00, 0x48, 0x54, 0x54, 0x54, 0x20, // s
        0x00, 0x04, 0x3F, 0x44, 0x40, 0x20, // t
        0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C, // u
        0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C, // v
        0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C, // w
        0x00, 0x44, 0x28, 0x10, 0x28, 0x44, // x
        0x00, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C, // y
        0x00, 0x44, 0x64, 0x54, 0x4C, 0x44, // z
        0x00, 0x00, 0x08, 0x77, 0x41, 0x00, // {
        0x00, 0x00, 0x00, 0x63, 0x00, 0x00, // ¦
        0x00, 0x00, 0x41, 0x77, 0x08, 0x00, // }
        0x00, 0x08, 0x04, 0x08, 0x08, 0x04, // ~
        0x00, 0x3D, 0x40, 0x40, 0x20, 0x7D, // ü
        0x00, 0x3D, 0x40, 0x40, 0x40, 0x3D, // Ü
        0x00, 0x21, 0x54, 0x54, 0x54, 0x79, // ä
        0x00, 0x7D, 0x12, 0x11, 0x12, 0x7D, // Ä
        0x00, 0x39, 0x44, 0x44, 0x44, 0x39, // ö
        0x00, 0x3D, 0x42, 0x42, 0x42, 0x3D, // Ö
        0x00, 0x02, 0x05, 0x02, 0x00, 0x00, // °
        0x00, 0x7E, 0x01, 0x49, 0x55, 0x73, // ß
    };
};
