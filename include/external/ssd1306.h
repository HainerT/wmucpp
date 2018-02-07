#pragma once

#include "mcu/avr/twimaster.h"
#include "mcu/avr/spi.h"
#include "appl/fonts.h"
#include "container/pgmarray.h"
#include <algorithm>

namespace detail {
    namespace SSD1306 {
        template<typename = void>
        struct Base {
            inline static constexpr auto reset_puls_width = 5_us;
            
            inline static constexpr std::byte DispOff = 0xAE_B;
            inline static constexpr std::byte DispOn = 0xAF_B;
            
            inline static constexpr std::byte White = 0x01_B;
            inline static constexpr std::byte Black = 0x00_B;
            
            inline static constexpr uint16_t Width  = 128;
            inline static constexpr uint16_t Height = 64;
            inline static constexpr uint16_t Size = Width * Height / 8;
            
            using font = Font<6,8>;
            struct InitGenerator {
                inline static constexpr auto data = std::make_array<std::byte>(	// Initialization Sequence
                                                                                0xAE_B,		// Display OFF (sleep mode)
                                                                                0x20_B, 0x00_B,		// Set Memory Addressing Mode
                                                                                // 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
                                                                                // 10=Page Addressing Mode (RESET); 11=Invalid
                                                                                0xB0_B,			// Set Page Start Address for Page Addressing Mode, 0-7
                                                                                0xC8_B,			// Set COM Output Scan Direction
                                                                                0x00_B,			// --set low column address
                                                                                0x10_B,			// --set high column address
                                                                                0x40_B,			// --set start line address
                                                                                0x81_B, 0xCF_B,		// Set contrast control register
                                                                                0xA1_B,			// Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
                                                                                0xA6_B,			// Set display mode. A6=Normal; A7=Inverse
                                                                                0xA8_B, 0x3F_B,		// Set multiplex ratio(1 to 64)
                                                                                0xA4_B,			// Output RAM to Display
                                                                                // 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
                                                                                0xD3_B, 0x00_B,		// Set display offset. 00 = no offset
                                                                                0xD5_B,			// --set display clock divide ratio/oscillator frequency
                                                                                0xF0_B,			// --set divide ratio
                                                                                0xD9_B, 0x22_B,		// Set pre-charge period
                                                                                0xDA_B, 0x12_B,		// Set com pins hardware configuration
                                                                                0xDB_B,			// --set vcomh
                                                                                0x20_B,			// 0x20,0.77xVcc
                                                                                0x8D_B, 0x14_B,		// Set DC-DC enable
                                                                                0xAF_B
                                                                                );
                constexpr auto operator()() {
                    return data;
                }
            };
            inline static constexpr typename ::Util::Pgm::Converter<InitGenerator>::pgm_type init_sequence{}; 
        };
        struct SPI {};
        struct TWI {};
        template<typename Dev, typename DCPin, typename ResetPin = void>
        struct SpiEndpoint {
            typedef SPI type;
            typedef Dev dev_type;
            typedef ResetPin reset_type;
            typedef DCPin dc_type;
        };
        template<typename Dev, const ::TWI::Address& Address>
        struct TwiEndpoint {
            typedef TWI type;
            typedef Dev TWIMaster;
            inline static constexpr auto address = Address;
        };
        template<typename E>
        concept bool Spi() {
            return std::is_same<typename E::type, detail::SSD1306::SPI>::value;
        }
        template<typename E>
        concept bool Twi() {
            return std::is_same<typename E::type, detail::SSD1306::TWI>::value;
        }
    }
}

template<typename E>
class SSD1306;

