#include "PacketRadio.h"

PacketRadio::PacketRadio(SPIClass *spi, Configuration config,
                         std::function<void(Message)> newDataCb)
    : _cb(newDataCb), _config(config) {
  _radio = std::make_unique<RFM69_ATC>(Radio::RadioCS, Radio::RadioInterrupt,
                                       true, spi);
}

void PacketRadio::init() {
  _radio->initialize(Radio::Frequency, hashTeamNumber(_config.teamNumber),
                     Radio::NetworkId);
  _radio->setFrequency(Radio::Frequency);
  _radio->enableAutoPower();
  _radio->setHighPower();
  _radio->setPowerLevel(31);
}

void PacketRadio::update() {
  if (_radio->receiveDone()) {
    _lastSenderId = _radio->SENDERID;
    _lastDataLen = _radio->DATALEN;
    memcpy(_lastData, _radio->DATA, _lastDataLen);
    _newDataFlag = true;
    _radio->sendACK();
  }

  if (_newDataFlag && _lastSenderId != 255) {
    _cb(getLastReceived());
  }
}

void PacketRadio::send(Message message) {
  _radio->send(hashTeamNumber(message.teamNumber), message.data, message.len);
}

void PacketRadio::sendToAll(Message message) {
  _radio->send(255, message.data, message.len);
}

Message PacketRadio::getLastReceived() {
  if (_lastSenderId != 255) {
    Message msg;
    memcpy(msg.data, _lastData, _lastDataLen);
    msg.len = _lastDataLen;
    msg.teamNumber = getTeamBySenderId(_lastSenderId);

    return msg;
  }

  Message msg;
  return msg;
}