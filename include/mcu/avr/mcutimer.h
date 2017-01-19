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

#include <stdint.h>

#include "mcu/avr8.h"
#include "mcu/avr/isr.h"
#include "mcu/avr/util.h"
#include "units/physical.h"

namespace AVR {

enum class TimerMode : uint8_t {Normal = 0, CTC, OverflowInterrupt, NumberOfModes};

template<typename MCU, uint8_t N>
struct TimerBase
{
    TimerBase() = delete;
    static constexpr auto mcuInterrupts = getBaseAddr<typename MCU::TimerInterrupts, N>;
    typedef MCU mcu_type;
};

template<uint8_t N, typename MCU = DefaultMcuType>
class Timer8Bit : public TimerBase<MCU, N> {
public:
    static constexpr uint8_t number = N;
    static constexpr uint8_t csBitMask = 0x07;
    static constexpr auto mcuTimer = getBaseAddr<typename MCU::Timer8Bit, N>;
    typedef typename MCU::Timer8Bit mcu_timer_type;
    typedef uint8_t value_type;

    static constexpr const bool hasOcrA = true;
    static constexpr const bool hasOcrB = true;
    static constexpr const bool hasOverflow = true;

    Timer8Bit() = delete;

    template<int PreScale>
    static void prescale() {
        constexpr uint8_t bits = AVR::Util::bitsFrom<PreScale>(MCU::Timer8Bit::template PrescalerBits<N>::values);
        static_assert(bits != 0, "wrong prescaler");
        mcuTimer()->tccrb |= bits;
    }

    static std::hertz frequency() {
        return Config::fMcu / (uint32_t)prescaler();
    }

    static AVR::PrescalerPair::scale_type prescaler() {
        const uint8_t bits = mcuTimer()->tccrb & csBitMask;
        return AVR::Util::bitsToPrescale(bits, MCU::Timer8Bit::template PrescalerBits<N>::values);
    }
    
    static void start(){
    }

    static void ocra(uint8_t v) {
        mcuTimer()->ocra = v;
    }
    template<uint8_t V>
    static void ocra() {
        mcuTimer()->ocra = V;
    }

    static inline volatile uint8_t& counter() {
        return mcuTimer()->tcnt;
    }

//    using ta = typename MCU::Timer8Bit::TCCRA;
    
    static void mode(const TimerMode& mode) {
        if (mode == TimerMode::CTC) {
            TimerBase<MCU, N>::mcuInterrupts()->timsk |= MCU::TimerInterrupts::template Flags<N>::ociea;
            
            // todo: test and cleanup
            
            mcuTimer()->tccra.template set<MCU::Timer8Bit::TCCRA::wgm1>();
//                        mcuTimer()->tccra = MCU::Timer8Bit::template Flags<N>::wgm1;
        }
        else if (mode == TimerMode::Normal) {
        }
        else if (mode == TimerMode::OverflowInterrupt) {
            TimerBase<MCU, N>::mcuInterrupts()->timsk |= MCU::TimerInterrupts::template Flags<N>::toie;
//            TimerBase<MCU, N>::mcuInterrupts()->timsk |= _BV(TOIE0);
        }
    }
};

template<>
class Timer8Bit<1, ATTiny25> : public TimerBase<ATTiny25, 1> {
public:
    static constexpr uint8_t number = 1;
    static constexpr uint8_t csBitMask = 0x0f;
    static constexpr auto mcuTimer = getBaseAddr<typename ATTiny25::Timer8BitHighSpeed, 1>;
    typedef typename ATTiny25::Timer8Bit mcu_timer_type;
    typedef uint8_t value_type;

    static constexpr const bool hasOcrA = true;
    static constexpr const bool hasOcrB = true;
    static constexpr const bool hasOcrC = true;
    static constexpr const bool hasOverflow = true;

    Timer8Bit() = delete;

    template<int PreScale>
    static void prescale() {
        constexpr uint8_t bits = AVR::Util::bitsFrom<PreScale>(ATTiny25::Timer8BitHighSpeed::template PrescalerBits<1>::values);
        static_assert(bits != 0, "wrong prescaler");
        mcuTimer()->tccr |= bits;
    }

    static std::hertz frequency() {
        return Config::fMcu / (uint32_t)prescaler();
    }