template<detail::SSD1306::Spi EndPoint>
class SSD1306<EndPoint> : public detail::SSD1306::Base<> {
    using spi = typename EndPoint::dev_type;
    using reset = typename EndPoint::reset_type;
    using dc = typename EndPoint::dc_type;
    inline static void send_command(std::byte b) {
        dc::low();
        spi::template put<true>(b);       
        spi::off();
    }
    inline static void send_data(std::byte b) {
        dc::high();
        spi::template put<true>(b);       
        spi::off();
    }
public:
    inline static void init() {
        dc::template dir<AVR::Output>();
        dc::high();
        spi::init();
        if constexpr(!std::is_same<reset, void>::value) {
            reset::template dir<AVR::Output>();
            reset::high();
            reset::low();
            Util::delay(reset_puls_width);
            reset::high();
        }
        for(auto v : init_sequence) {
            send_command(v);
        }
    }
    inline static void put(char c) {
        for(auto v : font()[c]) {
            send_data(v);
        }
    }
    inline static void put(const PgmStringView& string) {
        for(uint8_t i = 0, c = 0; (c = string[i]) != '\0'; ++i) {
            put(c);
        };   
    }
    inline static void clear() {
        for(uint8_t page = 0; page < 8; ++page) {
            send_command(std::byte{0xb0 + page});
            for(uint8_t column = 0; column < 128; ++column) {
                send_data(0x00_B);
            }
        }
    }
    inline static void image() {
        uint16_t index = 0;
        for(uint8_t page = 0; page < 8; ++page) {
            send_command(std::byte{0xb0 + page});
            for(uint8_t column = 0; column < 128; ++column) {
                send_data(std::byte{image1[index++]});
            }
        }
    }
private:
    struct ImageGenerator {
//        inline static constexpr std::array<uint8_t, 1024> data{};
        inline static constexpr auto data = std::make_array<uint8_t>(
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
                    0x55,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
                    0xaa,0xaa,0xaa,0x01,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x01,0x00,0x00,0x40,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x01,0x00,0xc0,0x06,0x00,0x00,0x80,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x20,0x08,0x00,0x00,0x40,0x55,0x55,
                    0x55,0x55,0x55,0x55,0x55,0x55,0xd5,0x00,0x00,0x10,0x10,0x00,0x00,0x80,0xaa,
                    0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0x00,0x00,0x10,0x10,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x15,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x10,
                    0x00,0x00,0x00,0x00,0x00,0xf8,0xf5,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x10,
                    0x10,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x0e,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x20,0x08,0x00,0x00,0x00,0x00,0x80,0x01,0x00,0x30,0x00,0x00,0x00,0x00,0x00,
                    0x00,0xc0,0x06,0x00,0x00,0x00,0x00,0x60,0x00,0x00,0xc0,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x01,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x06,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x08,
                    0x00,0x00,0x00,0x15,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
                    0x10,0x00,0x00,0x80,0x75,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
                    0x00,0x10,0x00,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,
                    0x00,0x00,0x20,0x00,0x00,0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,
                    0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x20,
                    0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x20,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x80,0x40,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x21,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x40,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x20,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x80,0x20,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x41,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x80,
                    0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,
                    0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x02,
                    0x00,0x80,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,
                    0x02,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,
                    0x00,0x02,0x00,0x80,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,
                    0x00,0x00,0x02,0x00,0x80,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,
                    0x00,0x00,0x00,0x02,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,
                    0x00,0x00,0x00,0x00,0x02,0x00,0x80,0x21,0x00,0x00,0x00,0x00,0x00,0x00,0x08,
                    0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x80,0x20,0x00,0x00,0x04,0x00,0x00,
                    0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x40,0x00,0x00,0x3f,0x00,
                    0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x20,0x00,0x80,0x40,
                    0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x20,0x00,0x40,
                    0x40,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x41,0x00,
                    0x40,0x80,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x80,0x20,
                    0x00,0x40,0x80,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x80,
                    0x21,0x00,0x20,0x00,0x01,0x00,0x00,0xc0,0x00,0x00,0x00,0x00,0x20,0x00,0x00,
                    0x00,0x41,0x00,0x40,0x80,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x10,0x00,
                    0x00,0x80,0x20,0x00,0x60,0x80,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x08,
                    0x00,0x00,0x80,0x20,0x00,0x40,0x80,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
                    0x04,0x00,0x00,0x80,0x40,0x00,0x80,0x40,0x00,0x00,0x00,0x00,0x04,0x00,0x00,
                    0x00,0x02,0x00,0x00,0x00,0x21,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x10,0x00,
                    0x00,0x00,0x01,0x00,0x00,0x80,0x20,0x00,0x00,0x36,0x00,0x00,0x00,0x00,0x60,
                    0x00,0x00,0xc0,0x00,0x00,0x00,0x80,0x41,0x00,0x00,0x08,0x00,0x00,0x00,0x00,
                    0x80,0x01,0x00,0x30,0x00,0x00,0x00,0x00,0x35,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x0e,0x00,0x0f,0x00,0x00,0x00,0x00,0x15,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0xf8,0xfb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                    0x00,0x00,0x00,0x00
                    );
        
