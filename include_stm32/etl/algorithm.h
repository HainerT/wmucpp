#pragma once

#include <cstdint>
#include <limits>
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <numbers>

namespace etl {
    constexpr inline uint8_t asDigit(const char c) {
        return (static_cast<uint8_t>(c) - '0');
    }

    template<auto L>
    inline static float from_chars(const std::array<char, L>& str) {
        float value = 0;
        int8_t decimals = -1;

        uint8_t index = 0;

        for(; index < L; ++index) {
            if (str[index] == '.') {
                decimals = 0;
            }
            else if (std::isdigit(str[index])) {
                value *= 10;
                value += etl::asDigit(str[index]);
                if (decimals >= 0) {
                    ++decimals;
                }
            }
            else if (str[index] == '\0') {
                break;
            }
        }
        while(decimals > 0) {
            value /= 10;
            --decimals;
        }
        return value;
    }


    template<typename T>
    bool equalStore(T& a, const T& b) {
        const bool result = (a == b);
        a = b;
        return result;
    }

    template<typename C>
    bool contains(const C& c, const typename C::value_type e) {
        for(const auto& i : c) {
            if (e == i) {
                return true;
            }
        }
        return false;
    }
    template<typename C1, typename C2>
    constexpr void copy(const C1& src, C2& dst) {
        std::copy(std::begin(src), std::end(src), std::begin(dst));
    }
    template<typename T>
    static inline T assign(T& lhs, const std::remove_cv_t<T> rhs) {
        lhs = rhs;
        return rhs;
    }
    template<typename C>
    constexpr void push_back_ntbs(const char* s, C& c) {
        do {
            c.push_back(typename C::value_type{*s});
        } while(*s++ != '\0');
    }
    template<typename C>
    constexpr void push_back_ntbs_or_emptyString(const char* s, C& c) {
        if (s) {
            do {
                c.push_back(typename C::value_type{*s});
            } while(*s++ != '\0');
        }
        else {
            c.push_back(uint8_t{'\0'});
        }
    }
    template<typename C>
    constexpr C::value_type maximum(const C& c) {
        return [&]<auto... II>(const std::index_sequence<II...>){
            typename C::value_type max{c[0]};
            ((c[II + 1] > max ? max = c[II + 1]: 0), ...);
            return max;
        }(std::make_index_sequence<std::tuple_size_v<C> - 1>{});
    }   

    template<typename C>
    inline consteval bool isSet(const C& c) {
        using size_type = C::size_type;
        for(size_type i{0}; i < c.size(); ++i) {
            for(size_type n = i + 1; n < c.size(); ++n) {
                if (c[i] == c[n]) {
                    return false;
                }
            }
        }
        return true;
    }
    
    template<uint8_t Number, typename T>
    constexpr inline std::byte nth_byte(const T v);
    
    template<>
    constexpr inline std::byte nth_byte<0, uint8_t>(const uint8_t v) {
        return std::byte(v);
    }
    template<>
    constexpr inline std::byte nth_byte<0, int8_t>(const int8_t v) {
        return std::byte(v);
    }
    template<>
    constexpr inline std::byte nth_byte<0, std::byte>(const std::byte v) {
        return v;
    }
    template<>
    constexpr inline std::byte nth_byte<0, uint16_t>(const uint16_t v) {
        return std::byte(v);
    }
    template<>
    constexpr inline std::byte nth_byte<1, uint16_t>(const uint16_t v) {
        return std::byte(v >> 8);
    }
    
    template<>
    constexpr inline std::byte nth_byte<0, uint32_t>(const uint32_t v) {
        return std::byte(v);
    }
    template<>
    constexpr inline std::byte nth_byte<1, uint32_t>(const uint32_t v) {
        return std::byte(v >> 8);
    }
    template<>
    constexpr inline std::byte nth_byte<2, uint32_t>(const uint32_t v) {
        return std::byte(v >> 16);
    }
    template<>
    constexpr inline std::byte nth_byte<3, uint32_t>(const uint32_t v) {
        return std::byte(v >> 24);
    }
    template<>
    constexpr inline std::byte nth_byte<0, int>(const int v) {
        return std::byte(v);
    }
    template<typename C>
    constexpr void serialize(const char* s, C& c) {
        push_back_ntbs(s, c);
    }
    template<typename T, typename C>
    constexpr void serialize(const T& v, C& c) {
        [&]<auto... II>(std::index_sequence<II...>){
            (c.push_back(nth_byte<II>(v)), ...);
        }(std::make_index_sequence<sizeof(T)>{});
    }

    template<typename T, typename C>
    constexpr void serializeBE(const T v, C& c) {
        [&]<auto... II>(std::index_sequence<II...>){
            (c.push_back(nth_byte<sizeof(T) - II - 1>(v)), ...);
        }(std::make_index_sequence<sizeof(T)>{});
    }

}

