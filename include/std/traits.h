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
#include "std/utility.h"

namespace std {

#ifndef __GLIBCXX__

// The underlying type of an enum.
template<typename _Tp>
struct underlying_type {
    typedef __underlying_type(_Tp) type;
};

template<bool B, class T = void>
struct enable_if {};

template<class T>
struct enable_if<true, T> {
    typedef T type;
};

template<bool B, class T = void>
struct enable_if_not {};

template<class T>
struct enable_if_not<false, T> {
    typedef T type;
};

template<bool B, class T, class F>
struct conditional { 
    typedef T type; 
};
 
template<class T, class F>
struct conditional<false, T, F> { 
    typedef F type; 
};

template<class T, class U>
struct is_same final {
    static constexpr bool value = false;
};
 
template<class T>
struct is_same<T, T> final {
    static constexpr bool value = true;
};

template<typename T1, typename T2>
struct logic_or : public std::conditional<T1::value, T1, T2>::type {};

template<typename T1, typename T2>
struct logic_and : public std::conditional<T1::value, T2, T1>::type {};

template<typename T, T v>
struct integral_constant {
    inline static constexpr T value = v;
    typedef T value_type;
    typedef integral_constant<T, v> type;
    constexpr operator T() const {return v;}
};

typedef integral_constant<bool, true>  true_type;
typedef integral_constant<bool, false> false_type;

template<typename T>
struct logic_not : public integral_constant<bool, !T::value> {};

namespace detail {
    template<typename T>
    struct is_integral_base : public false_type {};
    template<>
    struct is_integral_base<uint8_t> : public true_type {
    };
    template<>
    struct is_integral_base<int8_t> : public true_type {
    };
    template<>
    struct is_integral_base<uint16_t> : public true_type {
    };
    template<>
    struct is_integral_base<int16_t> : public true_type {
    };
    template<>
    struct is_integral_base<uint32_t> : public true_type {
    };
    template<>
    struct is_integral_base<int32_t> : public true_type {
    };
    template<>
    struct is_integral_base<uint64_t> : public true_type {
    };
    template<>
    struct is_integral_base<int64_t> : public true_type {
    };
    
    template<typename T>
    struct is_floatingpoint_helper : public false_type {};
    template<>
    struct is_floatingpoint_helper<float> : public true_type {};
    template<>
    struct is_floatingpoint_helper<double> : public true_type {};
    
    template<typename T>
    struct is_signed_helper: public integral_constant<bool, T(-1) < T(0)> {};
    
} // !detail

template<typename T>
struct is_integral : public detail::is_integral_base<typename std::remove_cv<T>::type> {
};
template<typename T>
struct is_floating_point : public detail::is_floatingpoint_helper<typename std::remove_cv<T>::type> {
};
template<typename T>
struct is_arithmatic : public logic_or<is_integral<T>, is_floating_point<T>>::type {};
template<typename T>
struct is_signed : public logic_and<is_arithmatic<T>, detail::is_signed_helper<T>>::type {};
template<typename T>
struct is_unsigned : public logic_and<is_arithmatic<T>, logic_not<is_signed<T>>>::type {};
        
#endif
}
