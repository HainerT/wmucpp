#pragma once

#include <cstdint>
#include <type_traits>

#include "type_traits.h"
#include "concepts.h"

namespace etl {
    using namespace std;
    
    enum class Char : uint8_t {};
    
    template<bool V>
    struct NamedFlag : integral_constant<bool, V> {};
    
    template<auto c>
    struct NamedConstant : integral_constant<decltype(c), c> {};
    
    template<auto Bits>
    class bitsN_t {
    public:
        inline static constexpr auto size = Bits;
        typedef typeForBits_t<Bits> value_type;
        inline static constexpr value_type mask = ((1 << Bits) - 1);
        constexpr bitsN_t(const volatile bitsN_t& o) : mValue{o.mValue} {}
        constexpr bitsN_t() = default;
        constexpr explicit bitsN_t(value_type v) : mValue(v & mask) {}
        constexpr explicit bitsN_t(std::byte v) : mValue(std::to_integer<value_type>(v) & mask) {}
        constexpr explicit operator value_type() const {
            return mValue;
        }
    private:
        value_type mValue{};
    };
    
    template<auto Bits>
    class uintN_t {
    public:
        typedef typeForBits_t<Bits> value_type;
        inline static constexpr value_type mask = ((1 << Bits) - 1);
        explicit uintN_t(value_type v = 0) : mValue(v & mask) {}
        constexpr operator value_type() const {
            return mValue;
        }
        constexpr uintN_t& operator++() {
            ++mValue;
            mValue &= mask;
            return *this;
        }
    private:
        value_type mValue{};
    };
    
    template<Unsigned T>
    class uint_NaN final {
        inline static constexpr T NaN = std::numeric_limits<T>::max();
    public:
        inline explicit constexpr uint_NaN(T v) : mValue(v) {
            assert(mValue != NaN);
        }
        inline constexpr uint_NaN() : mValue(NaN) {}
        
        inline void setNaN() volatile {
            mValue = NaN;
        }
        inline void operator=(T v) volatile {
            assert(v != NaN);
            mValue = v;
        }
        inline uint_NaN& operator=(T v){
            assert(v != NaN);
            mValue = v;
            return *this;
        }
        inline explicit operator bool() volatile const {
            return mValue != NaN;
        }
        inline constexpr explicit operator bool() const {
            return mValue != NaN;
        }
        inline constexpr T toInt() const {
            assert(mValue != NaN);
            return mValue;
        }
        inline volatile T& operator*() volatile {
            assert(mValue != NaN);
            return mValue;
        }
        inline T& operator*() {
            assert(mValue != NaN);
            return mValue;
        }
        inline const T& operator*() const {
            assert(mValue != NaN);
            return mValue;
        }
        inline void operator++() volatile {
            ++mValue;
        }
        inline uint_NaN& operator++() {
            ++mValue;
            return *this;
        }
        inline void operator--() volatile {
            --mValue;
        }
        inline uint_NaN& operator--() {
            --mValue;
            return *this;
        }
        inline constexpr bool operator==(uint_NaN rhs) volatile {
            if (*this && rhs) {
                return mValue == rhs.mValue;
            }
            return false;
        }
        inline constexpr bool operator<=(uint_NaN rhs) volatile {
            if (*this && rhs) {
                return mValue <= rhs.mValue;
            }
            return false;
        }
    private:
        T mValue = 0;
    };
    template<Unsigned T = uint8_t, T LowerBound = 0, T UpperBound = std::numeric_limits<T>::max()>
    class uint_ranged final {
    public:
        inline static constexpr T Lower = LowerBound;
        inline static constexpr T Upper = UpperBound;
        typedef T type;
        
        constexpr uint_ranged(T v = 0) : mValue(v) {
            assert(v >= LowerBound);
            assert(v <= UpperBound);
        }
        
        constexpr uint_ranged(const volatile uint_ranged& o) : mValue(o.mValue) {}
        
        constexpr bool isTop() const {
            return mValue == Upper;
        }
        
        constexpr bool isBottom() const {
            return mValue == Lower;
        }
        
        constexpr bool operator>(T rhs) {
            return mValue > rhs;
        }
        constexpr bool operator>(T rhs) volatile {
            return mValue > rhs;
        }
        uint_ranged& operator--() {
            if (mValue > LowerBound) {
                --mValue;
            }
            return *this;
        }
        void operator++() volatile {
            if (mValue < UpperBound) {
                ++mValue;
            }
        }
        uint_ranged& operator++() {
            if (mValue < UpperBound) {
                ++mValue;
            }
            return *this;
        }
        constexpr bool operator==(T rhs) {
            return mValue == rhs;
        }
        constexpr uint_ranged& operator=(T rhs) {
            assert(rhs >= LowerBound);
            assert(rhs <= UpperBound);
            mValue = rhs;
            return *this;
        }
        constexpr void operator=(T rhs) volatile {
            assert(rhs >= LowerBound);
            assert(rhs <= UpperBound);
            mValue = rhs;
        }
        constexpr operator T() const {
            return mValue;
        }
        constexpr operator T() volatile const {
            return mValue;
        }
        constexpr T toInt() const {
            return mValue;
        }
        constexpr T toInt() volatile const {
            return mValue;
        }
    private:
        T mValue{0};
    };
    template<Unsigned T = uint8_t, T LowerBound = 0, T UpperBound = std::numeric_limits<T>::max() - 1>
    class uint_ranged_NaN final {
    public:
        inline static constexpr T Lower = LowerBound;
        inline static constexpr T Upper = UpperBound;
        inline static constexpr T NaN   = std::numeric_limits<T>::max();
        
        static_assert(Upper != NaN);
        
        typedef T type;
        
        constexpr uint_ranged_NaN() = default;
        
        constexpr uint_ranged_NaN(T v) : mValue(v) {
            assert(v >= LowerBound);
            assert(v <= UpperBound);
        }
        
        constexpr explicit operator bool() const {
            return mValue != NaN;
        }
        
        constexpr bool operator>(T rhs) {
            return mValue > rhs;
        }
        uint_ranged_NaN& operator--() {
            if (mValue > LowerBound) {
                --mValue;
            }
            return *this;
        }
        uint_ranged_NaN& operator++() {
            if (mValue < UpperBound) {
                ++mValue;
            }
            return *this;
        }
        constexpr bool operator==(T rhs) {
            return mValue == rhs;
        }
        constexpr uint_ranged_NaN& operator=(T rhs) {
            assert(rhs >= LowerBound);
            assert(rhs <= UpperBound);
            mValue = rhs;
            return *this;
        }
    //    constexpr operator T() const {
    //        return mValue;
    //    }
        constexpr T toInt() const {
            return mValue;
        }
    private:
        T mValue{NaN};
    };

    template<typename T>
    struct combinedType;
    
    template<>
    struct combinedType<uint8_t> {
        typedef uint16_t type;
        static constexpr const uint8_t shift = 8;
    };
    
    template<typename T>
    typename combinedType<T>::type combinedValue(volatile const pair<T, T>& p) {
        return (p.first << combinedType<T>::shift) + p.second;
    }
    
    template<typename T>
    typename combinedType<T>::type combinedValue(const pair<T, T>& p) {
        return (p.first << combinedType<T>::shift) + p.second;
    }
}


namespace std {
    template<>
    struct numeric_limits<etl::uint_NaN<uint8_t>> {
        typedef etl::uint_NaN<uint8_t> type;
        static constexpr uint8_t max() {return UINT8_MAX - 1;}
        static constexpr uint8_t min() {return 0;}
    };
    
}
