#pragma once

#include <FastLED.h>

#include "Constants.h"

/**
 * Will be called after set delay has passed
 * @param state resets to 0 after current state >= numStates
 * @returns true if LEDs should show
 */
typedef bool (*ExecutePatternCallback)(CRGB *strip, uint32_t color,
                                       uint16_t state, uint16_t ledCount);

enum class PatternType
{
    None = 0,
    SetAll = 1,
    Blink = 2,
    RGBFade = 3,
    HackerMode = 4,
    Breathing = 5,
};

struct Pattern
{
    PatternType type;
    uint16_t numStates;
    uint16_t changeDelayDefault;
    ExecutePatternCallback cb;
};

static void setColorScaled(CRGB *strip, uint16_t ledNumber, byte red, byte green, byte blue, byte scaling)
{
    // Scale RGB with a common brightness parameter
    strip[ledNumber] = CRGB((red * scaling) >> 8, (green * scaling) >> 8, (blue * scaling) >> 8);
}

namespace Animation
{
    static inline uint32_t Wheel(uint8_t position)
    {
        if (position < 85)
        {
            return (uint32_t)CRGB(position * 3, 255 - position * 3, 0);
        }
        else if (position < 170)
        {
            position -= 85;
            return (uint32_t)CRGB(255 - position * 3, 0, position * 3);
        }
        return (uint32_t)CRGB(0, position * 3, 255 - position * 3);
    }
    // The function signature comes from ExecutePatternCallback in Patterns.h

    static bool executePatternNone(CRGB *strip, uint32_t color,
                                   uint16_t state, uint16_t ledCount)
    {
        return false;
    }

    static bool executePatternSetAll(CRGB *strip, uint32_t color,
                                     uint16_t state, uint16_t ledCount)
    {
        for (size_t i = 0; i < ledCount; i++)
        {
            strip[i] = color;
        }

        return true;
    }

    static bool executePatternBlink(CRGB *strip, uint32_t color,
                                    uint16_t state, uint16_t ledCount)
    {
        switch (state)
        {
        case 0:
            for (size_t i = 0; i < ledCount; i++)
            {
                strip[i] = color;
            }
            return true;

        case 1:
            for (size_t i = 0; i < ledCount; i++)
            {
                strip[i] = 0;
            }
            return true;

        default:
            return false;
        }
    }

    static bool executePatternRGBFade(CRGB *strip, uint32_t color,
                                      uint16_t state, uint16_t ledCount)
    {
        for (size_t i = 0; i < ledCount; i++)
        {
            strip[i] = Wheel(((i * 256 / ledCount) + state) & 255);
        }
        return true;
    }

    static bool executePatternHackerMode(CRGB *strip, uint32_t color,
                                         uint16_t state, uint16_t ledCount)
    {
        switch (state)
        {
        case 0:
            return executePatternSetAll(strip, (uint32_t)CRGB(0, 200, 0), 0,
                                        ledCount);

        case 1:
            return executePatternSetAll(strip, (uint32_t)CRGB(5, 100, 5), 0,
                                        ledCount);

        default:
            return false;
        }
    }

    static bool executePatternBreathing(CRGB *strip, uint32_t color,
                                        uint16_t state, uint16_t ledCount)
    {
        if (state > 255)
        {
            state = 511 - state;
        }
        for (size_t i = 0; i < ledCount; i++)
        {
            setColorScaled(strip, i, color >> 16, (color >> 8) & 0xff, color & 0xff, state & 0xff);
        }

        return true;
    }

    // ! The order of these MUST match the order in PatternType !
    static Pattern patterns[PatternCount] = {
        {.type = PatternType::None,
         .numStates = 0,
         .changeDelayDefault = 0,
         .cb = Animation::executePatternNone},
        {.type = PatternType::SetAll,
         .numStates = 1,
         .changeDelayDefault = 500u,
         .cb = Animation::executePatternSetAll},
        {.type = PatternType::Blink,
         .numStates = 2,
         .changeDelayDefault = 400u,
         .cb = Animation::executePatternBlink},
        {.type = PatternType::RGBFade,
         .numStates = 256,
         .changeDelayDefault = 10u,
         .cb = Animation::executePatternRGBFade},
        {.type = PatternType::HackerMode,
         .numStates = 2,
         .changeDelayDefault = 100u,
         .cb = Animation::executePatternHackerMode},
        {.type = PatternType::Breathing,
        .numStates = 512u,
        .changeDelayDefault = 5,
        .cb = Animation::executePatternBreathing}};
} // namespace Animation
