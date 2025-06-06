/*
 * WMuCpp - Bare Metal C++
 * Copyright (C) 2016 - 2025 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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

#include "board.h"

namespace External {
    template<typename OutList, typename PWM, typename StateProvider, typename NVM>
    struct Output {
        inline static constexpr uint8_t size() {
            return Meta::size_v<OutList>;
        }
        static_assert(size() == StateProvider::size());
        
        using pwm_index_t = typename PWM::index_type;
        using index_t = etl::uint_ranged<uint8_t, 0, size() - 1>;
             
//        pwm_index_t::_;
        
        inline static constexpr auto pwmMax = PWM::pwmMax;
        
        using nvm_t = std::remove_cvref_t<decltype(NVM::data())>;
        using nvm_data_t = nvm_t::value_type;
        
        using blinks_type = std::remove_cvref_t<decltype(nvm_data_t{}.blinks())>;
        using blink_index_t = etl::uint_ranged<uint8_t, 0, blinks_type::size()>;
        
        using tick_t = nvm_data_t::tick_t;
        inline static constexpr auto blinkMax = tick_t::max();
        
        static inline void init() {
            PWM::init();
            StateProvider::init();
            Meta::visit<OutList>([]<typename L>(Meta::Wrapper<L>){
                                     L::off();
                                     L::template dir<AVR::Output>();
                                 });
            for (uint8_t out = 0; out < StateProvider::size(); ++out) {
                if (out <= pwm_index_t::Upper) {
                    const auto p = pwm_index_t(out);
                    if (auto v = NVM::data()[out].pwmValue()) {
                        PWM::pwm(p, 0);
                        PWM::on(p);
                    }
                }
            }
        }        
        inline static void allOff() {
            index_t i{};
            while(true) {
                setSwitchOff(i);
                if (i.isTop()) break;
                ++i;
            };
        }
        
        inline static void setSwitchOff(const index_t index) {
            if (index <= pwm_index_t::Upper) {
                const auto li = pwm_index_t(index);
                if (const auto v = NVM::data()[index].pwmValue()) {
                    PWM::pwm(li, 0);
                    PWM::on(li);
                }
                else {
                    PWM::off(li);
                    off(index);
                }
            }
            else {
                off(index);
            }
        }
        inline static void setSwitchOn(const index_t index) {
            if (index <= pwm_index_t::Upper) {
                const auto li = pwm_index_t(index);
                if (const auto v = NVM::data()[index].pwmValue()) {
                    PWM::on(li);
                    PWM::pwm(li, v.toInt());
                }
                else {
                    PWM::off(li);
                    on(index);
                }
            }
            else {
                on(index);
            }    
        }
        
        inline static void setSwitch(const index_t index) {
            if (StateProvider::switches()[index] == StateProvider::SwState::Off) {
                setSwitchOff(index);
//                blinkTicks[index] = NVM::data()[index].blinks()[blinkIndex].intervall;
            }
            else if ((StateProvider::switches()[index] == StateProvider::SwState::Blink1)) {
                const blink_index_t blinkIndex((StateProvider::switches()[index] == StateProvider::SwState::Blink1) ? 0 : 1);
                                
                if (NVM::data()[index].blinks()[blinkIndex].duration) {
                    blinkTicks[index].match(NVM::data()[index].blinks()[blinkIndex].duration, [&]{
                        setSwitchOff(index);
                    });
                    blinkTicks[index].on(NVM::data()[index].blinks()[blinkIndex].intervall, [&]{
                        setSwitchOn(index);
                    });
                    ++blinkTicks[index];
                }
                else {
                    setSwitchOn(index);
                }
            }
            else if ((StateProvider::switches()[index] == StateProvider::SwState::Blink2)) {
                const blink_index_t blinkIndex((StateProvider::switches()[index] == StateProvider::SwState::Blink1) ? 0 : 1);
                                
                if (NVM::data()[index].blinks()[blinkIndex].duration) {
                    static uint8_t b{0};
                    if (b == 1) {
                        blinkTicks[index].match(NVM::data()[index].blinks()[blinkIndex].duration / 4, [&]{
                            setSwitchOff(index);
                            ++b;
                        });
                    }
                    else if (b == 2) {
                        blinkTicks[index].match(NVM::data()[index].blinks()[blinkIndex].duration / 2, [&]{
                            setSwitchOn(index);
                            ++b;
                        });
                    }
                    else if (b == 3) {
                        uint16_t t = NVM::data()[index].blinks()[blinkIndex].duration.value;
                        t = (t * 3) / 4;
                        auto tt = tick_t::fromRaw(t);
                        blinkTicks[index].match(tt, [&]{
                            setSwitchOff(index);
                            ++b;
                        });
                    }
                    blinkTicks[index].on(NVM::data()[index].blinks()[blinkIndex].intervall, [&]{
                        setSwitchOn(index);
                        b = 1;
                    });
                    ++blinkTicks[index];
                }
                else {
                    setSwitchOn(index);
                }
            }
            else if (StateProvider::switches()[index] == StateProvider::SwState::Steady) {
                setSwitchOn(index);                
            }
        }
        inline static void setSwitches() {
            for(uint8_t s = 0; s <= index_t::Upper; ++s) {
                setSwitch(index_t{s});
            }        
        }
        
        inline static void pwm(const index_t index, const uint8_t p) {
            NVM::data()[index].pwmValue(p);
            NVM::data().change();
            if (index <= pwm_index_t::Upper) {
                const auto li = pwm_index_t(index);
                if (const auto v = NVM::data()[index].pwmValue()) {
                    PWM::pwm(li, v.toInt());
                }
            }
        }
        
        inline static void reset(const index_t index) {
            NVM::data()[index] = nvm_data_t{};
            NVM::data().change();
        }
        inline static void duration(const index_t index, const tick_t d, const blink_index_t blinkIndex) {
            if (d < NVM::data()[index].blinks()[blinkIndex].intervall) {
                if (blinkIndex == 0) {
                    if (d.value >= 1) {
                        NVM::data()[index].blinks()[blinkIndex].duration = d;
                    }
                    else {
                        NVM::data()[index].blinks()[blinkIndex].duration.value.set(1);                
                    }
                }
                else if (blinkIndex == 1) {
                    if (d.value >= 4) {
                        NVM::data()[index].blinks()[blinkIndex].duration = d;                
                    }
                    else {
                        NVM::data()[index].blinks()[blinkIndex].duration.value.set(4);                
                    }
                }
                NVM::data().change();
//                blinkTicks[index].value.set(0);                
            }
        }
        inline static void intervall(const index_t index, const tick_t i, const blink_index_t blinkIndex) {
            NVM::data()[index].blinks()[blinkIndex].intervall = i;
            NVM::data().change();
            blinkTicks[index] = std::min(blinkTicks[index], i);
        }
        inline static void intervall2(const index_t index, const tick_t i, const blink_index_t blinkIndex) {
            static tick_t lastValue{};
            if (lastValue != i) {
                lastValue = i;
                NVM::data()[index].blinks()[blinkIndex].intervall = i;
                NVM::data().change();
            }
        }
        inline static auto intervall(const index_t index, const blink_index_t blinkIndex) {
            return NVM::data()[index].blinks()[blinkIndex].intervall;
        }
    private:
        inline static void on(const index_t index) {
            Meta::visitAt<OutList>(index, []<typename L>(Meta::Wrapper<L>){
                                       L::on();
                                   });
        }
        inline static void off(const index_t index) {
            Meta::visitAt<OutList>(index, []<typename L>(Meta::Wrapper<L>){
                                       L::off();
                                   });
        }
        inline static std::array<Storage::tick_type, size()> blinkTicks;
    };
}

