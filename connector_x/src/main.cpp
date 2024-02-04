#define PICO_USE_STACK_GUARDS 1

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#include <pico/mutex.h>

#include "CommandParser.h"
#include "Commands.h"
#include "Configurator.h"
#include "Constants.h"
#include "PacketRadio.h"
#include "PatternZone.h"

#include <memory>

// Uncomment to enable radio module communications
// #define ENABLE_RADIO

static mutex_t i2cCommandMtx;
static mutex_t radioDataMtx;

// Forward declarations
void receiveEvent(int);
void requestEvent(void);
void handleRadioDataReceive(Message msg);
void centralRespond(Response response);
void initI2C0(void);
void initPixels(LedConfiguration *config, uint8_t port);

CRGB *getPixels(uint8_t port);
// PatternRunner *getPatternRunner(uint8_t port);

static volatile uint8_t receiveBuf[PinConstants::I2C::ReceiveBufSize];
static volatile bool newData = false;
static volatile bool newDataToParse = false;

static Configuration config;
static Configurator configurator;
static volatile uint8_t ledPort = 0;
static const std::vector<ZoneDefinition> defaultZoneDefs = {
    ZoneDefinition{0, 25},
    ZoneDefinition{25, 25},
    ZoneDefinition{50, 25},
    ZoneDefinition{75, 25},
};

static CRGB *pixels[PinConstants::LED::NumPorts];
// TODO: Make this an array of arrays -> Each port can have multiple zones
static std::unique_ptr<PatternZone> zones[PinConstants::LED::NumPorts];
// static PatternRunner *patternRunners[PinConstants::LED::NumPorts];

#ifdef ENABLE_RADIO
static PacketRadio *radio;
#endif

static Command command;

static volatile bool systemOn = true;

void setup()
{
    rp2040.idleOtherCore();

    pinMode(PinConstants::LED::AliveStatus, OUTPUT);
    digitalWrite(PinConstants::LED::AliveStatus, HIGH);

    Wire1.setSDA(PinConstants::I2C::Port1::SDA);
    Wire1.setSCL(PinConstants::I2C::Port1::SCL);
    Wire1.begin();

    EEPROM.begin(PinConstants::CONFIG::EepromSize);

    Serial.begin(UartBaudRate);

    config = configurator.begin();

    // Peripherals
    delay(1000);
    initI2C0();
    Serial.println("I20 init done");

    SPI1.setTX(PinConstants::SPI::MOSI);
    SPI1.setRX(PinConstants::SPI::MISO);
    SPI1.setSCK(PinConstants::SPI::CLK);
    SPI1.setCS(PinConstants::SPI::CS0);
    SPI1.begin();

    Serial1.setTX(PinConstants::UART::TX);
    Serial1.setRX(PinConstants::UART::RX);
    Serial1.begin(UartBaudRate);

    mutex_init(&i2cCommandMtx);
    mutex_init(&radioDataMtx);

    pinMode(PinConstants::SPI::CS0, OUTPUT);
    digitalWrite(PinConstants::SPI::CS0, HIGH);

    for (auto pin : PinConstants::DIGITALIO::digitalIOMap)
    {
        pinMode(pin.second, OUTPUT);
        digitalWrite(pin.second, LOW);
    }

    Serial.println("Initializing pixels");
    initPixels(&config.led0, 0);
    initPixels(&config.led1, 1);

    rp2040.resumeOtherCore();


#ifdef ENABLE_RADIO
    radio = new PacketRadio(&SPI1, config, handleRadioDataReceive);

    for (int i = 0; i < 2; i++)
    {
        radio->addTeam(config.initialTeams[i]);
    }
#endif

    Serial.printf("Got config:\r\n%s\r\n",
                  Configurator::toString(config).c_str());

    
}

void setup1()
{
    Serial.println("Setup1 has ended");
}

