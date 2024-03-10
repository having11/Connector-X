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

        _zones->push_back(ZoneDefinition(offset, ledCountPerLength ));
        _runZones->push_back(RunZone(i, false));
    }

    // Serial.printf("Zone size=%d\r\n", _zones->size());
    // for (uint16_t i = 0; i < _zones->size(); i++)
    // {
    //     Serial.println(_zones->at(i).toString().c_str());
    // }
}

PatternZone::PatternZone(uint8_t port, uint8_t brightness,
            CRGB *leds, std::vector<ZoneDefinition> *zones)
    : _leds(leds), _port(port), _brightness(brightness)
{
    _zones.reset(zones);
    for (uint8_t i = 0; i < _zones->size(); i++)
    {
        Serial.printf("Zone: %s\r\n", _zones->at(i).toString().c_str());
    }
    _runZones = std::make_unique<std::vector<RunZone>>();

    // Serial.printf("Zone size=%d\r\n", _zones->size());
    for (uint16_t i = 0; i < _zones->size(); i++)
    {
        _runZones->push_back(RunZone(i, false));
    }
}

bool PatternZone::setRunZone(uint16_t index, bool reversed)
{
    if (index > _zones->size() - 1)
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
    auto& curZoneDef = getZoneDefinitionFromIndex(index);
    auto& runZone = getRunZoneFromIndex(index);

    // Serial.printf("Creating %d temp LED array\r\n", curZoneDef.count);

    CRGB *tempLeds = new CRGB[curZoneDef.count];
    memset(tempLeds, 0, sizeof(CRGB) * curZoneDef.count);

    // Serial.printf("Created %d temp LED array\r\n", curZoneDef.count);
    // Serial.printf("Color=%lu, state=%u\r\n", runZone.color, runZone.state);
    bool shouldShow = pattern->cb(tempLeds, runZone.color, runZone.state, curZoneDef.count);
    // Serial.printf("Should show=%u\r\n", shouldShow);

    // Serial.printf("First pixel=%lu\r\n", (uint32_t)tempLeds[0]);

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
            // Serial.printf("Mapping temp led=%d to led=%d color=%X\r\n", pixel, pixel + curZoneDef.offset, (uint32_t)tempLeds[pixel]);
            _leds[pixel + curZoneDef.offset] = tempLeds[pixel];
        }
    }

    delete[] tempLeds;

    return shouldShow;
}

void PatternZone::updateZones(bool forceUpdate)
{
    // Serial.printf("Updating zones\r\n");
    for (uint16_t zone = 0; zone < _runZones->size(); zone++)
    {
        updateZone(zone, forceUpdate);
    }
}

void PatternZone::updateZone(uint16_t index, bool forceUpdate)
{
    RunZone& runZone = getRunZoneFromIndex(index);
    auto& curZoneDef = getZoneDefinitionFromIndex(index);
    auto curPattern = getPattern(runZone.patternIndex);

    // Serial.printf("Updating zone index=%u, state=%d, patternIndex=%d\r\n",
    //     index, runZone.state, runZone.patternIndex);

    if (forceUpdate)
    {
        incrementState(index, curPattern);

        return;
    }

    if (runZone.shouldUpdate())
    {
        // Serial.printf("Updating zone index=%u, state=%d, patternIndex=%d\r\n",
        //     index, runZone.state, runZone.patternIndex);
        // If we're done, make sure to stop if one shot is set
        uint16_t stateCount = curPattern->mode == PatternStateMode::Constant ?
            curPattern->numStates :
            curZoneDef.count + curPattern->numStates;
        if (runZone.state >= stateCount)
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

            // Serial.printf("Reset zone index=%u, state=%d, lastUpdate=%lu\r\n",
            //     index, runZone.state, runZone.lastUpdateMs);
        }

        if (incrementState(index, curPattern)) {
            // Serial.printf("Showing %d\n", _port);
            FastLED[_port].showLeds(_brightness);
        }
    }
}

void PatternZone::setPattern(uint8_t patternIndex, uint16_t delay, bool isOneShot)
{
    RunZone& runZone = getRunZoneFromIndex(_zoneIndex);
    
    if (patternIndex > PatternCount - 1)
    {
        return;
    }

    runZone.patternIndex = patternIndex;
    runZone.oneShot = isOneShot;
    runZone.delay = delay;
    runZone.reset();

    // Serial.printf("Set pattern to %d | one shot=%d | delay=%d | zone=%d\r\n", patternIndex,
    //                 isOneShot, delay, _zoneIndex);
    
    updateZone(_zoneIndex, true);
}

void PatternZone::setColor(uint32_t color)
{
    RunZone& runZone = getRunZoneFromIndex(_zoneIndex);
    runZone.color = color;

    updateZone(_zoneIndex, true);
}

bool PatternZone::incrementState(uint16_t index, Pattern *pattern)
{
    // Serial.printf("Incrementing state for index=%u\r\n", index);
    RunZone& runZone = getRunZoneFromIndex(index);

    bool shouldUpdate = runPattern(index, pattern, _leds, runZone.color, runZone.state);
    if (shouldUpdate)
    {
        // Serial.printf("Showing\r\n");
    }

    runZone.state++;
    runZone.lastUpdateMs = millis();

    return shouldUpdate;
}