/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2013, 2014, 2015, 2016 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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

namespace std {

template<typename T>
struct underlying_type;

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


}
