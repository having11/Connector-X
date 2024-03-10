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
                    .pattern = 8,
                    .oneShot = 0,
                    .delay = 500,
                },
            },
        },
        .delayMs = 500,
    },
};