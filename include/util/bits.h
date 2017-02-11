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

#include <stdint.h>
#include "std/traits.h"

namespace std {

// todo: move to namespace Util

template<typename E>
struct enable_bitmask_operators final {
    static constexpr const bool enable = false;
};

}

namespace Util {
    template<typename T>
    constexpr uint8_t numberOfBits() {
        return sizeof(T) * 8;
    }
    template<typename Bit, typename T>
    constexpr bool isSet(T v) {
        return Bit::template Value<T>::value & v;
    }
    template<typename Bit, typename T>
    constexpr bool set(T& v) {
        return v |= Bit::template Value<T>::value;
    }
    
    struct MSB {
        template<typename T>
        struct Value {
            static constexpr const T value = (1 << (numberOfBits<T>() - 1));
        };
    };  
    struct LSB {
        template<typename T>
        struct Value {
            static constexpr const T value = 1;
        };
    };  
    template<typename T>
    constexpr uint8_t numberOfOnes(T x) {
        return x ? int(x & 0x01) + numberOfOnes(x>>1) : 0;
    }

    template<typename T>
    struct enclosingType;
    template<>
    struct enclosingType<uint8_t> {
        typedef uint16_t type;
    };
    template<>
    struct enclosingType<uint16_t> {
        typedef uint32_t type;
    };
}

template<typename E>
constexpr
typename std::enable_if<std::enable_bitmask_operators<E>::enable,E>::type
operator|(E lhs, E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
                static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename E>
constexpr
typename std::enable_if<std::enable_bitmask_operators<E>::enable,E>::type&
operator|=(E& lhs, E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return lhs = static_cast<E>(
                static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename E>
constexpr
typename std::enable_if<std::enable_bitmask_operators<E>::enable,E>::type
operator&(E lhs, E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
                static_cast<underlying>(lhs) & static_cast<underlying>(rhs)
                );
}

template<typename E>
constexpr
typename std::enable_if<std::enable_bitmask_operators<E>::enable,E>::type&
operator&=(E& lhs, E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return lhs = static_cast<E>(
                static_cast<underlying>(lhs) & static_cast<underlying>(rhs)
                );
}

template<typename E>
constexpr
typename std::enable_if<std::enable_bitmask_operators<E>::enable,E>::type
operator~(E rhs){
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<E>(
                ~static_cast<underlying>(rhs)
                );
}

template<typename E>
constexpr bool isset(E flags) {
    typedef typename std::underlying_type<E>::type underlying;
    return static_cast<underlying>(flags) != 0;
}

