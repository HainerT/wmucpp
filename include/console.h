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

#include "config.h"
#include "util/util.h"
#include "container/pgmstring.h"

namespace std {

template<typename DeviceType>
struct basic_ostream final {
    typedef DeviceType device_type;
};

template<typename Type>
struct basic_iomanip {
    typedef Type iomanip_type;
};

struct CRLF{};
struct LF{};
struct CR{};

template <typename Mode>
struct lineTerminator final : public basic_iomanip<lineTerminator<Mode>> {};

}

template<typename Stream>
Stream& operator<<(Stream& o, const char* str) {
    if (!Config::disableCout) {
        Util::put<typename Stream::device_type, Config::ensureTerminalOutput>(str);
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, char c) {
    if (!Config::disableCout) {
        Util::put<typename Stream::device_type, Config::ensureTerminalOutput>(c);
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, uint8_t v) {
    if (!Config::disableCout) {
        std::array<char, 4> buffer;
        Util::utoa(v, buffer);
        Util::put<typename Stream::device_type, Config::ensureTerminalOutput>(&buffer[0]);
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, int8_t v) {
    if (!Config::disableCout) {
        std::array<char, 5> buffer;
        Util::itoa(v, buffer);
        Util::put<typename Stream::device_type, Config::ensureTerminalOutput>(&buffer[0]);
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, uint16_t v) {
    if (!Config::disableCout) {
        std::array<char, 6> buffer;
        Util::utoa(v, buffer);
        Util::put<typename Stream::device_type, Config::ensureTerminalOutput>(&buffer[0]);
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, uint32_t v) {
    if (!Config::disableCout) {
        std::array<char, 12> buffer;
        Util::utoa(v, buffer);
        Util::put<typename Stream::device_type, Config::ensureTerminalOutput>(&buffer[0]);
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, const std::lineTerminator<std::CRLF>&) {
    if (!Config::disableCout) {
        return o << "\r\n";
    }
    return o;
}
template<typename Stream>
Stream& operator<<(Stream& o, const std::lineTerminator<std::CR>&) {
    if (!Config::disableCout) {
        return o << "\r";
    }
    return o;
}
template<typename Stream>
Stream& operator<<(Stream& o, const std::lineTerminator<std::LF>&) {
    if (!Config::disableCout) {
        return o << "\n";
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, const std::hertz& f) {
    if (!Config::disableCout) {
        return o << f.value << "Hz";
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, const std::megahertz& f) {
    if (!Config::disableCout) {
        return o << f.value << "MHz";
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, const std::milliseconds& d) {
    if (!Config::disableCout) {
        return o << d.value << "ms";
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, const std::microseconds& d) {
    if (!Config::disableCout) {
        return o << d.value << "us";
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, const std::DeciCelsius& t) {
    if (!Config::disableCout) {
        return o << t.degrees << "."_pgm << t.milliDegree << "°C";
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, bool b) {
    if (!Config::disableCout) {
        if (b) {
            return o << "true";
        }
        else {
            return o << "false";
        }
    }
    return o;
}

template<typename Stream>
Stream& operator<<(Stream& o, const Config&) {
    if (!Config::disableCout) {
        o << "fMcu: "_pgm << Config::fMcu << "\r\n"_pgm;
        o << "fMcuMhZ: "_pgm << Config::fMcuMhz << "\r\n"_pgm;
        o << "Timer::NumberOfTimers: "_pgm << Config::Timer::NumberOfTimers << "\r\n"_pgm;
        o << "Timer::frequency: "_pgm << Config::Timer::frequency << "\r\n"_pgm;
        o << "Timer::resolution: "_pgm << Config::Timer::resolution << "\r\n"_pgm;
        o << "EventManager::EventQueueLength: "_pgm << Config::EventManager::EventQueueLength << "\r\n"_pgm;
        o << "Usart::SendQueueLength: "_pgm << Config::Usart::SendQueueLength << "\r\n"_pgm;
        o << "Usart::ReceiveQueueLength: "_pgm << Config::Usart::RecvQueueLength << "\r\n"_pgm;
        o << "ensureTerminalOutput: "_pgm << Config::ensureTerminalOutput << "\r\n"_pgm;
        o << "disableCout: "_pgm << Config::disableCout << "\r\n"_pgm;
        o << "SoftSpiMaster::pulseDelay: "_pgm << Config::SoftSpiMaster::pulseDelay << "\r\n"_pgm;
        o << "Button::buttonTicksForPressed: "_pgm << Config::Button::buttonTicksForPressed << "\r\n"_pgm;
    }
    return o;
}

template<typename Stream, typename C, C... CC>
Stream& operator<<(Stream& out, const PgmString<C, CC...>& s) {
    const char * ptr = s.data;
    while (char c = pgm_read_byte(ptr++)) {
        out << c;
    };
    return out;
}

template<typename Stream>
Stream& operator<<(Stream& out, const PgmStringView& s) {
    const char * ptr = s.ptrToPgmData;
    while (char c = pgm_read_byte(ptr++)) {
        out << c;
    };
    return out;
}
