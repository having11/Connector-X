#pragma once

#include "Configurator.h"
#include "PacketRadio.h"
#include <Arduino.h>

enum class CommandType
{
    // W
    On = 0,
    // W
    Off = 1,
    // W
    Pattern = 2,
    // W
    ChangeColor = 3,
    // R
    ReadPatternDone = 4,
    // W
    SetLedPort = 5,
    // W
    DigitalSetup = 7,
    // W
    DigitalWrite = 8,
    // R
    DigitalRead = 9,
    // W
    SetConfig = 10,
    // R
    ReadConfig = 11,
    // W
    RadioSend = 12,
    // R
    RadioGetLatestReceived = 13
};

struct CommandOn
{
};

struct CommandOff
{
};

// * Set delay to -1 to use default delay
struct CommandPattern
{
    uint8_t pattern;
    uint8_t oneShot;
    int16_t delay;
};

struct CommandColor
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct CommandReadPatternDone
{
};

struct CommandSetLedPort
{
    uint8_t port;
};

struct CommandDigitalSetup
{
    uint8_t port;
    /** Follows the Arduino-defined values for pinMode
     * INPUT = 0
     * INPUT_PULLUP = 2
     * INPUT_PULLDOWN = 3
     * OUTPUT = 1
     * OUTPUT_2MA = 4
     * OUTPUT_4MA = 5
     * OUTPUT_8MA = 6
     * OUTPUT_12MA = 7
     */
    uint8_t mode;
};

struct CommandDigitalWrite
{
    uint8_t port;
    uint8_t value;
};

struct CommandDigitalRead
{
    uint8_t port;
};

struct CommandSetConfig
{
    Configuration config;
};

struct CommandReadConfig
{
};

struct CommandRadioSend
{
    Message msg;
};

struct CommandRadioGetLatestReceived
{
};

union CommandData
{
    CommandOn commandOn;
    CommandOff commandOff;
    CommandPattern commandPattern;
    CommandColor commandColor;
    CommandReadPatternDone commandReadPatternDone;
    CommandSetLedPort commandSetLedPort;
    CommandDigitalSetup commandDigitalSetup;
    CommandDigitalWrite commandDigitalWrite;
    CommandDigitalRead commandDigitalRead;
    CommandSetConfig commandSetConfig;
    CommandReadConfig commandReadConfig;
    CommandRadioSend commandRadioSend;
    CommandRadioGetLatestReceived commandRadioGetLatestReceived;
};

struct Command
{
    CommandType commandType;
    CommandData commandData;
};

struct ResponsePatternDone
{
    uint8_t done;
};

struct ResponseReadAnalog
{
    uint16_t value;
};

struct ResponseDigitalRead
{
    uint8_t value;
};

struct ResponseRadioLastReceived
{
    Message msg;
};

struct ResponseReadConfiguration
{
    Configuration config;
};

union ResponseData
{
    ResponsePatternDone responsePatternDone;
    ResponseDigitalRead responseDigitalRead;
    ResponseRadioLastReceived responseRadioLastReceived;
    ResponseReadConfiguration responseReadConfiguration;
};

struct Response
{
    CommandType commandType;
    ResponseData responseData;
};