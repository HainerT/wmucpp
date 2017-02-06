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

#include "config.h"
#include "mcu/avr8.h"
#include "mcu/avr/mcutimer.h"
#include "mcu/avr/isr.h"
#include "mcu/atomic.h"
#include "util/bits.h"
#include "util/disable.h"
#include "container/fifo.h"

enum class EventType : uint8_t {
    NoEvent,
    Test,
    Timer,
    UsartRecv0, UsartRecv1, UsartRecv2,
    UsartFe, // UsartFe1, UsartFe2,
    UsartUpe, // UsartUpe1, UsartUpe2,
    UsartDor, // UsartDor1, UsartDor2,
    SwUsartRecv0, SwUsartRecv1,
    Spi0, Spi1,
    HottBinaryRequest, HottAsciiRequest, HottSensorBroadcast, HottAsciiKey,
    Ppm1Up, Ppm1Down, Ppm2Up, Ppm2Down,
    ButtonPress,
    ButtonPress0, ButtonPress1, ButtonPress2, ButtonPress3, ButtonPress4, ButtonPress5, ButtonPress6, ButtonPress7,
    OneWireRecvComplete,
    DS18B20Measurement, DS18B20Error,
    TWIRecvComplete, TWISendComplete, TWIError,
    DS1307TimeAvailable, DS1307Error,
    I2CRamError, I2CRamValueAvailable,
    I2CLedError, I2CLedValueAvailable,
    I2CRpmError, I2CRpmValueAvailable,
    AdcConversion,
    DCFReceive0, DCFReceive1, DCFSync, DCFParityError, DCFError,
    TLE5205Error,
    ExternalInterrupt,
    NullPAEvent
};

template<typename T>
struct Event final {
    Event() = default;
    Event(const volatile Event& e) : type(e.type), value(e.value) {}
    Event(EventType t, T v) : type(t), value(v) {}
    void operator=(const volatile Event& e) volatile {
        type = e.type;
        value = e.value;
    }
    EventType type;
    T value;
};

typedef Event<uint8_t> Event8u_t;

namespace AVR {
template<uint8_t N, typename PA, typename MCU> class Usart;
template<uint8_t N, typename MCU> class Spi;
template<uint8_t N, typename MCU> class SWUsart;
}

namespace Hott {
template<uint8_t N> class SensorProtocollAdapter;
}

template<typename Interrupt = void, typename... PP>
class 
//        [[deprecated]] 
        PeriodicGroup : public IsrBaseHandler<Interrupt> {
public:
    static void periodic() {
        if (tickCounter > 0) {
            --tickCounter;
            (PP::periodic(),...); // fold
        }
    }
    static void isr() {
        static_assert(Interrupt::number >= 0, "wrong interrupt number");
        ++tickCounter;
    }
private:
    static volatile uint8_t tickCounter;
};
template<typename Int, typename... PP>
volatile uint8_t PeriodicGroup<Int, PP...>::tickCounter = 0;

template<typename... EE>
class EventHandlerGroup {
    template<int N, typename T, typename... TT>
    class Processor final {
    public:
        static bool process(const Event8u_t& e) {
            bool processed = false;
            if (e.type == T::eventType) {
                processed = T::process(e.value) || processed;
            }
            if constexpr((N - 1) > 0) {
                processed = Processor<N - 1, TT..., void>::process(e) || processed;
            }
            return processed;
        }
    };
public:
    static bool process(const Event8u_t& event) {
        if constexpr(sizeof... (EE) > 0) {
            return Processor<sizeof...(EE), EE...>::process(event);
        }
        else {
            return false;
        }
    }
    static constexpr uint8_t numberOfEvents = sizeof...(EE);
    static constexpr EventType events[] = {EE::eventType...};
    static constexpr bool uniqueEvents = [](){
        for(uint8_t i = 0; i < (numberOfEvents - 1); ++i) {
            for(uint8_t k = (i + 1); k < numberOfEvents; ++k) {
                if (events[i] == events[k]) {
                    return false;
                }
            }
        }
        return true;
    }();
 };

class EventManager final
{
    template<uint8_t> friend class Hott::SensorProtocollAdapter;
    template<uint8_t N, typename PA, typename MCU> friend class AVR::Usart;
    template<uint8_t N, typename MCU> friend class AVR::Spi;
    template<uint8_t N, typename MCU> friend class SWUsart;
public:
    EventManager() = delete;
    static bool enqueue(const Event8u_t& event) {
        if (!fifo().push_back(event)) { // lockfree fifo
            leakedEvent() = true;
            return false;
        }
        return true;
    }

    template<typename EE, typename P>
    static void run2(const P& periodic) {
        while(true) {
            periodic();
            if (auto event = fifo().pop_front()) {
                if (!EE::process(*event)) {
                    unprocessedEvent() = true;
                }
            }
        }
    }
    template<typename PP, typename EE, typename P>
//    [[deprecated]] 
    static void run(const P& periodic) {
        while(true) {
            PP::periodic();
            periodic();
            if (auto event = fifo().pop_front()) {
                if (!EE::process(*event)) {
                    unprocessedEvent() = true;
                }
            }
        }
    }
    template<typename PP, typename EE>
    static void run() {
        while(true) {
            PP::periodic();
            if (auto event = fifo().pop_front()) {
                if (!EE::process(*event)) {
                    unprocessedEvent() = true;
                }
            }
        }
    }
    static bool& unprocessedEvent() {
        static bool unprocessed = false;
        return unprocessed;
    }
    static bool& leakedEvent() {
        static bool leaked = false;
        return leaked;
    }
private:
    static bool enqueueISR(const Event8u_t& event) {
        if (!fifo().push_back(event)) {
            leakedEvent() = true;
            return false;
        }
        return true;
    }
    // header only: to avoid static data member
    static volatile std::FiFo<Event8u_t, Config::EventManager::EventQueueLength>& fifo()  {
        static volatile std::FiFo<Event8u_t, Config::EventManager::EventQueueLength> fifo;
        return fifo;
    }
};

template<EventType Type>
struct EventHandler {
    EventHandler() = delete;
    friend class EventManager;
    static constexpr EventType eventType = Type;
};
