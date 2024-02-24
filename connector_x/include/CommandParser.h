#pragma once

#include <pico/mutex.h>

#include "Arduino.h"
#include "Commands.h"

namespace CommandParser
{
    static void parseCommand(uint8_t *buf, size_t len, Command *cmd)
    {
        auto type = (CommandType)buf[0];
        Serial.print("Received command type=");
        Serial.println(buf[0]);
        cmd->commandType = type;
        switch (type)
        {
        case CommandType::On:
            cmd->commandData.commandOn = {};
            break;

        case CommandType::Off:
            cmd->commandData.commandOff = {};
            break;

        case CommandType::Pattern:
            memcpy(&cmd->commandData.commandPattern, &buf[1], sizeof(CommandPattern));
            break;

        case CommandType::ChangeColor:
            memcpy(&cmd->commandData.commandColor.red, &buf[1], sizeof(CommandColor));
            break;

        case CommandType::ReadPatternDone:
            cmd->commandData.commandReadPatternDone = {};
            break;

        case CommandType::SetLedPort:
            memcpy(&cmd->commandData.commandSetLedPort.port, &buf[1],
                   sizeof(CommandSetLedPort));
            break;

        case CommandType::DigitalSetup:
            memcpy(&cmd->commandData.commandDigitalSetup.port, &buf[1],
                   sizeof(CommandDigitalSetup));
            break;

        case CommandType::DigitalWrite:
            memcpy(&cmd->commandData.commandDigitalWrite.port, &buf[1],
                   sizeof(CommandDigitalWrite));
            break;

        case CommandType::DigitalRead:
            memcpy(&cmd->commandData.commandDigitalRead.port, &buf[1],
                   sizeof(CommandDigitalRead));
            break;

        case CommandType::SetConfig:
            memcpy(&cmd->commandData.commandSetConfig.config, &buf[1],
                   sizeof(CommandSetConfig));
            break;

        case CommandType::ReadConfig:
            cmd->commandData.commandReadConfig = {};
            break;

        case CommandType::RadioSend:
            memcpy(&cmd->commandData.commandRadioSend.msg, &buf[1],
                   sizeof(CommandRadioSend));
            break;

        case CommandType::RadioGetLatestReceived:
            cmd->commandData.commandRadioGetLatestReceived = {};
            break;
        
        case CommandType::GetColor:
            cmd->commandData.commandGetColor = {};
            break;

        case CommandType::GetPort:
            cmd->commandData.commandGetPort = {};
            break;

        case CommandType::SetPatternZone:
            memcpy(&cmd->commandData.commandSetPatternZone.zoneIndex, &buf[1],
                sizeof(CommandSetPatternZone));
            break;

        case CommandType::SetNewZones:
        {
            uint8_t zones = buf[1];
            cmd->commandData.commandSetNewZones.zoneCount = zones;
            memcpy(&cmd->commandData.commandSetNewZones.zones, &buf[2],
                zones * sizeof(NewZone));
            break;
        }

        case CommandType::SyncStates:
        {
            uint8_t zones = buf[1];
            cmd->commandData.commandSyncZoneStates.zoneCount = zones;
            memcpy(&cmd->commandData.commandSyncZoneStates.zones, &buf[2],
                zones * sizeof(uint8_t));
            break;
        }

        default:
            break;
        }
    }
} // namespace CommandParser

struct CommandDequeNode {
    CommandDequeNode* next;
    Command cmd;

    CommandDequeNode(CommandDequeNode* after, Command command) {
        next = after;
        cmd = command;
    }
};

class CommandDeque {
    public:
        CommandDeque() : _size{0}, _head{nullptr}, _tail{nullptr} {
        }

        ~CommandDeque() {
            if (!_head) {
                return;
            }

            CommandDequeNode* curNode = _head;
            while (curNode) {
                auto* nextNode = curNode->next;
                delete curNode;
                curNode = nextNode;
            }
        }

        bool nextCommandAvailable() {
            return _head;
        }

        int getNextCommand(Command* outCommand) {
            if (_size <= 0 || !_head) {
                return -1;
            }

            auto* newHead = _head->next;
            *outCommand = _head->cmd;
            delete _head;
            _head = newHead;

            _size--;

            if (!_head) {
                _tail = nullptr;
            }

            return size();
        }

        int pushCommand(Command command) {
            CommandDequeNode* newNode = new CommandDequeNode(nullptr, command);

            if (!_head || !_tail) {
                _head = newNode;
                _tail = newNode;
            } else {
                _tail->next = newNode;
                _tail = newNode;
            }

            _size++;

            return size();
        }

        int size() {
            return _size;
        }

    private:
        int _size = 0;
        CommandDequeNode* _head;
        CommandDequeNode* _tail;
};