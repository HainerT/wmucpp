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

#include "config.h"
#include "mcu/avr/isr.h"
#include "mcu/avr/util.h"
#include "util/bits.h"
#include "container/fixedvector.h"
#include "std/optional.h"
#include "util/disable.h"
#include "hal/event.h"
#include "std/types.h"

enum class AlarmFlags : uint8_t {
    NoTimer =    0,
    Periodic =   1 << 0,
    OneShot  =   1 << 1,
    AutoDelete = 1 << 2,
    Disabled   = 1 << 3
};

namespace std {
template<>
struct enable_bitmask_operators<AlarmFlags>{
    static const bool enable = true;
};
template<>
struct underlying_type<AlarmFlags> {
    typedef uint8_t type;
};
}

template<typename MCUTimer>
class AlarmTimer final {
public:
    AlarmTimer() = delete;

    static void init() {
        constexpr auto t = AVR::Util::calculate<MCUTimer>(Config::Timer::frequency);
        static_assert(t, "falscher wert für p");

        MCUTimer::template prescale<t.prescaler>();
        MCUTimer::template ocra<t.ocr>();
        MCUTimer::mode(AVR::TimerMode::CTC);
        MCUTimer::start();
    }

    static std::optional<uint7_t> create(std::milliseconds millis, AlarmFlags flags){
        Scoped<DisbaleInterrupt> di;
        if (auto index = mTimers.insert({millis / Config::Timer::resolution, millis  / Config::Timer::resolution, flags})) {
            assert(*index <= std::numeric_limits<uint7_t>::max());
            return uint7_t{*index};
        }
        return {};
    }
    static std::optional<uint7_t> create(std::seconds secs, AlarmFlags flags){
        return create(static_cast<std::milliseconds>(secs), flags);
    }

    static void remove(uint7_t id) {
        Scoped<DisbaleInterrupt> di;
        mTimers.removeAt(id);
    }

    static void start(uint7_t id) {
        mTimers[id].flags &= ~AlarmFlags::Disabled;
        mTimers[id].ticksLeft = mTimers[id].ticks;
    }

    static void stop(uint7_t id) {
        mTimers[id].flags |= AlarmFlags::Disabled;
    }
    
    static bool isActive(uint7_t id) {
        return !isset((mTimers[id].flags & AlarmFlags::Disabled));
    }

    static void periodic() {
        using namespace std::literals::chrono;
        for(uint8_t i = 0; i < mTimers.capacity; ++i) {
            if (auto& t = mTimers[i]) {
                if ((--t.ticksLeft == 0) && !isset(t.flags & AlarmFlags::Disabled)) {
                    EventManager::enqueue({EventType::Timer, i});
                    if (isset(t.flags & AlarmFlags::Periodic)) {
                        t.ticksLeft = t.ticks;
                    }
                    if (isset(t.flags & AlarmFlags::OneShot)) {
                        t.flags |= AlarmFlags::Disabled;
                    }
                    if (isset(t.flags & AlarmFlags::AutoDelete)) {
                        t.flags = AlarmFlags::NoTimer;
                    }
                }
            }
        }
    }
    static constexpr auto rateProcess = periodic;
private:
    struct TimerImpl final {
        inline explicit operator bool() {
            return flags != AlarmFlags::NoTimer;
        }
        uint16_t ticks = 0;
        uint16_t ticksLeft = 0;
        AlarmFlags flags = AlarmFlags::NoTimer;
    };

    inline static std::FixedVector<TimerImpl, Config::Timer::NumberOfTimers> mTimers;
};