void loop1()
{
    if (rp2040.fifo.available())
    {
        Command cmd;
        Serial.println("fifo available");
        uint32_t value = rp2040.fifo.pop();

        mutex_enter_blocking(&i2cCommandMtx);
        cmd = command;
        mutex_exit(&i2cCommandMtx);

        switch (cmd.commandType)
        {
            case CommandType::On:
            {
                Serial.println("Switching to on");
                // Go back to running the current color and pattern
                for (int i = 0; i < PinConstants::LED::NumPorts; i++)
                {
                    zones[i]->reset();
                }
                systemOn = true;
                break;
            }

            case CommandType::Off:
            {
                Serial.println("Switching to off");
                // Set LEDs to black and stop running the pattern
                for (uint8_t port = 0; port < PinConstants::LED::NumPorts; port++)
                {
                    auto pixels = getPixels(ledPort);
                    Animation::executePatternSetAll(pixels, 0, 0, FastLED[port].size());
                    FastLED[port].showLeds();
                }
                systemOn = false;
                break;
            }

            case CommandType::Pattern:
            {
                Serial.println("Pattern");
                // To set everything to a certain color, change color then call
                // the 'set all' pattern
                CommandPattern data = cmd.commandData.commandPattern;

                uint16_t delay =
                    data.delay == -1
                        ? zones[ledPort]->getPattern(data.pattern)
                            ->changeDelayDefault
                        : data.delay;

                zones[ledPort]->setPattern(data.pattern,
                                        delay,
                                        data.oneShot);
                break;
            }

            case CommandType::ChangeColor:
            {
                CommandColor data = cmd.commandData.commandColor;

                Serial.printf("Color=%d|%d|%d\n", data.red, data.green, data.blue);

                // Handle port 0 being RGB instead of GRB
                if (ledPort == 0)
                {
                    uint8_t temp = data.red;
                    data.red = data.green;
                    data.green = temp;
                }

                zones[ledPort]->setColor(
                    (uint32_t)CRGB(data.red, data.green, data.blue));
                break;
            }

            case CommandType::SetPatternZone:
            {
                CommandSetPatternZone data = cmd.commandData.commandSetPatternZone;

                zones[ledPort]->setRunZone(data.zoneIndex, data.reversed);
                Serial.printf("Pattern zone index=%u, reversed=%d\r\n",
                    data.zoneIndex, data.reversed);
                break;
            }

            case CommandType::SetNewZones:
            {
                CommandSetNewZones data = cmd.commandData.commandSetNewZones;
                auto* zoneDefs = new std::vector<ZoneDefinition>();

                Serial.printf("Seting up %d new zones\r\n", data.zoneCount);

                for (uint8_t i = 0; i < data.zoneCount; i++)
                {
                    zoneDefs->push_back(ZoneDefinition(data.zones[i]));
                    Serial.printf("Zone: %s\r\n", zoneDefs->at(i).toString().c_str());
                }

                auto& ledConfig = ledPort == 0 ? config.led0 : config.led1;

                zones[ledPort] = std::make_unique<PatternZone>(
                    ledPort, ledConfig.brightness, pixels[ledPort], zoneDefs);

                Serial.printf("Set %d new zones\r\n", data.zoneCount);

                break;
            }

            case CommandType::SyncStates:
            {
                CommandSyncZoneStates data = cmd.commandData.commandSyncZoneStates;

                zones[ledPort]->resetZones(data.zones, data.zoneCount);
                Serial.printf("Reset %d zones\r\n", data.zoneCount);
                break;
            }
        }
    }

    if (systemOn)
    {
        // zones[1]->updateZones();
        for (int i = 0; i < PinConstants::LED::NumPorts; i++)
        {
            // Serial.printf("Updating leds for port=%d\r\n", i);
            zones[i]->updateZones();
        }
    }
}

CRGB *getPixels(uint8_t port)
{
    if (port < PinConstants::LED::NumPorts)
    {
        return pixels[port];
    }

    return pixels[PinConstants::LED::DefaultPort];
}

// PatternRunner *getPatternRunner(uint8_t port)
// {
//     if (port < PinConstants::LED::NumPorts)
//     {
//         return patternRunners[port];
//     }

//     return patternRunners[PinConstants::LED::DefaultPort];
// }

