#pragma once

#include <FastLED.h>
#include <Arduino.h>

#include "Constants.h"
#include "Patterns.h"

class PatternRunner
{
public:
    PatternRunner() {}

    PatternRunner(CRGB *strip, Pattern *patterns, uint8_t port, uint8_t brightness,
                  uint8_t numPatterns = PatternCount)
        : _pixels(strip), _patternArr(patterns), _port(port), _brightness(brightness),
            _numPatterns(numPatterns),
          _curPattern(0), _curState(0), _oneShot(false), _doneRunning(false),
          _curColor((0)), _lastUpdate(0) {}

    /**
     * Call this every in every iteration of loop
     *
     * @param forceUpdate update even if not enough delay has passed
     */
    void update(bool forceUpdate = false)
    {
        auto curPattern = currentPattern();

        if (forceUpdate)
        {
            incrementState(curPattern);

            return;
        }

        if (shouldUpdate())
        {
            // If we're done, make sure to stop if one shot is set
            if (_curState >= curPattern->numStates)
            {
                if (_oneShot)
                {
                    _doneRunning = true;
                    return;
                }
                else
                {
                    reset();
                }
            }

            incrementState(curPattern);
        }
    }

    void setCurrentColor(uint32_t color)
    {
        _curColor = color;
        reset();
        update(true);
    }

    uint32_t getCurrentColor()
    {
        return _curColor;
    }

    bool setCurrentPattern(uint8_t pattern, uint16_t delay,
                           bool isOneShot = false)
    {
        if (pattern > PatternCount - 1)
        {
            return false;
        }

        _curPattern = pattern;
        _oneShot = isOneShot;
        _delay = delay;
        reset();

        Serial.printf("Set pattern to %d | one shot=%d | delay=%d\r\n", pattern,
                      isOneShot, delay);

        // Force the newly set pattern to run immediately
        update(true);

        return true;
    }

    bool setCurrentPattern(PatternType type, uint16_t delay,
                           bool isOneShot = false)
    {
        return setCurrentPattern((uint8_t)type, delay, isOneShot);
    }

    /**
     * Get pointer to the current pattern
     */
    Pattern *currentPattern() const { return &_patternArr[_curPattern]; }

    Pattern *getPattern(uint8_t index) const { return &_patternArr[index]; }

    inline bool patternDone() const { return _oneShot && _doneRunning; }

    /**
     * Start the current pattern over from the start
     * Will be called automatically at end of each pattern
     */
    inline void reset()
    {
        _curState = 0;
        _lastUpdate = millis();
        _doneRunning = false;
    }

private:
    inline bool shouldUpdate() const
    {
        // Only update if enough delay has passed and pattern can be run again
        return (millis() - _lastUpdate >= _delay) && !(_oneShot && _doneRunning);
    }

    void incrementState(Pattern *curPattern)
    {
        if (curPattern->cb(_pixels, _curColor, _curState, FastLED[_port].size()))
        {
            FastLED[_port].showLeds(_brightness);
        }

        _curState++;
        _lastUpdate = millis();
    }

    CRGB *_pixels;
    Pattern *_patternArr;
    uint8_t _numPatterns;
    uint8_t _curPattern;
    uint8_t _brightness;
    uint16_t _curState;
    uint16_t _delay;
    uint8_t _port;
    bool _oneShot;
    bool _doneRunning;
    uint32_t _curColor;
    uint32_t _lastUpdate;
};
