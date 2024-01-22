#pragma once

#include <Arduino.h>
#include <FastLED.h>

#include "Commands.h"
#include "Patterns.h"

#include <vector>
#include <memory>
#include <string>

struct ZoneDefinition {
    uint16_t offset;
    uint16_t count;

    ZoneDefinition(uint16_t off, uint16_t ct)
    {
        offset = off;
        count = ct;
    }

    explicit ZoneDefinition(const NewZone &zone)
    {
        offset = zone.offset;
        count = zone.count;
    }

    inline std::string toString()
    {
        std::string str = "Offset: ";
        str += std::to_string(offset) + "\t";
        str += "count: " + std::to_string(count) + "\n";

        return str;
    }
};

struct RunZone {
    uint16_t index;
    bool reversed;
    uint16_t state;
    uint32_t color;
    uint8_t patternIndex;
    uint32_t lastUpdateMs;
    uint16_t delay;
    bool oneShot;
    bool doneRunning;

    explicit RunZone() = default;

    RunZone(uint16_t idx, bool rev)
    {
        index = idx;
        reversed = rev;
        init();
    }

    explicit RunZone(const CommandSetPatternZone &command)
    {
        index = command.zoneIndex;
        reversed = command.reversed;
        init();
    }

    inline bool done() const { return oneShot && doneRunning; }

    inline void reset()
    {
        state = 0;
        lastUpdateMs = millis();
        doneRunning = false;
    }

    void init()
    {
        reset();
        color = 0;
        patternIndex = 0;
    }

    bool shouldUpdate() const
    {
        // Only update if enough delay has passed and pattern can be run again
        return (millis() - lastUpdateMs >= delay) && !(oneShot && doneRunning);
    }
};

class PatternZone {
    public:
        explicit PatternZone(uint8_t port, uint8_t brightness,
            CRGB *leds, uint16_t ledCount, uint16_t zoneCount = 1);
        explicit PatternZone(uint8_t port, uint8_t brightness,
            CRGB *leds, std::vector<ZoneDefinition> *zones);

        /**
         * @brief Update the current zone index
         * 
         * @param zoneIndex 
         * @return true if successfully set
         */
        bool setRunZone(uint16_t zoneIndex, bool reversed);

        bool runPattern(uint16_t index, Pattern *pattern, CRGB* pixels,
            uint32_t color, uint16_t state);

        void updateZones(bool forceUpdate = false);

        void updateZone(uint16_t index, bool forceUpdate = false);

        void setPattern(uint8_t patternIndex, uint16_t delay, bool isOneShot = false);

        inline void setPattern(PatternType type, uint16_t delay, bool isOneShot = false)
        {
            setPattern((uint8_t)type, delay, isOneShot);
        }

        void setColor(uint32_t color);

        void incrementState(uint16_t index, Pattern *pattern);

        inline void reset()
        {
            for (auto& zone : *_runZones)
            {
                zone.reset();
            }
        }

        inline void resetZones(uint8_t *zoneIndexes, uint8_t count)
        {
            for (uint8_t i = 0; i < count; i++)
            {
                auto& runZone = getRunZoneFromIndex(zoneIndexes[i]);
                runZone.reset();
            }
        }

        inline Pattern *getPattern(uint8_t index) const { return &Animation::patterns[index]; }

        std::unique_ptr<std::vector<ZoneDefinition>> _zones;

    private:
        inline ZoneDefinition& getZoneDefinitionFromIndex(uint16_t index)
        {
            return _zones->at(index);
        }

        inline RunZone& getRunZoneFromIndex(uint16_t index)
        {
            return _runZones->at(index);
        }

        inline uint16_t getOffsetFromLength(uint16_t index, uint16_t ledCountPerLength)
        {
            return index * ledCountPerLength;
        }

        uint16_t _zoneIndex = 0;
        uint8_t _port;
        uint8_t _brightness;
        CRGB *_leds;
        std::unique_ptr<std::vector<RunZone>> _runZones;
};