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
#include "std/optional.h"
#include "std/limits.h"
#include "util/dassert.h"

template<typename U, uint8_t FirstBits, uint8_t SecondBits>
class Splitted_NaN {
    static_assert((FirstBits + SecondBits) <= sizeof(U) * 8, "too much bits for type U");
    static constexpr U firstMask = (1 << FirstBits) - 1;
    static constexpr U secondMask = (1 << SecondBits) - 1;
public:
    Splitted_NaN() = default;
    Splitted_NaN(U first, U second) : value(((first & firstMask) << SecondBits) | (second & SecondBits)) {
        assert((first & ~firstMask) == 0);
        assert((second & ~secondMask) == 0);
    }
    U first() const {
        return (value >> SecondBits);
    }
    U second() const {
        return (value & secondMask);
    }
    explicit operator bool() const {
        return (value != std::numeric_limits<U>::max());
    }
private:
    U value = std::numeric_limits<U>::max();
};

template<typename T>
class uint_bounded {
public:
    explicit constexpr uint_bounded(T v = 0) : mValue(v) {}
    
    constexpr bool operator>(T rhs) {
        return mValue > rhs;
    }
    constexpr bool operator>(T rhs) volatile {
        return mValue > rhs;
    }
    uint_bounded& operator--() {
        if (mValue > 0) {
            --mValue;
        }
        return *this;
    }
    void operator++() volatile {
        if (mValue < std::numeric_limits<T>::max()) {
            ++mValue;
        }
    }
    uint_bounded& operator++() {
        if (mValue < std::numeric_limits<T>::max()) {
            ++mValue;
        }
        return *this;
    }
    
    constexpr bool operator==(T rhs) {
        return mValue == rhs;
    }

    constexpr uint_bounded& operator=(T rhs) {
        mValue = rhs;
        return *this;
    }
    constexpr void operator=(T rhs) volatile {
        mValue = rhs;
    }

private:
    T mValue{0};
};

template<typename T>
class uint_NaN final {
    static constexpr T NaN = std::numeric_limits<uint8_t>::max();
public:
    explicit constexpr uint_NaN(T v = 0) : mValue(v) {
        assert(mValue != NaN);
    }
    void setNaN() volatile {
        mValue = NaN;
    }
    void operator=(T v) volatile {
        assert(v != NaN);
        mValue = v;
    }
    uint_NaN& operator=(T v){
        assert(v != NaN);
        mValue = v;
        return *this;
    }
    explicit operator bool() volatile const {
        return mValue != NaN;
    }
    explicit operator bool() const {
        return mValue != NaN;
    }
    operator T() const {
        assert(mValue != NaN);
        return mValue;
    }
    T operator*() volatile {
        assert(mValue != NaN);
        return mValue;
    }
    T operator*() {
        assert(mValue != NaN);
        return mValue;
    }
    void operator++() volatile {
        ++mValue;
    }
    uint_NaN& operator++() {
        ++mValue;
        return *this;
    }
    constexpr bool operator==(uint_NaN& rhs) volatile {
        if (*this && rhs) {
            return mValue == rhs.mValue;
        }
        return false;
    }
    constexpr bool operator<=(uint_NaN& rhs) volatile {
        if (*this && rhs) {
            return mValue <= rhs.mValue;
        }
        return false;
    }
private:
    T mValue = 0;
};

struct uint4_t {
    uint8_t upper : 4, lower : 4;
};

struct uint7_t final {
    constexpr explicit uint7_t(const uint8_t& v) : pad(0), value(v) {}

    constexpr explicit uint7_t(const uint7_t& v) : pad(0), value(v) {}
    explicit uint7_t(volatile uint7_t& v) : pad(0), value(v) {}

    constexpr uint7_t(uint7_t&&) = default;
    constexpr uint7_t& operator=(uint7_t&&) = default;
    void operator=(uint7_t&& rhs) volatile {
        value = rhs.value;
    }

    uint7_t& operator=(const uint7_t&) = default;

    void operator=(volatile uint7_t& rhs) volatile {
        value = rhs.value;
    }

    constexpr operator uint8_t() const {
        return value;
    }
    operator uint8_t() const volatile {
        return value;
    }
    uint8_t pad : 1, value : 7;
};

namespace std {

template<>
class optional<uint7_t> {
public:
    constexpr optional() = default;
    constexpr optional(uint7_t value) : data{(uint8_t)(value.value | 0x80)} {}
    constexpr explicit operator bool() const {
        return data & 0x80;
    }
    constexpr explicit operator bool() {
        return data & 0x80;
    }
    constexpr uint7_t operator*() const {
        return uint7_t{data};
    }
private:
    uint8_t data{0};
};

template<>
struct numeric_limits<uint7_t> {
    static constexpr uint8_t max() {return UINT8_MAX / 2 - 1;}
    static constexpr uint8_t min() {return 0;}
};


}
