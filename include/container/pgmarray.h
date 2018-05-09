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

#include <cstddef>
#include <array>
#include <type_traits>
#include <memory>
#include "util/meta.h"

#if __has_include(<avr/pgmspace.h>)
# include <avr/pgmspace.h>
#endif

#ifndef __AVR__
# undef PROGMEM
# define PROGMEM
#endif

namespace Util {
    
    namespace detail {
        template<class T>
        using maybe_cref = typename std::conditional<std::is_integral<T>::value || 
        std::is_same<T, std::byte>::value || std::is_same<T, char>::value, T, const T&>::type;
    }
    
    template<typename T, detail::maybe_cref<T>... Ts>
    class PgmArray final {
        PgmArray() = delete;
    public:
        using U = std::remove_const_t<std::remove_reference_t<T>>;

        typedef typename std::conditional<(sizeof...(Ts) < 256), uint8_t, uint16_t>::type size_type;
        
        inline static U value(size_type index) {
            if constexpr(std::is_same<uint8_t, T>::value || std::is_same<std::byte, T>::value || std::is_same<char, T>::value) {
                return U{pgm_read_byte((uint8_t*)&data[index])};
            }
            else {
                std::array<std::byte, sizeof(T)> bytes;
                for(uint8_t i = 0; i < sizeof(T); ++i) {
                    bytes[i] = std::byte{pgm_read_byte((uint8_t*)(&data[index]) + i)};
                }
                return U::createFrom(bytes);
            }
        }
        
        inline static constexpr size_type size = sizeof... (Ts);
        typedef U type;
        typedef U value_type;
        
        class Iterator {
        public:
            constexpr Iterator(size_type index = 0) : mIndex(index) {}
            inline U operator*() {
                return value(mIndex);
            }
            inline void operator++() {
                ++mIndex;
            }
            inline bool operator!=(const Iterator& rhs) {
                return mIndex != rhs.mIndex;
            }
        private:
            size_type mIndex = 0;
        };
        constexpr Iterator begin() const {
            return Iterator();
        }
        constexpr Iterator end() const {
            return Iterator(size);
        }
        U operator[](size_type index) const {
            return value(index);
        }
    private:
        inline static constexpr const U data[] PROGMEM = {U{Ts}...}; 
    };
 
    namespace Pgm {
        template<typename Generator>
        class Converter {
            inline static constexpr auto mData = Generator{}();    
            typedef typename decltype(mData)::size_type size_type;
            typedef typename decltype(mData)::value_type value_type;
            using index_list = std::make_integer_sequence<size_type, mData.size>;

            template<typename> struct Pgm;
            template<auto... I, typename ValueType> 
            struct Pgm<std::integer_sequence<ValueType, I...>> {
                typedef Util::PgmArray<value_type, mData[I] ...> type;
            };
            
        public:
            using pgm_type = typename Pgm<index_list>::type;
        };
    }
}
