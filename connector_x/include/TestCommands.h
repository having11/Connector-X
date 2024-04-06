#pragma once

#include <vector>

#include "Commands.h"

struct TestCommand
{
    Command cmd;
    uint16_t delayMs = 0;
};

static std::vector<TestCommand> testCommands =
{
    {
        .cmd = {
            .commandType = CommandType::SetLedPort,
            .commandData = {
                .commandSetLedPort = {
                    .port = 1,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::On,
            .commandData = {
                .commandOn = {},
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::SetPatternZone,
            .commandData = {
                .commandSetPatternZone = {
                    .zoneIndex = 1,
                    .reversed = 0,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::ChangeColor,
            .commandData = {
                .commandColor = {
                    .red = 255,
                    .green = 50,
                    .blue = 40,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::Pattern,
            .commandData = {
                .commandPattern = {
                    .pattern = 14,
                    .oneShot = 0,
                    .delay = -1,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::SetLedPort,
            .commandData = {
                .commandSetLedPort = {
                    .port = 0,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::SetPatternZone,
            .commandData = {
                .commandSetPatternZone = {
                    .zoneIndex = 3,
                    .reversed = 0,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::ChangeColor,
            .commandData = {
                .commandColor = {
                    .red = 255,
                    .green = 255,
                    .blue = 0,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::Pattern,
            .commandData = {
                .commandPattern = {
                    .pattern = 7,
                    .oneShot = 0,
                    .delay = 50,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::SetPatternZone,
            .commandData = {
                .commandSetPatternZone = {
                    .zoneIndex = 1,
                    .reversed = 1,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::ChangeColor,
            .commandData = {
                .commandColor = {
                    .red = 0,
                    .green = 255,
                    .blue = 0,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::Pattern,
            .commandData = {
                .commandPattern = {
                    .pattern = 6,
                    .oneShot = 0,
                    .delay = 50,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::SetPatternZone,
            .commandData = {
                .commandSetPatternZone = {
                    .zoneIndex = 0,
                    .reversed = 0,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::ChangeColor,
            .commandData = {
                .commandColor = {
                    .red = 0,
                    .green = 255,
                    .blue = 0,
                },
            },
        },
        .delayMs = 500,
    },
    {
        .cmd = {
            .commandType = CommandType::Pattern,
            .commandData = {
                .commandPattern = {
                    .pattern = 3,
                    .oneShot = 0,
                    .delay = 20,
                },
            },
        },
        .delayMs = 500,
    },
};