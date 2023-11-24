#pragma once

#include <Arduino.h>
#include <Hash.h>
#include <RFM69.h>
#include <RFM69_ATC.h>
#include <SPI.h>
#include <functional>
#include <memory>
#include <unordered_map>

#include "Configurator.h"
#include "Constants.h"

#define ATC_RSSI -80

struct Message
{
    uint8_t data[61];
    uint8_t len;
    // Set to 0xFFFF to send to all
    uint16_t teamNumber;
};

class PacketRadio
{
public:
    PacketRadio(SPIClass *spi, Configuration config,
                std::function<void(Message)> newDataCb);

    void init();

    void update();

    void send(Message message);

    void sendToAll(Message message);

    Message getLastReceived();

    inline uint16_t getTeamBySenderId(uint8_t senderId)
    {
        auto itr = _addressToTeam.find(senderId);
        if (itr != _addressToTeam.end())
        {
            return itr->second;
        }

        return 0xFFFF;
    }

    void addTeam(uint16_t teamNumber) { hashTeamNumber(teamNumber); }

private:
    // Increase speed of lookups by using a cache
    // Run this for every expected team BEFORE receiving packets
    uint8_t hashTeamNumber(uint16_t teamNumber)
    {
        auto cachedEl = _teamCache.find(teamNumber);
        if (cachedEl != _teamCache.end())
        {
            return cachedEl->second;
        }

        uint8_t hash[20];
        sha1((uint8_t *)&teamNumber, sizeof(teamNumber), hash);
        uint16_t val = hash[1] << 8 + hash[0];
        uint8_t hashVal = val % Radio::MaxRadioAddresses;
        _teamCache.insert({teamNumber, hashVal});
        _addressToTeam.insert({hashVal, teamNumber});

        return hashVal;
    }

    std::unique_ptr<RFM69_ATC> _radio;
    std::unordered_map<uint16_t, uint8_t> _teamCache;
    std::unordered_map<uint8_t, uint16_t> _addressToTeam;
    std::function<void(Message)> _cb;
    Configuration _config;

    uint8_t _lastSenderId = 255;
    uint8_t _lastDataLen = 0;
    uint8_t _lastData[Radio::MaxDataLen] = {0};
    bool _newDataFlag = false;
};