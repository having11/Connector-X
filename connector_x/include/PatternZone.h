#pragma once

#include <Arduino.h>
#include <FastLED.h>

#include "Commands.h"
#include "Patterns.h"

#include <vector>
#include <memory>

struct ZoneDefinition {
    uint16_t offset;
    uint16_t count;
};

struct RunZone {
    uint16_t index;
    bool reversed;

    explicit RunZone() = default;

    RunZone(uint16_t idx, bool rev)
    {
        index = idx;
        reversed = rev;
    }

    explicit RunZone(const CommandSetPatternZone &command)
    {
        index = command.zoneIndex;
        reversed = command.reversed;
    }
};

class PatternZone {
    public:
        explicit PatternZone(uint16_t ledCount, uint16_t zoneCount = 1);
        explicit PatternZone(const std::vector<ZoneDefinition> &zones);

        /**
         * @brief Update the current zone index
         * 
         * @param index 
         * @return true if successfully set
         */
        bool setRunZone(RunZone zone);

        bool runPattern(Pattern *pattern, CRGB* pixels,
            uint32_t color, uint16_t state);

        std::unique_ptr<std::vector<ZoneDefinition>> _zones;

    private:
        inline ZoneDefinition getZoneFromIndex(uint16_t index)
        {
            return _zones->at(index);
        }

        inline uint16_t getOffsetFromLength(uint16_t index, uint16_t ledCountPerLength)
        {
            return index * ledCountPerLength;
        }

        RunZone _runZone{0, false};
};