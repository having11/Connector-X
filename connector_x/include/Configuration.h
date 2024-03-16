#pragma once

#include <FastLED_NeoMatrix.h>

#include "Constants.h"
#include "Configurator.h"

static Configuration configuration = {
    .teamNumber = 5690,
    .led0 = {
        .strip = {
            .count = 66,
            .zoneCount = 3,
            .initialZones = {
                ZoneDefinition{0, 17},
                ZoneDefinition{17, 32},
                ZoneDefinition{49, 17},
                ZoneDefinition{0, 0},
                ZoneDefinition{0, 0},
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
        .brightness = 50,
        .isMatrix = true,
    }
};