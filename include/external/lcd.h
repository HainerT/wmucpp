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

#include <cstdint>
#include <utility>

#include "mcu/ports.h"
#include "mcu/avr/delay.h"
#include "util/types.h"
#include "appl/command.h"

namespace LCD { 
    struct Lcd1x8  {};
    struct Lcd1x16 {};
    struct Lcd1x20 {};
    struct Lcd1x40 {};
    struct Lcd2x8  {};
    struct Lcd2x12 {};
    struct Lcd2x16 {};
    struct Lcd2x20 {};
    struct Lcd2x24 {};
    struct Lcd2x40 {};
    struct Lcd4x16 {};
    struct Lcd4x20 {};
    
    template<typename Type>
    struct Parameter;
    
    template<>
    struct Parameter<Lcd2x8> {
        static constexpr uint8_t rows = 2;
        static constexpr uint8_t cols = 8;
        static constexpr uint8_t rowStartAddress[rows] PROGMEM = {0x00, 0x40};
        typedef Splitted_NaN<uint8_t, 1, 3> position_t;
    };
    template<>
    struct Parameter<Lcd2x16> {
        static constexpr uint8_t rows = 2;
        static constexpr uint8_t cols = 16;
        static constexpr uint8_t rowStartAddress[rows] PROGMEM = {0x00, 0x40};
        typedef Splitted_NaN<uint8_t, 1, 4> position_t;
    };
    template<>
    struct Parameter<Lcd2x20> {
        static constexpr uint8_t rows = 2;
        static constexpr uint8_t cols = 20;
        static constexpr uint8_t rowStartAddress[rows] PROGMEM = {0x00, 0x40};
        typedef Splitted_NaN<uint8_t, 1, 5> position_t;
    };

//    enum class Instruction : uint8_t {
//        clear    = (1 << 0),
//        home     = (1 << 1),
        
//        mode     = (1 << 2),
//        shift    = (1 << 0),
//        increment= (1 << 1),
//        decrement= (0 << 1),
        
//        control  = (1 << 3),
//        blink    = (1 << 0),
//        cursorOn = (1 << 1),
//        displayOn= (1 << 2),
        
//        cdshift  = (1 << 4),
//        dshift   = (1 << 3),
//        right    = (1 << 2),
        
//        function = (1 << 5),
//        I8bit    = (1 << 4),
//        I4bit    = (0 << 4),
//        twoLines = (1 << 3),
//        oneLine  = (0 << 3),
//        bigFont  = (1 << 2),
        
//        cgram    = (1 << 6),
//        ddram    = (1 << 7),
//        readBusy = (1 << 7)
//    };
//}

//template<>
//struct std::enable_bitmask_operators<LCD::Instruction> {
//    static constexpr const bool enable = true;    
//};

//namespace LCD {
    struct Row {
        uint8_t value = 0;
    };
    struct Column {
        uint8_t value = 0;
    };
    
    namespace HD44780 {
        namespace Instructions {
            struct Clear : Command::Value<0x01_B> {};
            struct Home : Command::Value<0x02_B>{};
            struct Mode : Command::Value<0x04_B>{};
            struct Control : Command::Value<0x08_B>{};
            struct Shift : Command::Value<0x10_B>{};
            struct Function : Command::Value<0x20_B>{};
            struct CGRam : Command::Value<0x40_B>{};
            struct DDRam : Command::Value<0x80_B>{};
            
            struct ShiftDisplay : Command::Option<Mode, 0x01_B> {};
            struct CursorIncrement : Command::Option<Mode, 0x02_B> {};
            struct CursorDecrement : Command::Option<Mode, 0x00_B> {};
            
            struct Blink : Command::Option<Control, 0x01_B> {};
            struct CursorOn : Command::Option<Control, 0x02_B> {};
            struct DisplayOn : Command::Option<Control, 0x04_B> {};
            
            struct DisplayShift : Command::Option<Shift, 0x08_B> {};
            struct ShiftRight : Command::Option<Shift, 0x04_B> {};
            
            struct I8bit : Command::Option<Function, 0x10_B> {};
            struct I4bit : Command::Option<Function, 0x00_B> {};
            struct TwoLines : Command::Option<Function, 0x08_B> {};
            struct BigFont : Command::Option<Function, 0x04_B> {};
            
            using commands = Command::CommandSet<Meta::List<Clear, Home, Mode, Control, Shift, Function, CGRam, DDRam>, 
                                                 Meta::List<ShiftDisplay, CursorIncrement, CursorDecrement, 
                                                            Blink, CursorOn, DisplayOn,
                                                            DisplayShift, ShiftRight, 
                                                            I8bit, TwoLines, BigFont>>;
        }
    }
    
    template<typename Data, typename RS, typename RW, typename E, typename Type>
    class HD44780Port final {
        HD44780Port() = delete;
        static_assert(Data::size == 4, "wrong number of pins in PinSet");
    public:
        static constexpr auto enableDelay = 20_us;
        
