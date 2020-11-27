#pragma once

#include <std/chrono>
#include <etl/types.h>

#include <compare>

namespace External {
    
    template<typename Timer, typename T = uint16_t, T MAX = std::numeric_limits<T>::max()>
    struct Tick {
        inline static constexpr auto intervall = Timer::intervall;
        
        inline constexpr Tick() = default;
        
        template<typename R, typename P>
        inline constexpr explicit Tick(const std::chrono::duration<R, P>& v) : value(v / intervall) {
//            static_assert(v / intervall <= std::numeric_limits<T>::max());
        }
        
        template<typename R, typename P>
        inline constexpr void operator=(const std::chrono::duration<R, P>& v) {
            value.set(v / intervall);
        }  
        
        inline constexpr auto time() const {
            return intervall * value.toInt();
        }
        
        inline explicit constexpr operator bool() const {
            return value != 0;
        }
        inline constexpr void operator++() {
            ++value;
        }
        inline constexpr void operator--() {
            --value;
        }

        inline constexpr void operator++() volatile {
            ++value;
        }
        
        template<typename F>
        inline constexpr void on(const Tick& t, F f) {
            if (value >= t.value) {
                f();
                reset();
            }
        }
        template<typename F>
        inline constexpr void on(const Tick& t, F f) volatile {
            if (value >= t.value) {
                f();
                reset();
            }
        }
        template<typename F>
        inline constexpr void match(const Tick& t, F f) const{
            if (value == t.value) {
                f();
            }
        }
        
        inline constexpr auto operator<=>(const Tick& rhs) const = default;
        
        inline constexpr void reset() {
            value.setToBottom();
        }

        inline constexpr void reset() volatile {
            value.setToBottom();
        }

        inline static constexpr Tick fromRaw(const T v) {
            return Tick(v);
        }

        inline static constexpr T max() {
            return decltype(value)::Upper;
        }
        
        inline constexpr auto operator/(const uint16_t& d) const {
            return Tick(value / d);    
        }

        inline constexpr auto operator*(const uint16_t& m) const {
            return Tick(value * m);    
        }
        
        //    private: // structural type
        etl::uint_ranged<T, 0, MAX> value;
        
    private:
        inline constexpr Tick(const T v) : value{v} {}
    };
}
