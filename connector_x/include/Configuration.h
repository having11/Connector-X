#pragma once

#include <FastLED_NeoMatrix.h>

#include "Constants.h"
#include "Configurator.h"

static Configuration configuration = {
    .teamNumber = 5690,
    .led0 = {
        .strip = {
            .count = 93,
            .zoneCount = 4,
            .initialZones = {
                ZoneDefinition{0, 18},
                ZoneDefinition{17, 32},
                ZoneDefinition{50, 17},
                ZoneDefinition{67, 26},
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