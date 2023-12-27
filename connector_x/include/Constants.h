#pragma once

#include <Arduino.h>
#include <RFM69.h>
#include <unordered_map>

namespace Pin
{
    namespace I2C
    {
        constexpr uint8_t ReceiveBufSize = 128u;
        namespace Port0
        {
            constexpr uint8_t BaseAddress = 0b0010000;

            constexpr uint8_t SDA = 4;
            constexpr uint8_t SCL = 5;
            constexpr uint8_t AddrSwPin0 = 26;
            constexpr uint8_t AddrSwPin1 = 27;
            constexpr uint8_t AddrSwPin2 = 28;
        } // namespace Port0

        namespace Port1
        {
            constexpr uint8_t SDA = 14;
            constexpr uint8_t SCL = 15;
        } // namespace Port1
    }     // namespace I2C

    namespace SPI
    {
        constexpr uint8_t MISO = 12;
        constexpr uint8_t MOSI = 11;
        constexpr uint8_t CLK = 10;
        constexpr uint8_t CS0 = 9;
    } // namespace SPI

    namespace UART
    {
        constexpr uint8_t TX = 0;
        constexpr uint8_t RX = 1;
    } // namespace UART

    namespace LED
    {
        constexpr uint8_t Dout0 = 20;
        constexpr uint8_t Dout1 = 21;
        constexpr uint8_t AliveStatus = 19;
        constexpr uint8_t NumPorts = 2;
        constexpr uint8_t DefaultPort = 0;
    } // namespace LED

    namespace DIGITALIO
    {
        constexpr uint8_t P0 = 2;
        constexpr uint8_t P1 = 3;
        constexpr uint8_t P2 = 16;

        static const std::unordered_map<uint8_t, uint8_t> digitalIOMap = {
            {0, P0}, {1, P1}, {2, P2}};
    } // namespace DIGITALIO

    namespace CONFIG
    {
        constexpr uint8_t ConfigSetupBtn = 8;
        constexpr uint8_t ConfigLed = 22;
        constexpr uint16_t EepromSize = 4096u;
    }
} // namespace Pin

namespace Radio
{
    // ! Change to match schematic
    constexpr uint8_t RadioCS = 13;
    constexpr uint8_t RadioInterrupt = 17;
    constexpr uint8_t MaxRadioAddresses = 254;
    constexpr uint8_t NetworkId = 0;
    constexpr uint8_t Frequency = RF69_915MHZ;
    constexpr uint8_t MaxDataLen = RF69_MAX_DATA_LEN;
    constexpr uint16_t SendToAll = 0xFFFF;
} // namespace Radio

constexpr uint32_t UartBaudRate = 115200;
constexpr uint8_t PatternCount = 6;