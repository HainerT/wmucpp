#pragma once

#include <type_traits>
#include <concepts>
#include <chrono>

#include "units.h"
#include "concepts.h"
#include "mcu/mcu_traits.h"

namespace Mcu::Stm {
    using namespace Units::literals;
    
    struct HSI;
    
    template<Units::megahertz Freq, Units::hertz SysTickFreq = 1000_Hz, typename ClockSource = HSI, typename MCU = DefaultMcu>
    struct ClockConfig;

    template<Units::hertz SysTickFreq, G4xx MCU>
    struct ClockConfig<170_MHz, SysTickFreq, HSI, MCU> {
        static inline constexpr uint8_t pllM{4 - 1}; // 4
        static inline constexpr uint8_t pllN{85}; // 85
        static inline constexpr uint8_t pllR{0}; // 2
        static inline constexpr uint8_t pllP{2};
        static inline constexpr uint8_t pllQ{2};
        
        static inline constexpr Units::megahertz f{170_MHz};
        static inline constexpr Units::hertz frequency = f;
        static inline constexpr uint32_t systick{f / SysTickFreq}; 
//        std::integral_constant<uint32_t, systick>::_;
        
        static inline constexpr std::chrono::microseconds systickIntervall{1'000'000 / SysTickFreq.value};
    };
    template<Units::hertz SysTickFreq, G4xx MCU>
    struct ClockConfig<60_MHz, SysTickFreq, HSI, MCU> {
        static inline constexpr uint8_t pllM{4 - 1}; // 4
        static inline constexpr uint8_t pllN{30};
        static inline constexpr uint8_t pllR{0}; // 2
        static inline constexpr uint8_t pllP{2};
        static inline constexpr uint8_t pllQ{2};

        static inline constexpr Units::megahertz f{60_MHz};
        static inline constexpr Units::hertz frequency = f;
        static inline constexpr uint32_t systick{f / SysTickFreq};
        //        std::integral_constant<uint32_t, systick>::_;

        static inline constexpr std::chrono::microseconds systickIntervall{1'000'000 / SysTickFreq.value};
    };

    template<Units::hertz SysTickFreq, G0xx MCU>
    struct ClockConfig<64_MHz, SysTickFreq, HSI, MCU> {
        static inline constexpr uint8_t pllM{0}; // 1
        static inline constexpr uint8_t pllN{8}; // 8
        static inline constexpr uint8_t pllR{1}; // 2
        static inline constexpr uint8_t pllP{1}; // 2

        static inline constexpr Units::megahertz f{64_MHz};
        static inline constexpr Units::hertz frequency = f;
        static inline constexpr uint32_t systick{f / SysTickFreq};
               // std::integral_constant<uint32_t, systick>::_;

        static inline constexpr std::chrono::microseconds systickIntervall{1'000'000 / SysTickFreq.value};
    };

    template<typename Config, typename MCU = DefaultMcu> struct Clock;

    template<typename Config, Mcu::Stm::G4xx MCU>
    struct Clock<Config, MCU> {
        using config = Config;
#ifdef STM32G4
        static inline void init() {
            RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;            
            RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
            
            RCC->CFGR |= RCC_CFGR_HPRE_DIV2;         
            
//            FLASH->ACR |= FLASH_ACR_LATENCY_5WS 
            FLASH->ACR |= FLASH_ACR_LATENCY_4WS // sollte laut DB reichen
                          | FLASH_ACR_ICEN 
                          | FLASH_ACR_DCEN 
                          | FLASH_ACR_PRFTEN;
            
            PWR->CR5 &= PWR_CR5_R1MODE;
            PWR->CR1 |= PWR_CR1_VOS_0;
            
            RCC->CR |= RCC_CR_HSION;
            while (!(RCC->CR & RCC_CR_HSIRDY));
            
            RCC->PLLCFGR = (Config::pllM << RCC_PLLCFGR_PLLM_Pos) 
                            | (Config::pllR << RCC_PLLCFGR_PLLR_Pos)
                            | (Config::pllN << RCC_PLLCFGR_PLLN_Pos)
                            | (Config::pllP << RCC_PLLCFGR_PLLPDIV_Pos)
                            | (Config::pllQ << RCC_PLLCFGR_PLLQ_Pos)
                            | RCC_PLLCFGR_PLLSRC_HSI | RCC_PLLCFGR_PLLREN;
            
            RCC->CR |= RCC_CR_PLLON;
            while (!(RCC->CR & RCC_CR_PLLRDY));
            
            RCC->CFGR |= RCC_CFGR_SW_PLL;
            
            while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_PLL);

            MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE_Msk, RCC_CFGR_HPRE_DIV1);            
            MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1_Msk, RCC_CFGR_PPRE1_DIV1);            
            MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2_Msk, RCC_CFGR_PPRE2_DIV1);            
        }
#endif
    };

    template<typename Config, Mcu::Stm::G0xx MCU>
    struct Clock<Config, MCU> {
        using config = Config;
#ifdef STM32G0
        static inline void init() {
            RCC->APBENR2 |= RCC_APBENR2_SYSCFGEN;
            RCC->APBENR1 |= RCC_APBENR1_PWREN;

            RCC->CFGR |= RCC_CFGR_HPRE_3;

            FLASH->ACR |= FLASH_ACR_LATENCY_1 // 2WS sollte laut DB reichen
                          | FLASH_ACR_ICEN
                          // | FLASH_ACR_DCEN
                          | FLASH_ACR_PRFTEN;

            // PWR->CR5 &= PWR_CR5_R1MODE;
            PWR->CR1 |= PWR_CR1_VOS_0;

            RCC->CR |= RCC_CR_HSION;
            while (!(RCC->CR & RCC_CR_HSIRDY));

            RCC->CR &= ~RCC_CR_PLLON;
            while ((RCC->CR & RCC_CR_PLLRDY));

            RCC->PLLCFGR = (Config::pllM << RCC_PLLCFGR_PLLM_Pos)
                            | (Config::pllR << RCC_PLLCFGR_PLLR_Pos)
                            | (Config::pllN << RCC_PLLCFGR_PLLN_Pos)
                            | (Config::pllP << RCC_PLLCFGR_PLLP_Pos)
                            // | (Config::pllQ << RCC_PLLCFGR_PLLQ_Pos)
                            | RCC_PLLCFGR_PLLSRC_HSI | RCC_PLLCFGR_PLLREN;

            RCC->CR |= RCC_CR_PLLON;
            while (!(RCC->CR & RCC_CR_PLLRDY));

            RCC->CFGR |= RCC_CFGR_SW_1; // PLL
            while ((RCC->CFGR & RCC_CFGR_SWS_Msk) != RCC_CFGR_SWS_1);

            MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE_Msk, 0x00);
            MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE_Msk, 0x00);
            // MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2_Msk, RCC_CFGR_PPRE2_DIV1);
        }
#endif
    };

    template<typename Clock,  typename UseInterrupts = std::false_type, typename MCU = void> struct SystemTimer;

    template<typename Clock, Concept::Flag UseInterrupts, typename MCU>
    requires(Mcu::Stm::G4xx<MCU> || Mcu::Stm::G0xx<MCU>)
    struct SystemTimer<Clock, UseInterrupts, MCU> {
        static inline constexpr std::chrono::microseconds intervall = Clock::config::systickIntervall;
        
        static inline constexpr Units::hertz frequency{1'000'000 / intervall.count()};
        
        inline static void init() {
            SysTick->LOAD = Clock::config::systick;
            SysTick->CTRL |= (1 << SysTick_CTRL_ENABLE_Pos) 
                             | (1 << SysTick_CTRL_CLKSOURCE_Pos);

            if constexpr(UseInterrupts::value) {
                SysTick->CTRL |= (1 << SysTick_CTRL_TICKINT_Pos);                
            }
        }
        template<typename F = UseInterrupts, typename = std::enable_if_t<!F::value>>
        inline static void periodic(const auto f) {
            if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) {
                ++mValue;
                f();
            }
        }
        template<typename F = UseInterrupts, typename = std::enable_if_t<F::value>>
        inline static void 
        isr() {
            ++mValue;
        }
        inline static void test() {
            periodic();
        }
    private:
//        inline static Interrupt::volatile_atomic<uint32_t> mValue{0};
        inline static uint32_t mValue{0};
    public:
        inline static volatile auto& value{mValue};
    };
}

