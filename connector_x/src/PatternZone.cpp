#include "PatternZone.h"

PatternZone::PatternZone(uint8_t port, uint8_t brightness,
            CRGB *leds, uint16_t ledCount, uint16_t zoneCount)
    : _leds(leds), _port(port), _brightness(brightness)
{
    uint16_t ledCountPerLength = ledCount / zoneCount;
    _zones = std::make_unique<std::vector<ZoneDefinition>>();
    _runZones = std::make_unique<std::vector<RunZone>>();

    for (uint16_t i = 0; i < zoneCount; i++)
    {
        uint16_t offset = getOffsetFromLength(i, ledCountPerLength);

        _zones->push_back({ .offset = offset, .count = ledCountPerLength });
        _runZones->push_back(RunZone(i, false));
    }

    Serial.printf("Zone size=%d\r\n", _zones->size());
}

PatternZone::PatternZone(uint8_t port, uint8_t brightness,
            CRGB *leds, const std::vector<ZoneDefinition> &zones)
    : _leds(leds), _port(port), _brightness(brightness)
{
    *_zones = zones;
    _runZones = std::make_unique<std::vector<RunZone>>();

    for (uint16_t i = 0; i < zones.size(); i++)
    {
        _runZones->push_back(RunZone(i, false));
    }
}

bool PatternZone::setRunZone(uint16_t index, bool reversed)
{
    if (index > _zones->size())
    {
        return false;
    }

    RunZone& runZone = getRunZoneFromIndex(index);
    runZone.reversed = reversed;

    _zoneIndex = index;
    return true;
}

bool PatternZone::runPattern(uint16_t index, Pattern *pattern, CRGB* pixels,
            uint32_t color, uint16_t state)
{
    auto curZoneDef = getZoneDefinitionFromIndex(_zoneIndex);
    auto& runZone = getRunZoneFromIndex(_zoneIndex);

    CRGB tempLeds[curZoneDef.count];
    
    bool shouldShow = pattern->cb(tempLeds, color, state, curZoneDef.count);

    if (runZone.reversed)
    {
        for (uint16_t pixel = 0; pixel < curZoneDef.count; pixel++)
        {
            _leds[(curZoneDef.count - pixel) + curZoneDef.offset] = tempLeds[pixel];
        }
    }
    else
    {
        for (uint16_t pixel = 0; pixel < curZoneDef.count; pixel++)
        {
            _leds[pixel + curZoneDef.offset] = tempLeds[pixel];
        }
    }

    return shouldShow;
}

void PatternZone::updateZones(bool forceUpdate)
{
    for (uint16_t zone = 0; zone < _runZones->size(); zone++)
    {
        updateZone(zone, forceUpdate);
    }
}

void PatternZone::updateZone(uint16_t index, bool forceUpdate)
{
    RunZone& runZone = getRunZoneFromIndex(index);
    auto curPattern = getPattern(runZone.patternIndex);

    if (forceUpdate)
    {
        incrementState(index, curPattern);

        return;
    }

    if (runZone.shouldUpdate())
    {
        // If we're done, make sure to stop if one shot is set
        if (runZone.state >= curPattern->numStates)
        {
            if (runZone.oneShot)
            {
                runZone.doneRunning = true;
                return;
            }
            else
            {
                runZone.reset();
            }
        }

        incrementState(index, curPattern);
    }
}

void PatternZone::setPattern(uint8_t patternIndex, uint16_t delay, bool isOneShot)
{
    RunZone& runZone = getRunZoneFromIndex(_zoneIndex);
    
    if (patternIndex > PatternCount - 1)
    {
        return;
    }

    runZone.patternIndex = _zoneIndex;
    runZone.oneShot = isOneShot;
    runZone.delay = delay;
    runZone.reset();

    Serial.printf("Set pattern to %d | one shot=%d | delay=%d | zone=%d\r\n", patternIndex,
                    isOneShot, delay, _zoneIndex);
    
    updateZone(_zoneIndex, true);
}

void PatternZone::setColor(uint32_t color)
{
    RunZone& runZone = getRunZoneFromIndex(_zoneIndex);
    runZone.color = color;

    updateZone(_zoneIndex, true);
}

void PatternZone::incrementState(uint16_t index, Pattern *pattern)
{
    RunZone& runZone = getRunZoneFromIndex(index);

    if (runPattern(index, pattern, _leds, runZone.color, runZone.state))
    {
        FastLED[_port].showLeds(_brightness);
    }

    runZone.state++;
    runZone.lastUpdateMs = millis();
}