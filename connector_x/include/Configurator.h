#pragma once

#include <Arduino.h>
#include <EEPROM.h>

#include <memory>
#include <string>

#include "Constants.h"

struct LedConfiguration
{
    uint16_t count;
    uint8_t brightness;
};

struct Configuration
{
    uint8_t valid;
    uint16_t teamNumber;
    // Send messages to 2 other teams
    uint16_t initialTeams[2];
    LedConfiguration led0;
    LedConfiguration led1;
};

class Configurator
{
public:
    /**
     * @brief Read the config and optionally create a new one
     * if invalid or config button is held
     *
     * @return Configuration
     */
    Configuration begin();

    Configuration readConfig()
    {
        Configuration config;

        EEPROM.get(0, config);

        for (uint8_t i = 0; i < sizeof(Configuration); i++)
        {
            Serial.printf("%X\r\n", *((uint8_t *)&config + i));
        }

        return config;
    }

    void storeConfig(Configuration config)
    {
        config.valid = 1;

        EEPROM.put(0, config);
        EEPROM.commit();
    }

    bool checkIfValid()
    {
        auto cfg = readConfig();
        return cfg.valid == 1;
    }

    void createConfig();

    static std::string toString(Configuration);
};