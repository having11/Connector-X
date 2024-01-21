#include "PatternZone.h"

PatternZone::PatternZone(uint16_t ledCount, uint16_t zoneCount)
{
    _zones.reserve(zoneCount);
    uint16_t ledCountPerLength = ledCount / zoneCount;

    for (uint16_t i = 0; i < zoneCount; i++)
    {
        uint16_t offset = getOffsetFromLength(i, ledCountPerLength);

        _zones[i] = { .offset = offset, .count = ledCountPerLength };
    }
}

PatternZone::PatternZone(const std::vector<ZoneDefinition> &zones)
{
    _zones = zones;
}

bool PatternZone::setRunZone(RunZone zone)
{
    if (zone.index > _zones.size())
    {
        return false;
    }

    _runZone = zone;
    return true;
}

bool PatternZone::runPattern(Pattern *pattern, CRGB* pixels,
            uint32_t color, uint16_t state)
{
    auto curZone = _zones[_runZone.index];
    CRGB tempLeds[curZone.count];
    
    bool shouldShow = pattern->cb(tempLeds, color, state, curZone.count);

    if (_runZone.reversed)
    {
        for (uint16_t pixel = 0; pixel < curZone.count; pixel++)
        {
            pixels[(curZone.count - pixel) + curZone.offset] = tempLeds[pixel];
        }
    }
    else
    {
        for (uint16_t pixel = 0; pixel < curZone.count; pixel++)
        {
            pixels[pixel + curZone.offset] = tempLeds[pixel];
        }
    }

    return shouldShow;
}