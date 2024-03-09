#pragma once

#include <FastLED_NeoMatrix.h>

#include "Constants.h"
#include "Configurator.h"

static Configuration configuration = {
    .teamNumber = 5690,
    .led0 = {
        .strip = {
            .count = 42,
            .zoneCount = 5,
            .initialZones = {
                ZoneDefinition{0, 9},
                ZoneDefinition{9, 15},
                ZoneDefinition{24, 8},
                ZoneDefinition{32, 5},
                ZoneDefinition{37, 5},
                ZoneDefinition{0, 0},
                ZoneDefinition{0, 0},
                ZoneDefinition{0, 0},
                ZoneDefinition{0, 0},
                ZoneDefinition{0, 0}
            },
        },
        .brightness = 80,
        .isMatrix = false,
    },
    .led1 = {
        .matrix = {
            .width = Matrix::Width,
            .height = Matrix::Height,
            .flags = NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
        },
        .brightness = 40,
        .isMatrix = true,
    }
};