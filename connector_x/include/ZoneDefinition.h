#pragma once

#include <Arduino.h>

#include <string>
#include "Commands.h"

struct ZoneDefinition {
    uint16_t offset;
    uint16_t count;

    explicit ZoneDefinition(uint16_t off, uint16_t ct)
    {
        offset = off;
        count = ct;
    }

    ZoneDefinition()
    {
        offset = 0;
        count = 0;
    }

    inline std::string toString()
    {
        std::string str = "Offset: ";
        str += std::to_string(offset) + "\t";
        str += "count: " + std::to_string(count) + "\n";

        return str;
    }
};