        inline static constexpr bool getPixelXBM(uint8_t x, uint8_t y, const std::array<uint8_t, 1024>& xbm) {
            uint16_t b = (x / 8) + (y * 16);
            uint8_t x1 = (x % 8) ;
            uint8_t  mask = (1 << x1);
            if (xbm[b] & mask) {
                return true;
            }
            return false;
        }
        
        inline static constexpr void setPixelSSD(uint8_t x, uint8_t y, std::array<uint8_t, 1024>& ssd) {
            uint16_t b = (x % 128) + (y / 8) * 128;
            uint8_t y1 = y % 8;
            uint8_t mask = (1 << y1);
            ssd[b] |= mask;
        }
        
        inline static constexpr auto data1 = []{
            std::array<uint8_t, 1024> d;
            for(uint8_t x = 0; x < 128; ++x) {
                for(uint8_t y = 0; y < 64; ++y) {
                    if (getPixelXBM(x, y, data)) {
                        setPixelSSD(x, y, d);
                    }
                }
            }
            return d;
        }();
        
        constexpr auto operator()() {
            return data1;
        }
    };
    inline static constexpr typename Util::Pgm::Converter<ImageGenerator>::pgm_type image1{}; 
};

template<detail::SSD1306::Twi EndPoint>
class SSD1306<EndPoint> : public detail::SSD1306::Base<> {
    using TWIMaster = typename EndPoint::TWIMaster;
    inline static constexpr auto Address = EndPoint::address;
    using master_sync = typename TWIMaster::master_type;
    
public:
    inline static bool init() {
        return TWIMaster::template startWrite<Address>(init_sequence, 0x00_B);        
    }
    inline static void clear() {
        std::array<std::byte, 1> a;
        std::array<std::byte, 128> col;
        for(uint8_t page = 0; page < 8; ++page) {
            a[0] = std::byte(0xb0 + page);
            master_sync::template write<Address>(a, 0x00_B);
            master_sync::template write<Address>(col, 0x40_B);
        }
    }
    inline static void put(char c) {
        TWIMaster::template startWrite<Address>(font()[c], 0x40_B);
    }
    inline static void put(PgmStringView string) {
        char c{'\0'};
        for(uint8_t i = 0; (c = string[i]) != '\0'; ++i) {
            put(c);
        };   
    }
    inline static bool home() {
        auto cs = std::make_array(0xb0_B, 0x21_B, 0x00_B, 0x7f_B);
        return TWIMaster::template startWrite<Address>(cs, 0x00_B);        
    }
    
