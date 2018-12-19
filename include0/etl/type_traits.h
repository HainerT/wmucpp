#pragma once

#include <cstdint>
#include <limits>

#include "concepts.h"
#include "meta.h"

namespace etl {
    namespace detail {
        template<auto Bits>
        struct typeForBits {
            using type = typename std::conditional<(Bits <= 8), uint8_t, 
                             typename std::conditional<(Bits <= 16), uint16_t, 
                                 typename std::conditional<(Bits <= 32), uint32_t,
                                      typename std::conditional<(Bits <= 64), uint64_t, void>::type>::type>::type>::type;
        };
        template<auto V>
        struct typeForValue {
            using type = typename std::conditional<(V > std::numeric_limits<uint32_t>::max()), uint64_t, 
                                  typename std::conditional<(V > std::numeric_limits<uint16_t>::max()), uint32_t,
                                    typename std::conditional<(V > std::numeric_limits<uint8_t>::max()), uint16_t, uint8_t>::type>::type>::type;
        };
    
        template<typename T> struct enclosingType;

        template<> struct enclosingType<uint8_t> {
            typedef uint16_t type;
        };
        template<> struct enclosingType<uint16_t> {
            typedef uint32_t type;
        };
        template<> struct enclosingType<uint32_t> {
            typedef uint64_t type;
        };
        template<> struct enclosingType<int8_t> {
            typedef int16_t type;
        };
        template<> struct enclosingType<int16_t> {
            typedef int32_t type;
        };
        template<> struct enclosingType<int32_t> {
            typedef int64_t type;
        };
        
        template<typename T>struct fragmentType;
        
        template<> struct fragmentType<uint16_t> {
            typedef uint8_t type;
            static constexpr const uint8_t shift = 8;
        };
        
        template<> struct fragmentType<uint32_t> {
            typedef uint16_t type;
            static constexpr const uint8_t shift = 16;
        };
        template<> struct fragmentType<uint64_t> {
            typedef uint32_t type;
            static constexpr const uint8_t shift = 32;
        };
    }
    
    template<typename E, typename = std::enable_if_t<std::enable_bitmask_operators_v<E>>>
    inline constexpr bool
    toBool(E v) {
        return static_cast<bool>(v);        
    }
    
    template<typename T>
    constexpr uint8_t numberOfBits() {
        return sizeof(T) * 8;
    }

    template<uint64_t Bits>
    using typeForBits_t = typename detail::typeForBits<Bits>::type;  
    
    template<auto V>
    using typeForValue_t = typename detail::typeForValue<V>::type;
    
    template<typename T>
    using enclosingType_t = typename detail::enclosingType<T>::type;
    
    template<typename T>
    using fragmentType_t = typename detail::fragmentType<T>::type;    
    
    
    using namespace etl::Concepts;
    
    template<Integral T, uint8_t Base = 10>
    constexpr uint8_t numberOfDigits() {
        T v = std::numeric_limits<T>::max();
        uint8_t number = 0;
        while(v > 0) {
            v /= Base;
            ++number;
        }
        if (number == 0) {
            number = 1;
        }
        if constexpr(std::is_signed<T>::value) {
            number += 1;
        }
        return number;
    }

    template<typename T, uint8_t Base = 10>
    requires (T::valid_bits > 0)
    constexpr uint8_t numberOfDigits() {
        double v = 1.0;
        for(uint8_t b = 0; b < T::valid_bits; ++b) {
            v /= 2.0;
        }
        uint8_t number = 0;
        while((v - (uint64_t)v) > 0.0) {
            number += 1;
            v *= 10.0;
        }
        return number;
    }
    
    constexpr bool isPowerof2(int v) {
        return v && ((v & (v - 1)) == 0);
    }
    
}