void loop()
{
    // If there's new data, process it
    if (newDataToParse)
    {
        Serial.printf("Got new data\n");
        uint8_t buf[PinConstants::I2C::ReceiveBufSize];
        // Copy our new data
        memcpy(buf, (const void *)receiveBuf, PinConstants::I2C::ReceiveBufSize);
        Serial.printf("Rec data buffer=\t");
        for (int i = 0; i < 16; i++)
        {
            Serial.printf("%X ", receiveBuf[i]);
        }
        Serial.println();
        Command cmdTemp;
        CommandParser::parseCommand(buf, PinConstants::I2C::ReceiveBufSize, &cmdTemp);

        mutex_enter_blocking(&i2cCommandMtx);
        command = cmdTemp;
        mutex_exit(&i2cCommandMtx);

        newDataToParse = false;
        newData = true;
    }

    if (newData)
    {
        newData = false;
        // Serial.printf("Command struct=\t");
        // for (int i = 0; i < sizeof(command); i++)
        // {
        //     Serial.printf("%X ", ((uint8_t *)&command)[i]);
        // }
        // Serial.println();
        switch (command.commandType)
        {
        case CommandType::On:
        {
            // Go back to running the current color and pattern
            rp2040.fifo.push(0xFE010000);
            break;
        }

        case CommandType::Off:
        {
            // Set LEDs to black and stop running the pattern
            rp2040.fifo.push(0xFE010000);
            break;
        }

        case CommandType::Pattern:
        {
            // To set everything to a certain color, change color then call
            // the 'set all' pattern
            Serial.println("pattern pushing to fifo");
            rp2040.fifo.push(0xFE030000);
            break;
        }

        case CommandType::ChangeColor:
        {
            Serial.println("color pushing to fifo");
            rp2040.fifo.push(0xFE020000);
            break;
        }

        case CommandType::SetLedPort:
        {
            ledPort = command.commandData.commandSetLedPort.port;
            break;
        }

        case CommandType::DigitalSetup:
        {
            auto cfg = command.commandData.commandDigitalSetup;
            auto pin = PinConstants::DIGITALIO::digitalIOMap.at(cfg.port);
            pinMode(pin, cfg.mode);
            break;
        }

        case CommandType::DigitalWrite:
        {
            auto cfg = command.commandData.commandDigitalWrite;
            auto pin = PinConstants::DIGITALIO::digitalIOMap.at(cfg.port);
            digitalWrite(pin, cfg.value);
            break;
        }

        case CommandType::SetConfig:
        {
            config = command.commandData.commandSetConfig.config;
            Serial.println("Storing new config=");
            Serial.println(configurator.toString(config).c_str());
            configurator.storeConfig(config);
            break;
        }

#ifdef ENABLE_RADIO
        case CommandType::RadioSend:
        {
            auto message = command.commandData.commandRadioSend.msg;
            if (message.teamNumber == Radio::SendToAll)
            {
                radio->sendToAll(message);
            }
            else
            {
                radio->send(message);
            }
        }
#endif

        case CommandType::SetPatternZone:
        {
            rp2040.fifo.push(0xFE020000);
            break;
        }

        case CommandType::SetNewZones:
        {
            rp2040.fifo.push(0xFE020000);
            break;
        }

        case CommandType::SyncStates:
        {
            rp2040.fifo.push(0xFE020000);
            break;
        }

        default:
            break;
        }

        Serial.print(F("ON="));
        Serial.println(systemOn);
    }

#ifdef ENABLE_RADIO
    radio->update();
#endif
}

void handleRadioDataReceive(Message msg)
{
    mutex_enter_blocking(&radioDataMtx);
    Serial.println("New data:");
    Serial.printf("Team = %d\n", msg.teamNumber);
    Serial.printf("Data len = %d\n", msg.len);
    Serial.print("Data (HEX) = ");

    for (uint8_t i = 0; i < msg.len; i++)
    {
        Serial.printf("%02X ", msg.data[i]);
    }

    Serial.println();

    mutex_exit(&radioDataMtx);
}

void receiveEvent(int howMany)
{
    Wire.readBytes((uint8_t *)receiveBuf, howMany);
    newDataToParse = true;
}

