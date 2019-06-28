/*
 * WMuCpp - Bare Metal C++ 
 * Copyright (C) 2016, 2017, 2018 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
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

#include <etl/meta.h>

#include "../common/concepts.h"
#include "../common/groups.h"

namespace AVR {
    namespace Portmux {
        namespace detail {
            namespace usart {
                template<typename T, typename MCU = DefaultMcuType>
                struct Mapper {
                    using type = void;    
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Usart<0>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::UsartRoute_t;
                    using type = std::integral_constant<route_t, route_t::usart0_alt1>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Usart<1>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::UsartRoute_t;
                    using type = std::integral_constant<route_t, route_t::usart1_alt1>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Usart<2>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::UsartRoute_t;
                    using type = std::integral_constant<route_t, route_t::usart2_alt1>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Usart<3>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::UsartRoute_t;
                    using type = std::integral_constant<route_t, route_t::usart3_alt1>;
                };
                
            }
            namespace ccl {
                template<typename T, typename MCU = DefaultMcuType>
                struct Mapper {
                    using type = void;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Ccl<0>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::CclRoute_t;
                    using type = std::integral_constant<route_t, route_t::lut0_alt1>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Ccl<1>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::CclRoute_t;
                    using type = std::integral_constant<route_t, route_t::lut1_alt1>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Ccl<2>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::CclRoute_t;
                    using type = std::integral_constant<route_t, route_t::lut2_alt1>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Ccl<3>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::CclRoute_t;
                    using type = std::integral_constant<route_t, route_t::lut3_alt1>;
                };
            }
            namespace tca {
                template<typename T, typename MCU = DefaultMcuType>
                struct Mapper {
                    using type = void;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tca<0>, AVR::Portmux::AltA>, MCU> {
                    using route_t = typename MCU::Portmux::TcaRoute_t;
                    using type = std::integral_constant<route_t, route_t::onA>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tca<0>, AVR::Portmux::AltB>, MCU> {
                    using route_t = typename MCU::Portmux::TcaRoute_t;
                    using type = std::integral_constant<route_t, route_t::onB>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tca<0>, AVR::Portmux::AltC>, MCU> {
                    using route_t = typename MCU::Portmux::TcaRoute_t;
                    using type = std::integral_constant<route_t, route_t::onC>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tca<0>, AVR::Portmux::AltD>, MCU> {
                    using route_t = typename MCU::Portmux::TcaRoute_t;
                    using type = std::integral_constant<route_t, route_t::onD>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tca<0>, AVR::Portmux::AltE>, MCU> {
                    using route_t = typename MCU::Portmux::TcaRoute_t;
                    using type = std::integral_constant<route_t, route_t::onE>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tca<0>, AVR::Portmux::AltF>, MCU> {
                    using route_t = typename MCU::Portmux::TcaRoute_t;
                    using type = std::integral_constant<route_t, route_t::onF>;
                };
                
            }
            namespace tcb {
                template<typename T, typename MCU = DefaultMcuType>
                struct Mapper {
                    using type = void;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tcb<0>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::TcbRoute_t;
                    using type = std::integral_constant<route_t, route_t::tcb0_alt1>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tcb<1>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::TcbRoute_t;
                    using type = std::integral_constant<route_t, route_t::tcb1_alt1>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tcb<2>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::TcbRoute_t;
                    using type = std::integral_constant<route_t, route_t::tcb2_alt1>;
                };
                template<typename MCU>
                struct Mapper<AVR::Portmux::Position<AVR::Component::Tcb<3>, AVR::Portmux::Alt1>, MCU> {
                    using route_t = typename MCU::Portmux::TcbRoute_t;
                    using type = std::integral_constant<route_t, route_t::tcb3_alt1>;
                };
                
            }
        }
        
        template<typename CCList, typename MCU = DefaultMcuType>
        struct StaticMapper final {
        private:
            static constexpr auto mcu_pmux = getBaseAddr<typename MCU::Portmux>;
            using usart_list = Meta::filter<Meta::nonVoid, Meta::transform_type<detail::usart::Mapper, CCList>>;
            using ccl_list = Meta::filter<Meta::nonVoid, Meta::transform_type<detail::ccl::Mapper, CCList>>;
            using tca_list = Meta::filter<Meta::nonVoid, Meta::transform_type<detail::tca::Mapper, CCList>>;
            using tcb_list = Meta::filter<Meta::nonVoid, Meta::transform_type<detail::tcb::Mapper, CCList>>;
            
            static_assert(Meta::size_v<usart_list> <= 4);
            static_assert(Meta::size_v<ccl_list> <= 4);
            static_assert(Meta::size_v<tca_list> <= 1);
            
        public:
            static inline void init() {
                if constexpr(Meta::size_v<usart_list> > 0) {
                    constexpr auto value = Meta::value_or_v<usart_list>;
//                    std::integral_constant<decltype(value), value>::_;
                    mcu_pmux()->usartroutea.template set<value>();                
                }
                if constexpr(Meta::size_v<ccl_list> > 0) {
                    constexpr auto value = Meta::value_or_v<ccl_list>;
//                    std::integral_constant<decltype(value), value>::_;
                    mcu_pmux()->cclroutea.template set<value>();                
                }
                if constexpr(Meta::size_v<tca_list> > 0) {
                    constexpr auto value = Meta::front<tca_list>::value;
//                    tca_list::_;
//                    std::integral_constant<decltype(value), value>::_;
                    mcu_pmux()->tcaroutea.template set<value>();                
                }
                if constexpr(Meta::size_v<tcb_list> > 0) {
                    constexpr auto value = Meta::value_or_v<tcb_list>;
//                    std::integral_constant<decltype(value), value>::_;
                    mcu_pmux()->tcbroutea.template set<value>();                
                }
            }
        };
        
        struct DynamicMapper {
            
        };
        
    }
}