    static AVR::PrescalerPair::scale_type prescaler() {
        const uint8_t bits = mcuTimer()->tccr & csBitMask;
        return AVR::Util::bitsToPrescale(bits, ATTiny25::Timer8BitHighSpeed::template PrescalerBits<1>::values);
    }
    
    static void start(){
    }

    static void ocra(uint8_t v) {
        mcuTimer()->ocra = v;
    }
    template<uint8_t V>
    static void ocra() {
        mcuTimer()->ocra = V;
    }

    static inline volatile uint8_t& counter() {
        return mcuTimer()->tcnt;
    }
    
    static void mode(const TimerMode& mode) {
        if (mode == TimerMode::CTC) {
            TimerBase<ATTiny25, 0>::mcuInterrupts()->timsk |= ATTiny25::TimerInterrupts::template Flags<1>::ociea;
        }
        else if (mode == TimerMode::Normal) {
        }
    }
};


template<uint8_t N>
struct Timer8Bit<N, ATMega8> : public TimerBase<ATMega8, N>
{
    static constexpr uint8_t number = N;
    static constexpr auto mcuTimer = getBaseAddr<typename ATMega8::Timer8Bit, N>;
    typedef typename ATMega8::Timer8Bit mcu_timer_type;
    typedef uint8_t value_type;

    static constexpr const bool hasOcrA = false;
    static constexpr const bool hasOcrB = false;
    static constexpr const bool hasOverflow = true;
    
    Timer8Bit() = delete;

    template<int PreScale>
    static constexpr void prescale() {
        constexpr uint8_t bits = AVR::Util::bitsFrom<PreScale>(ATMega8::Timer8Bit::template PrescalerBits<N>::values);
        static_assert(bits != 0, "wrong prescaler");
        mcuTimer()->tccr |= bits;
    }
    static void start(){
    }
    
    static void mode(const TimerMode& mode) {
        if (mode == TimerMode::CTC) {
            TimerBase<ATMega8, N>::mcuInterrupts->timsk |= ATMega8::TimerInterrupts::template Flags<N>::ociea;
            mcuTimer()->tccr = ATMega8::Timer8Bit::template Flags<N>::wgm1;
        }
        else if (mode == TimerMode::Normal) {
        }
    }
};

template<uint8_t N>
struct Timer8Bit<N, ATMegaNone> : public TimerBase<ATMegaNone, N>
{
    template<int PreScale>
    static void prescale() {
    }
    static void start(){
    }
};

template<uint8_t N, typename MCU = DefaultMcuType>
struct Timer16Bit: public TimerBase<MCU, N>
{
    typedef AVR::ISR::Timer<N> isr_type;
    static constexpr uint8_t number = N;
    static constexpr uint8_t csBitMask = 0x07;
    static constexpr auto mcuTimer = getBaseAddr<typename MCU::Timer16Bit, N>;
    typedef typename MCU::Timer16Bit mcu_timer_type;
    typedef uint16_t value_type;

    static constexpr const bool hasOcrA = true;
    static constexpr const bool hasOcrB = true;
    static constexpr const bool hasOverflow = true;

    Timer16Bit() = delete;

    template<int PreScale>
    static void prescale() {
        constexpr uint8_t bits = AVR::Util::bitsFrom<PreScale>(MCU::Timer16Bit::template PrescalerBits<N>::values);
        static_assert(bits != 0, "wrong prescaler");
        mcuTimer()->tccrb |= bits;
    }
    
    static std::hertz frequency() {
        return Config::fMcu / (uint32_t)prescaler();
    }

    static AVR::PrescalerPair::scale_type prescaler() {
        const uint8_t bits = mcuTimer()->tccrb & csBitMask;
        return AVR::Util::bitsToPrescale(bits, MCU::Timer16Bit::template PrescalerBits<N>::values);
    }

    static void ocra(uint16_t v) {
        mcuTimer()->ocra = v;
    }
    template<uint16_t V>
    static void ocra() {
        mcuTimer()->ocra = V;
    }

    static inline volatile uint16_t& counter() {
        return mcuTimer()->tcnt;
    }
    
    static void start(){
    }

    static void mode(const TimerMode& mode) {
        if (mode == TimerMode::CTC) {
            TimerBase<MCU, N>::mcuInterrupts()->timsk |= MCU::TimerInterrupts::template Flags<N>::ociea;
            mcuTimer()->tccrb = MCU::Timer16Bit::template Flags<N>::wgm2;
//            mcuTimer()->tccrb |= _BV(WGM12);
        }
    }
};

}
