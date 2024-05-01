#pragma once

#include <Arduino.h>
#include <EEPROM.h>

#include <memory>
#include <string>

#include "Constants.h"
#include "ZoneDefinition.h"

struct MatrixConfiguration
{
    uint16_t width;
    uint16_t height;
    uint8_t flags;
};

struct LedStripConfiguration
{
    uint16_t count;
    // If -1, will use initialZones rather than creating a single zone
    int8_t zoneCount = -1;
    ZoneDefinition initialZones[10];
};

struct LedConfiguration
{
    LedStripConfiguration strip;
    MatrixConfiguration matrix;
    uint8_t brightness;
    bool isMatrix;
};

struct Configuration
{
    uint16_t teamNumber;
    LedConfiguration led0;
    LedConfiguration led1;
    uint8_t valid = 1;
};

// class Configurator
// {
// public:
//     /**
//      * @brief Read the config and optionally create a new one
//      * if invalid or config button is held
//      *
//      * @return Configuration
//      */
//     Configuration begin();

//     Configuration readConfig()
//     {
//         Configuration config;

//         EEPROM.get(0, config);

//         for (uint8_t i = 0; i < sizeof(Configuration); i++)
//         {
//             Serial.printf("%X\r\n", *((uint8_t *)&config + i));
//         }

//         return config;
//     }

//     void storeConfig(Configuration config)
//     {
//         config.valid = 1;

//         EEPROM.put(0, config);
//         EEPROM.commit();
//     }

//     bool checkIfValid()
//     {
//         auto cfg = readConfig();
//         return cfg.valid == 1;
//     }

//     void createConfig();

//     static std::string toString(Configuration);
// };