void requestEvent()
{
    Response res;
    res.commandType = command.commandType;
    // Serial.printf("Sending back command=%d\n", (uint8_t)res.commandType);

    switch (command.commandType)
    {
    /**
     * @brief Returns uint8_t
     *
     */
    case CommandType::ReadPatternDone:
    {
        // auto done = getPatternRunner(ledPort)->patternDone();
        res.responseData.responsePatternDone.done = false;
        break;
    }

    /**
     * @brief Returns uint8_t
     *
     */
    case CommandType::DigitalRead:
    {
        uint8_t value = digitalRead(PinConstants::DIGITALIO::digitalIOMap.at(
            command.commandData.commandDigitalRead.port));
        res.responseData.responseDigitalRead.value = value;
        break;
    }

#ifdef ENABLE_RADIO
    case CommandType::RadioGetLatestReceived:
    {
        mutex_enter_blocking(&radioDataMtx);
        auto msg = radio->getLastReceived();
        mutex_exit(&radioDataMtx);

        res.responseData.responseRadioLastReceived.msg = msg;
        break;
    }
#endif

    case CommandType::ReadConfig:
    {
        auto cfg = configurator.readConfig();
        res.responseData.responseReadConfiguration.config = cfg;
        break;
    }

    case CommandType::GetColor:
    {
        // auto runner = getPatternRunner(ledPort);
        // uint32_t msg = runner->getCurrentColor();

        res.responseData.responseReadColor.color = 0x00;
        break;
    }

    case CommandType::GetPort:
    {
        res.responseData.responseReadPort.port = ledPort;
        break;
    }

    default:
        // Send back 255 (-1 signed) to indicate bad/no data
        Wire.write(0xff);
        return;
    }

    centralRespond(res);
}

void centralRespond(Response response)
{
    uint16_t size;

    switch (response.commandType)
    {
    case CommandType::ReadPatternDone:
        size = sizeof(ResponsePatternDone);
        break;
    case CommandType::DigitalRead:
        size = sizeof(ResponseDigitalRead);
        break;
    case CommandType::RadioGetLatestReceived:
        size = sizeof(ResponseRadioLastReceived);
        break;
    case CommandType::ReadConfig:
        size = sizeof(ResponseReadConfiguration);
        break;
    case CommandType::GetColor:
        size = sizeof(ResponseReadColor);
        break;
    case CommandType::GetPort:
        size = sizeof(ResponseReadPort);
        break;
    default:
        size = 0;
    }

    Wire.write((uint8_t)response.commandType);
    Wire.write((uint8_t *)&response.responseData, size);
}

void initI2C0(void)
{
    pinMode(PinConstants::I2C::Port0::AddrSwPin0, INPUT_PULLUP);
    uint8_t addr0Val = digitalRead(PinConstants::I2C::Port0::AddrSwPin0) == HIGH ? 1 : 0;
    pinMode(PinConstants::I2C::Port0::AddrSwPin1, INPUT_PULLUP);
    uint8_t addr1Val = digitalRead(PinConstants::I2C::Port0::AddrSwPin1) == HIGH ? 1 : 0;
    pinMode(PinConstants::I2C::Port0::AddrSwPin2, INPUT_PULLUP);
    uint8_t addr2Val = digitalRead(PinConstants::I2C::Port0::AddrSwPin2) == HIGH ? 1 : 0;

    uint8_t i2cAddress =
        PinConstants::I2C::Port0::BaseAddress |
        (addr2Val << 2) |
        (addr1Val << 1) |
        (addr0Val);

    Serial.printf("I2C address DEC=%d\n", i2cAddress);

    Wire.setSDA(PinConstants::I2C::Port0::SDA);
    Wire.setSCL(PinConstants::I2C::Port0::SCL);
    Wire.onReceive(receiveEvent); // register events
    Wire.onRequest(requestEvent);
    Wire.begin(i2cAddress); // join i2c bus as slave
}

void initPixels(LedConfiguration *config, uint8_t port)
{
    Serial.println("Pixel start");
    auto* strip = new CRGB[config->count];
    pixels[port] = strip;

    if (port == 0)
    {
        FastLED.addLeds<WS2812, PinConstants::LED::Dout0, RGB>(strip, config->count);
    }
    else if (port == 1)
    {
        FastLED.addLeds<WS2812, PinConstants::LED::Dout1, GRB>(strip, config->count);
    }

    // TODO: Make the # of zones configurable
    zones[port] = std::make_unique<PatternZone>(port, config->brightness, strip, config->count, 4);

    Serial.printf("zones size=%d\r\n", zones[port]->_zones->size());

    FastLED[port].clearLedData();
    strip[0] = CRGB(255, 127, 31);
    FastLED[port].showLeds();
    delay(1000);
    // Initialize all LEDs to black
    Animation::executePatternSetAll(strip, 0, 0, FastLED[port].size());
    FastLED[port].showLeds();
    Serial.println("Pixel end");
}