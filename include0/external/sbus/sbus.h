#pragma once

#include <cstdint>
#include <cstddef>
#include <limits>
#include <chrono>

#include <etl/ranged.h>
#include <etl/algorithm.h>

#include <mcu/internals/usart.h>

namespace External {
    namespace SBus {
#if 0
#define RC_CHANNEL_MIN 990
#define RC_CHANNEL_MAX 2010

#define SBUS_MIN_OFFSET 173
#define SBUS_MID_OFFSET 992
#define SBUS_MAX_OFFSET 1811
#define SBUS_CHANNEL_NUMBER 16
#define SBUS_PACKET_LENGTH 25
#define SBUS_FRAME_HEADER 0x0f
#define SBUS_FRAME_FOOTER 0x00
#define SBUS_FRAME_FOOTER_V2 0x04
#define SBUS_STATE_FAILSAFE 0x08
#define SBUS_STATE_SIGNALLOSS 0x04
#define SBUS_UPDATE_RATE 15 //ms

void sbusPreparePacket(uint8_t packet[], int channels[], bool isSignalLoss, bool isFailsafe){

    static int output[SBUS_CHANNEL_NUMBER] = {0};

    /*
     * Map 1000-2000 with middle at 1500 chanel values to
     * 173-1811 with middle at 992 S.BUS protocol requires
     */
    for (uint8_t i = 0; i < SBUS_CHANNEL_NUMBER; i++) {
        output[i] = map(channels[i], RC_CHANNEL_MIN, RC_CHANNEL_MAX, SBUS_MIN_OFFSET, SBUS_MAX_OFFSET);
    }

    uint8_t stateByte = 0x00;
    if (isSignalLoss) {
        stateByte |= SBUS_STATE_SIGNALLOSS;
    }
    if (isFailsafe) {
        stateByte |= SBUS_STATE_FAILSAFE;
    }
    packet[0] = SBUS_FRAME_HEADER; //Header

    packet[1] = (uint8_t) (output[0] & 0x07FF);
    usart::put((uint8_t) ((output[0] & 0x07FF)>>8 | (output[1] & 0x07FF)<<3);
    packet[3] = (uint8_t) ((output[1] & 0x07FF)>>5 | (output[2] & 0x07FF)<<6);
    packet[4] = (uint8_t) ((output[2] & 0x07FF)>>2);
    packet[5] = (uint8_t) ((output[2] & 0x07FF)>>10 | (output[3] & 0x07FF)<<1);
    packet[6] = (uint8_t) ((output[3] & 0x07FF)>>7 | (output[4] & 0x07FF)<<4);
    packet[7] = (uint8_t) ((output[4] & 0x07FF)>>4 | (output[5] & 0x07FF)<<7);
    packet[8] = (uint8_t) ((output[5] & 0x07FF)>>1);
    packet[9] = (uint8_t) ((output[5] & 0x07FF)>>9 | (output[6] & 0x07FF)<<2);
    packet[10] = (uint8_t) ((output[6] & 0x07FF)>>6 | (output[7] & 0x07FF)<<5);
    packet[11] = (uint8_t) ((output[7] & 0x07FF)>>3);
    packet[12] = (uint8_t) ((output[8] & 0x07FF));
    packet[13] = (uint8_t) ((output[8] & 0x07FF)>>8 | (output[9] & 0x07FF)<<3);
    packet[14] = (uint8_t) ((output[9] & 0x07FF)>>5 | (output[10] & 0x07FF)<<6);  
    packet[15] = (uint8_t) ((output[10] & 0x07FF)>>2);
    packet[16] = (uint8_t) ((output[10] & 0x07FF)>>10 | (output[11] & 0x07FF)<<1);
    packet[17] = (uint8_t) ((output[11] & 0x07FF)>>7 | (output[12] & 0x07FF)<<4);
    packet[18] = (uint8_t) ((output[12] & 0x07FF)>>4 | (output[13] & 0x07FF)<<7);
    packet[19] = (uint8_t) ((output[13] & 0x07FF)>>1);
    packet[20] = (uint8_t) ((output[13] & 0x07FF)>>9 | (output[14] & 0x07FF)<<2);
    packet[21] = (uint8_t) ((output[14] & 0x07FF)>>6 | (output[15] & 0x07FF)<<5);
    packet[22] = (uint8_t) ((output[15] & 0x07FF)>>3);

    packet[23] = stateByte; //Flags byte
    packet[24] = SBUS_FRAME_FOOTER; //Footer
}

uint8_t sbusPacket[SBUS_PACKET_LENGTH];
int rcChannels[SBUS_CHANNEL_NUMBER];
uint32_t sbusTime = 0;

#endif   
        namespace Servo {
            template<auto N = 0>
            struct ProtocollAdapter {
            };
        }
        namespace Output {
            template<typename CN>
            struct Generator {
                
                using usart = AVR::Usart<CN, External::Hal::NullProtocollAdapter, AVR::UseInterrupts<false>, AVR::ReceiveQueueLength<2>, AVR::SendQueueLength<26>>;
                
                inline static constexpr uint16_t sbus_min = 172;
                inline static constexpr uint16_t sbus_max = 1811;
                
                
                inline static void init() {
                    usart::template init<AVR::BaudRate<100000>, AVR::FullDuplex, true, 1>();
                    for(auto& o : output) {
//                        o = (sbus_max + sbus_min) / 2;
                        o = sbus_max;
                    }
                }

                static inline constexpr std::byte sbus_start = 0x0f_B;

                inline static void periodic() {
                    usart::periodic();
                }
                
                inline static void ratePeriodic() { // 14ms
                    usart::put(sbus_start);
                
                    usart::put((std::byte) (output[0] & 0x07FF));
                    usart::put((std::byte) ((output[0] & 0x07FF)>>8 | (output[1] & 0x07FF)<<3));
                    usart::put((std::byte) ((output[1] & 0x07FF)>>5 | (output[2] & 0x07FF)<<6));
                    usart::put((std::byte) ((output[2] & 0x07FF)>>2));
                    usart::put((std::byte) ((output[2] & 0x07FF)>>10 | (output[3] & 0x07FF)<<1));
                    usart::put((std::byte) ((output[3] & 0x07FF)>>7 | (output[4] & 0x07FF)<<4));
                    usart::put((std::byte) ((output[4] & 0x07FF)>>4 | (output[5] & 0x07FF)<<7));
                    usart::put((std::byte) ((output[5] & 0x07FF)>>1));
                    usart::put((std::byte) ((output[5] & 0x07FF)>>9 | (output[6] & 0x07FF)<<2));
                    usart::put((std::byte) ((output[6] & 0x07FF)>>6 | (output[7] & 0x07FF)<<5));
                    usart::put((std::byte) ((output[7] & 0x07FF)>>3));
                    usart::put((std::byte) ((output[8] & 0x07FF)));
                    usart::put((std::byte) ((output[8] & 0x07FF)>>8 | (output[9] & 0x07FF)<<3));
                    usart::put((std::byte) ((output[9] & 0x07FF)>>5 | (output[10] & 0x07FF)<<6));  
                    usart::put((std::byte) ((output[10] & 0x07FF)>>2));
                    usart::put((std::byte) ((output[10] & 0x07FF)>>10 | (output[11] & 0x07FF)<<1));
                    usart::put((std::byte) ((output[11] & 0x07FF)>>7 | (output[12] & 0x07FF)<<4));
                    usart::put((std::byte) ((output[12] & 0x07FF)>>4 | (output[13] & 0x07FF)<<7));
                    usart::put((std::byte) ((output[13] & 0x07FF)>>1));
                    usart::put((std::byte) ((output[13] & 0x07FF)>>9 | (output[14] & 0x07FF)<<2));
                    usart::put((std::byte) ((output[14] & 0x07FF)>>6 | (output[15] & 0x07FF)<<5));
                    usart::put((std::byte) ((output[15] & 0x07FF)>>3));
                
                    usart::put(0x00_B); //Flags byte
                    usart::put(0x00_B); //Footer
                }
//            private:
                static inline std::array<uint16_t, 16> output;                 
            };  
        }
    }
}
