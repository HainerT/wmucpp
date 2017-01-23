/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2013, 2014, 2015, 2016, 2017 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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
#include "util/disable.h"
#include "units/duration.h"
#include "sumdprotocoll.h"

namespace Hott {

template<uint8_t M>
class SumDProtocollAdapter final {
    enum sumdstate {Undefined = 0, Start1, StartNormal, StartFailSafe, ChannelDataL, ChannelDataH, CrcL, CrcH, NumberOfStates};
    typedef enum sumdstate sumdstate_t;
    template<int N, typename PA> friend class AVR::Usart;

public:
    SumDProtocollAdapter() = delete;

    static uint16_t value(uint8_t channel) {
        Scoped<DisbaleInterrupt> di;
        return std::combinedValue(msg().channelData[channel]);
    }
    static uint8_t value8Bit(uint8_t channel) {
        return msg().channelData[channel].first;
    }
    static uint8_t numberOfChannels() {
        return msg().nChannels;
    }
private:
    inline static bool process(uint8_t c) { // from isr only
        static sumdstate state = sumdstate::Undefined;
        static uint8_t channel = 0;

        switch (state) {
        case sumdstate::Undefined:
            if (c == 0xa8) {
                state = sumdstate::Start1;
            }
            else {
                state = sumdstate::Undefined;
            }
            break;
        case sumdstate::Start1:
            if (c == 0x01) {
                state = sumdstate::StartNormal;
            }
            else if (c == 0x81) {
                state = sumdstate::StartFailSafe;
            }
            else {
                state = sumdstate::Undefined;
            }
            break;
        case sumdstate::StartNormal:
            msg().nChannels = c;
            state = sumdstate::ChannelDataH;
            break;
        case sumdstate::StartFailSafe:
            msg().nChannels = c;
            state = sumdstate::ChannelDataH;
            break;
        case sumdstate::ChannelDataH:
            msg().channelData[channel].first = c;
            state = sumdstate::ChannelDataL;
            break;
        case sumdstate::ChannelDataL:
            msg().channelData[channel].second = c;
            state = sumdstate::ChannelDataL;
            ++channel;
            if (channel < msg().nChannels) {
                state = sumdstate::ChannelDataH;
            }
            else {
                state = sumdstate::CrcH;
                channel = 0;
            }
            break;
        case sumdstate::CrcH:
            msg().crc = c << 8;
            state = sumdstate::CrcL;
            break;
        case sumdstate::CrcL:
            msg().crc |= c;
            state = sumdstate::Undefined;
            break;
        default:
            assert(false);
            break;
        }
        return true;
    }
    static volatile SumDMsg& msg() {
        static volatile SumDMsg data;
        return data;
    }
};

}