    inline static bool gotoxy(uint8_t x, uint8_t y) {
        x = x * 6;					// one char: 6 pixel width
        auto cs = std::make_array<std::byte>(std::byte(0xb0+y), 0x21_B, std::byte{x}, 0x7f_B);
        return TWIMaster::template startWrite<Address>(cs, 0x00_B);        
    }
    
private:
//    struct ImageGenerator {
//        inline static constexpr auto data = std::make_array<uint8_t>(
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
//                    0x55,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,
//                    0xaa,0xaa,0xaa,0x01,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x01,0x00,0x00,0x40,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x01,0x00,0xc0,0x06,0x00,0x00,0x80,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x20,0x08,0x00,0x00,0x40,0x55,0x55,
//                    0x55,0x55,0x55,0x55,0x55,0x55,0xd5,0x00,0x00,0x10,0x10,0x00,0x00,0x80,0xaa,
//                    0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0x00,0x00,0x10,0x10,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x15,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x10,
//                    0x00,0x00,0x00,0x00,0x00,0xf8,0xf5,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x10,
//                    0x10,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x0e,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x20,0x08,0x00,0x00,0x00,0x00,0x80,0x01,0x00,0x30,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0xc0,0x06,0x00,0x00,0x00,0x00,0x60,0x00,0x00,0xc0,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x01,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x06,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x08,
//                    0x00,0x00,0x00,0x15,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
//                    0x10,0x00,0x00,0x80,0x75,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
//                    0x00,0x10,0x00,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,
//                    0x00,0x00,0x20,0x00,0x00,0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,
//                    0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x20,
//                    0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x20,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x80,0x40,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x21,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x40,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x20,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x80,0x20,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x41,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x80,
//                    0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,
//                    0x80,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x02,
//                    0x00,0x80,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,
//                    0x02,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,
//                    0x00,0x02,0x00,0x80,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,
//                    0x00,0x00,0x02,0x00,0x80,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,
//                    0x00,0x00,0x00,0x02,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,
//                    0x00,0x00,0x00,0x00,0x02,0x00,0x80,0x21,0x00,0x00,0x00,0x00,0x00,0x00,0x08,
//                    0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x08,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x80,0x20,0x00,0x00,0x04,0x00,0x00,
//                    0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x40,0x00,0x00,0x3f,0x00,
//                    0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x20,0x00,0x80,0x40,
//                    0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x80,0x20,0x00,0x40,
//                    0x40,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x41,0x00,
//                    0x40,0x80,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x80,0x20,
//                    0x00,0x40,0x80,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x80,
//                    0x21,0x00,0x20,0x00,0x01,0x00,0x00,0xc0,0x00,0x00,0x00,0x00,0x20,0x00,0x00,
//                    0x00,0x41,0x00,0x40,0x80,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x10,0x00,
//                    0x00,0x80,0x20,0x00,0x60,0x80,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x08,
//                    0x00,0x00,0x80,0x20,0x00,0x40,0x80,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
//                    0x04,0x00,0x00,0x80,0x40,0x00,0x80,0x40,0x00,0x00,0x00,0x00,0x04,0x00,0x00,
//                    0x00,0x02,0x00,0x00,0x00,0x21,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x10,0x00,
//                    0x00,0x00,0x01,0x00,0x00,0x80,0x20,0x00,0x00,0x36,0x00,0x00,0x00,0x00,0x60,
//                    0x00,0x00,0xc0,0x00,0x00,0x00,0x80,0x41,0x00,0x00,0x08,0x00,0x00,0x00,0x00,
//                    0x80,0x01,0x00,0x30,0x00,0x00,0x00,0x00,0x35,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x0e,0x00,0x0f,0x00,0x00,0x00,0x00,0x15,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0xf8,0xfb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//                    0x00,0x00,0x00,0x00
//                    );
        
////        inline static constexpr auto data1 = []{
////            std::array<std::byte, 1024> d;
////            for(uint8_t r = 0; r < 8; ++r) {
////                for(uint8_t c = 0; c < 128; ++c) {
////                    for(uint8_t i = 0; i < 8; ++i) {
                        
////                    }
////                }
////            }
////            return d;
////        }();
        
//        constexpr auto operator()() {
////            return std::array<std::byte, 1024>{0xff_B};
//            return data;
//        }
//    };
//    using t = typename Util::Pgm::Converter<ImageGenerator>::pgm_type;
//    inline static constexpr typename Util::Pgm::Converter<ImageGenerator>::pgm_type image1{}; 
};
