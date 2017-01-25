/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2013, 2014, 2015, 2016, 2016, 2017 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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

#include "avr8defs.h"

#pragma pack(push)
#pragma pack(1)

namespace AVR {

struct ATMega8 final
{
    ATMega8() = delete;
    struct Usart {
        volatile uint8_t ubbrl;
        volatile uint8_t ucsrb;
        volatile uint8_t ucsra;
        volatile uint8_t udr;
        template<int N> struct Address;
    };
    struct Timer8BitSimple {
        volatile uint8_t tcnt;
        enum class TCCR : uint8_t {
#ifdef CS02
            cs2 = (1 << CS02),
#endif
#ifdef CS01
            cs1 = (1 << CS01),
#endif
#ifdef CS00
            cs0 = (1 << CS00),
#endif
        };
        ControlRegister<Timer8BitSimple, TCCR> tccr;
        
        template<int N> struct Address;
        template<int N> struct PrescalerBits;
    };
    struct Timer8BitSimple2 {
        static constexpr const uint8_t count = 1;
        volatile uint8_t ocr;
        volatile uint8_t tcnt;
        enum class TCCR : uint8_t {
#ifdef FOC2
            foc = (1 << FOC2),
#endif
#ifdef WGM20
            wgm0 = (1 << WGM20),
#endif
#ifdef WGM21
            wgm1 = (1 << WGM21),
#endif
#ifdef CS22
            cs2 = (1 << CS22),
#else
            cs2 = 0,
#endif
#ifdef CS21
            cs1 = (1 << CS21),
#else
            cs1 = 0,
#endif
#ifdef CS20
            cs0 = (1 << CS20),
#else
            cs0 = 0,
#endif
        };
        
        ControlRegister<Timer8BitSimple2, TCCR> tccr;
        // todo: together in one map struct
        template<int N> struct Address;
        template<int N> struct PrescalerBits;
    };
    
    struct Timer16Bit {
        union {
            struct {
                volatile uint8_t icrl;
                volatile uint8_t icrh;
            };
            volatile uint16_t icr;
        };
        union {
            struct {
                volatile uint8_t ocrbl;
                volatile uint8_t ocrbh;
            };
            volatile uint16_t ocrb;
        };
        union {
            struct {
                volatile uint8_t ocral;
                volatile uint8_t ocrah;
            };
            volatile uint16_t ocra;
        };
        volatile uint8_t tcntl;
        volatile uint8_t tcnth;

        enum class TCCRB : uint8_t {
#ifdef ICNC1
            icnc = (1 << ICNC1),
#endif
#ifdef ICES1
            ices = (1 << ICES1),
#endif
#ifdef WGM13
            wgm3 = (1 << WGM13),
#endif
#ifdef WGM12
            wgm2 = (1 << WGM12),
#endif
#ifdef CS12
            cs2 = (1 << CS12),
#endif
#ifdef CS11
            cs1 = (1 << CS11),
#endif
#ifdef CS10
            cs0 = (1 << CS10),
#endif
        };
        ControlRegister<Timer16Bit, TCCRB> tccrb;
//        volatile uint8_t tccrb;
        enum class TCCRA : uint8_t {
#ifdef COM1A0
            coma0 = (1 << COM1A0),
#endif
#ifdef COM1A1
            coma1 = (1 << COM1A1),
#endif
#ifdef COM1B0
            comb0 = (1 << COM1B0),
#endif
#ifdef COM1B1
            comb1 = (1 << COM1B1),
#endif
#ifdef FOC1A
            foca = (1 << FOC1A),
#endif
#ifdef FOC1B
            focb = (1 << FOC1B),
#endif
#ifdef WGM10
            wgm0 = (1 << WGM10),
#endif        
#ifdef WGM11
            wgm1 = (1 << WGM11)
#endif        
        };
        ControlRegister<Timer16Bit, TCCRA> tccra;
//        volatile uint8_t tccra;
        template<int N> struct Address;
        template<int N> struct PrescalerBits;
        template<uint8_t N> struct Flags; 
    };
    struct PortRegister {
        volatile uint8_t in;
        volatile uint8_t ddr;
        volatile uint8_t out;
        template<typename P> struct Address;
    };
    struct Interrupt {
        volatile uint8_t tifr;
        volatile uint8_t timsk;
        volatile uint8_t gifr;
        volatile uint8_t gicr;
        static constexpr uint8_t address = 0x58;
    };
    class TimerInterrupts {
    public:
        volatile uint8_t tifr;
        volatile uint8_t timsk;
        template<uint8_t N> struct Address;
        template<uint8_t N> struct Flags; 
    };
};
}

namespace std {
template<>
struct enable_bitmask_operators<AVR::ATMega8::Timer8BitSimple::TCCR> {
    static constexpr bool enable = true;
};
template<>
struct enable_bitmask_operators<AVR::ATMega8::Timer8BitSimple2::TCCR> {
    static constexpr bool enable = true;
};
template<>
struct enable_bitmask_operators<AVR::ATMega8::Timer16Bit::TCCRA> {
    static constexpr bool enable = true;
};
template<>
struct enable_bitmask_operators<AVR::ATMega8::Timer16Bit::TCCRB> {
    static constexpr bool enable = true;
};
}

namespace AVR {

template<>
struct ATMega8::TimerInterrupts::Address<0> {
    static constexpr uint8_t value = 0x58;
};
template<>
struct ATMega8::TimerInterrupts::Address<1> {
    static constexpr uint8_t value = 0x58;
};
template<>
struct ATMega8::TimerInterrupts::Address<2> {
    static constexpr uint8_t value = 0x58;
};
template<>
struct ATMega8::TimerInterrupts::Flags<0> {
};
template<>
struct ATMega8::TimerInterrupts::Flags<1> {
    static constexpr uint8_t ociea = _BV(OCIE1A);
};

template<>
struct ATMega8::PortRegister::Address<B> {
    static constexpr uint8_t value = 0x36;
};
template<>
struct ATMega8::PortRegister::Address<C> {
    static constexpr uint8_t value = 0x33;
};
template<>
struct ATMega8::PortRegister::Address<D> {
    static constexpr uint8_t value = 0x30;
};
template<>
struct ATMega8::Usart::Address<0> {
    static constexpr uint8_t value = 0x29;
};

template<>
struct ATMega8::Timer8BitSimple::Address<0> {
    static constexpr uint8_t value = 0x52;
};
template<>
struct ATMega8::Timer8BitSimple::PrescalerBits<0> {
    static constexpr auto values = prescalerValues10Bit<ATMega8::Timer8BitSimple::TCCR>;
};

template<>
struct ATMega8::Timer8BitSimple2::Address<2> {
    static constexpr uint8_t value = 0x45;
};
template<>
struct ATMega8::Timer8BitSimple2::PrescalerBits<2> {
    static constexpr auto values = prescalerValues10BitExtended<ATMega8::Timer8BitSimple2::TCCR>;
};

template<>
struct ATMega8::Timer16Bit::Address<1> {
    static constexpr uint8_t value = 0x46;
};
template<>
struct ATMega8::Timer16Bit::PrescalerBits<1> {
    static constexpr auto values = prescalerValues10Bit<ATMega8::Timer16Bit::TCCRB>;
};

}

#pragma pack(pop)