        typedef Parameter<Type> param_type;
        typedef typename Parameter<Type>::position_t position_t;

        
        static void init() {
            Data::template dir<AVR::Output>();
            RS::template dir<AVR::Output>();
            RW::template dir<AVR::Output>();
            E::template dir<AVR::Output>();
            Data::allOff();
            RS::off();
            RW::off();
            E::off();
            Util::delay(16_ms);
            
            using namespace LCD::HD44780::Instructions;
            
//            Data::template set<(uint8_t(Instruction::function | Instruction::I8bit) >> 4)>();
//            Data::set(std::byte(Instruction::function | Instruction::I8bit) >> 4);
            Data::set(UpperNibble::convert(commands::template value<Function, I8bit>()));
            toggle<E>();
            Util::delay(5_ms);
            
//            Data::set(std::byte(Instruction::function | Instruction::I8bit) >> 4);
            Data::set(UpperNibble::convert(commands::template value<Function, I8bit>()));
            toggle<E>();
            Util::delay(1_ms);
            
//            Data::set(std::byte(Instruction::function | Instruction::I8bit) >> 4);
            Data::set(UpperNibble::convert(commands::template value<Function, I8bit>()));
            toggle<E>();
            Util::delay(1_ms);
            
//            Data::set(std::byte(Instruction::function | Instruction::I4bit) >> 4);
            Data::set(UpperNibble::convert(commands::template value<Function, I4bit>()));
            toggle<E>();
            Util::delay(5_ms);
            
            writeCommand<Control, DisplayOn, CursorOn, Blink>();
            writeCommand<Clear>();
            writeCommand<Home>();
            writeCommand<Mode, CursorIncrement>();
            
//            writeCommand(Instruction::control | Instruction::displayOn | Instruction::cursorOn | Instruction::blink);
//            writeCommand(Instruction::clear);
//            writeCommand(Instruction::home);
//            writeCommand(Instruction::mode | Instruction::increment);

            Data::template dir<AVR::Input>();
        }
        static void clear() {
            writeCommand<HD44780::Instructions::Clear>();
//            writeCommand(Instruction::clear);
        }
        static void home() {
            writeCommand<HD44780::Instructions::Home>();
            actualRow = 0;
        }
        static void writeData(std::byte data) {
            waitBusy();
            RS::high();
            write(data);
        }
        template<typename C, typename... OO>
        static void writeCommand() {
            waitBusy();
            RS::low();
            write(HD44780::Instructions::commands::template value<C, OO...>());
        }
//        static void writeCommand(Instruction instruction) {
//            waitBusy();
//            RS::low();
//            write(std::byte(instruction));
//        }
        static void writeAddress(uint8_t a) {
            waitBusy();
            RS::low();
//            write(std::byte(Instruction::ddram) | std::byte(a & 0x7f));
            write(HD44780::Instructions::DDRam::value | std::byte(a & 0x7f));
        }
        static std::byte readData() {
            RS::high();
            return read();
        }
        static std::byte readCommand() {
            RS::low();
            return read();
        }
        static bool put(std::byte c) {
            if (c == std::byte{'\n'}) {
                actualRow = (actualRow + 1) % param_type::rows;
                setPosition(Row{actualRow}, Column{0});
            }
            else {
                auto xy = position();            
                if (xy) {
                    writeData(c);        
                }
                else {
                    return false;
                }
            }
            return true;
        }
        template<uint16_t Size>
        static void put(const volatile std::array<std::byte, Size>& data) {
            setPosition(Row{0}, Column{0});
            auto it = data.begin();
            for(uint8_t row = 0; row < param_type::rows; ++row) {
                for(uint8_t column = 0; column < param_type::cols; ++column) {
                    put(*it++);
                }
                put(std::byte{'\n'});
            }
        }
        
        static position_t position() {
            uint8_t address = std::to_integer<uint8_t>(waitBusy());
            for(uint8_t row = 0; row < Parameter<Type>::rows; ++row) {
                if ((param_type::rowStartAddress[row] <= address) && 
                        (address < (param_type::rowStartAddress[row] + param_type::cols))) {
                    actualRow = row;
                    uint8_t column = address - param_type::rowStartAddress[row];
                    return position_t(row, column);
                }
            }
            return position_t();
        }
        //    static std::optional<std::pair<uint8_t, uint8_t>> position() {
        //        uint8_t address = waitBusy();
        //        for(uint8_t row = 0; row < Parameter<Type>::rows; ++row) {
        //            if ((param_type::rowStartAddress[row] <= address) && 
        //                    (address < (param_type::rowStartAddress[row] + param_type::cols))) {
        //                actualRow = row;
        //                return std::pair<uint8_t, uint8_t>{row, static_cast<uint8_t>(address - param_type::rowStartAddress[row])};
        //            }
        //        }
        //        return{};
        //    }
        static void setPosition(Row row, Column column) {
            actualRow = row.value;
            writeAddress(param_type::rowStartAddress[row.value] + column.value);
        }
        
    private:
        static void write(std::byte data) {
            RW::low();        
            Data::template dir<AVR::Output>();
//            Data::set((data >> 4) & std::byte{0x0f});
            Data::set(UpperNibble::convert(data));
            toggle<E>();
//            Data::set(data & std::byte(0x0f));
            Data::set(LowerNibble::convert(data));
            toggle<E>();
            Data::template dir<AVR::Input>();
        }
        static std::byte read() {
            RW::high();
            Data::template dir<AVR::Input>();
            E::high();
            Util::delay(enableDelay);
            std::byte data = Data::read() << 4;
            E::low();
            Util::delay(enableDelay);
            E::high();
            Util::delay(enableDelay);
            data |= Data::read() & std::byte{0x0f};
            E::low();
            return data;
        }
        static std::byte waitBusy() {
//            while (std::any(readCommand() & std::byte(Instruction::readBusy)));
            while (std::any(readCommand() & 0x80_B));
            Util::delay(4_us);
            return readCommand(); // Address Counter
        }
        template<typename Pin>
        static void toggle() {
            Pin::on();
            Util::delay(enableDelay);
            Pin::off();
        }
        
        inline static uint8_t actualRow = 0;
    };
    
}
