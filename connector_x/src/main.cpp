#define PICO_USE_STACK_GUARDS 1

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#include <pico/mutex.h>

#include <Adafruit_GFX.h>
#include <FastLED_NeoMatrix.h>
#include <LittleFS.h>

#include "CommandParser.h"
#include "Commands.h"
#include "Configurator.h"
#include "Configuration.h"
#include "Constants.h"
#include "PacketRadio.h"
#include "PatternZone.h"

#include <memory>

// Uncomment to enable radio module communications
// #define ENABLE_RADIO

static mutex_t i2cCommandMtx;
static mutex_t radioDataMtx;
static mutex_t commandMtx;

// Give Core1 8K of stack space
bool core1_separate_stack = true;

// Forward declarations
void receiveEvent(int);
void requestEvent(void);
void handleRadioDataReceive(Message msg);
void centralRespond(Response response);
void initI2C0(void);
void initPixels(uint8_t port);

CRGB *getPixels(uint8_t port);

static volatile uint8_t receiveBuf[PinConstants::I2C::ReceiveBufSize];
static volatile bool newData = false;
static volatile bool newDataToParse = false;

static volatile uint8_t ledPort = 0;

static CRGB *pixels[PinConstants::LED::NumPorts];
static std::unique_ptr<PatternZone> zones[PinConstants::LED::NumPorts];

#ifdef ENABLE_RADIO
static PacketRadio *radio;
#endif

static Command command;
static CommandDeque commandDequeue;

static volatile bool systemOn = true;

void setup()
{
    pinMode(PinConstants::LED::AliveStatus, OUTPUT);
    digitalWrite(PinConstants::LED::AliveStatus, HIGH);

    Wire1.setSDA(PinConstants::I2C::Port1::SDA);
    Wire1.setSCL(PinConstants::I2C::Port1::SCL);
    Wire1.begin();

    EEPROM.begin(PinConstants::CONFIG::EepromSize);

    Serial.begin(UartBaudRate);

    rp2040.idleOtherCore();
    // config = configurator.begin();

    // Peripherals
    delay(3000);
    initI2C0();
    // Serial.println("I20 init done");

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
    mutex_init(&commandMtx);

    LittleFSConfig cfg;
    cfg.setAutoFormat(false);
    LittleFS.setConfig(cfg);

    LittleFS.begin();

    pinMode(PinConstants::SPI::CS0, OUTPUT);
    digitalWrite(PinConstants::SPI::CS0, HIGH);

    for (auto pin : PinConstants::DIGITALIO::digitalIOMap)
    {
        pinMode(pin.second, OUTPUT);
        digitalWrite(pin.second, LOW);
    }

    // Serial.println("Initializing pixels");
    // TODO: make into a loop
    initPixels(0);
    initPixels(1);

    rp2040.resumeOtherCore();
    rp2040.restartCore1();

#ifdef ENABLE_RADIO
    radio = new PacketRadio(&SPI1, config, handleRadioDataReceive);

    for (int i = 0; i < 2; i++)
    {
        radio->addTeam(config.initialTeams[i]);
    }
#endif

    // Serial.printf("Got config:\r\n%s\r\n",
    //               Configurator::toString(config).c_str());
}

void setup1()
{
    delay(2000);
    // Serial.println("Setup1 has ended");
}

void loop1()
{
    mutex_enter_blocking(&commandMtx);
    bool available = commandDequeue.nextCommandAvailable();
    mutex_exit(&commandMtx);

    if (available)
    {
        Command cmd{};
        // Serial.printf("fifo available\n");
        mutex_enter_blocking(&commandMtx);
        int remaining = commandDequeue.getNextCommand(&cmd);
        mutex_exit(&commandMtx);

        // Serial.printf("remaining = %d\n", remaining);
        // Serial.printf("cmd type = %d\n", (uint8_t)cmd.commandType);

        if (remaining == -1) {
            // Serial.printf("ERROR: no actual commands to process\n");
            return;
        }

        switch (cmd.commandType)
        {
            case CommandType::On:
            {
                // Serial.println("Switching to on");
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
                // Serial.println("Switching to off");
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
                // Serial.println("Pattern");
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

                // Serial.printf("Color=%d|%d|%d\n", data.red, data.green, data.blue);

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

            case CommandType::SetLedPort:
            {
                ledPort = cmd.commandData.commandSetLedPort.port;
                break;
            }

            case CommandType::SetPatternZone:
            {
                CommandSetPatternZone data = cmd.commandData.commandSetPatternZone;

                zones[ledPort]->setRunZone(data.zoneIndex, data.reversed);
                // Serial.printf("Pattern zone index=%u, reversed=%d\r\n",
                //     data.zoneIndex, data.reversed);
                break;
            }

            case CommandType::SetNewZones:
            {
                CommandSetNewZones data = cmd.commandData.commandSetNewZones;
                auto* zoneDefs = new std::vector<ZoneDefinition>();

                // Serial.printf("Seting up %d new zones\r\n", data.zoneCount);

                for (uint8_t i = 0; i < data.zoneCount; i++)
                {
                    zoneDefs->push_back(ZoneDefinition(data.zones[i].offset, data.zones[i].count));
                    // Serial.printf("Zone: %s\r\n", zoneDefs->at(i).toString().c_str());
                }

                auto& ledConfig = ledPort == 0 ? configuration.led0 : configuration.led1;

                zones[ledPort] = std::make_unique<PatternZone>(
                    ledPort, ledConfig.brightness, pixels[ledPort], zoneDefs);

                // Serial.printf("Set %d new zones\r\n", data.zoneCount);

                break;
            }

            case CommandType::SyncStates:
            {
                CommandSyncZoneStates data = cmd.commandData.commandSyncZoneStates;

                zones[ledPort]->resetZones(data.zones, data.zoneCount);
                break;
            }
        }

        // Serial.print(F("ON="));
        // Serial.println(systemOn);
    }

    if (systemOn)
    {
        for (int i = 0; i < PinConstants::LED::NumPorts; i++)
        {
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
        uint8_t buf[PinConstants::I2C::ReceiveBufSize];
        // Copy our new data
        mutex_enter_blocking(&i2cCommandMtx);
        memcpy(buf, (const void *)receiveBuf, PinConstants::I2C::ReceiveBufSize);
        mutex_exit(&i2cCommandMtx);
        // Serial.printf("Rec data buffer=\t");
        // for (int i = 0; i < 16; i++)
        // {
        //     Serial.printf("%X ", receiveBuf[i]);
        // }
        // Serial.println();
        Command cmdTemp{};
        CommandParser::parseCommand(buf, PinConstants::I2C::ReceiveBufSize, &cmdTemp);

        command = cmdTemp;

        newDataToParse = false;
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
            mutex_enter_blocking(&commandMtx);
            commandDequeue.pushCommand(command);
            mutex_exit(&commandMtx);
            // Go back to running the current color and pattern
            break;
        }

        case CommandType::Off:
        {
            mutex_enter_blocking(&commandMtx);
            // Set LEDs to black and stop running the pattern
            commandDequeue.pushCommand(command);
            mutex_exit(&commandMtx);
            break;
        }

        case CommandType::Pattern:
        {
            mutex_enter_blocking(&commandMtx);
            // To set everything to a certain color, change color then call
            // the 'set all' pattern
            
            commandDequeue.pushCommand(command);
            mutex_exit(&commandMtx);
            break;
        }

        case CommandType::ChangeColor:
        {
            mutex_enter_blocking(&commandMtx);
            commandDequeue.pushCommand(command);
            mutex_exit(&commandMtx);
            break;
        }

        case CommandType::SetLedPort:
        {
            mutex_enter_blocking(&commandMtx);
            commandDequeue.pushCommand(command);
            mutex_exit(&commandMtx);
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
            // config = command.commandData.commandSetConfig.config;
            // // Serial.println("Storing new config=");
            // // Serial.println(configurator.toString(config).c_str());
            // configurator.storeConfig(config);
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
            mutex_enter_blocking(&commandMtx);
            commandDequeue.pushCommand(command);
            mutex_exit(&commandMtx);
            break;
        }

        case CommandType::SetNewZones:
        {
            mutex_enter_blocking(&commandMtx);
            commandDequeue.pushCommand(command);
            mutex_exit(&commandMtx);
            break;
        }

        case CommandType::SyncStates:
        {
            mutex_enter_blocking(&commandMtx);
            commandDequeue.pushCommand(command);
            mutex_exit(&commandMtx);
            break;
        }

        default:
            break;
        }
    }

#ifdef ENABLE_RADIO
    radio->update();
#endif
}

void handleRadioDataReceive(Message msg)
{
    mutex_enter_blocking(&radioDataMtx);
    // Serial.println("New data:");
    // Serial.printf("Team = %d\n", msg.teamNumber);
    // Serial.printf("Data len = %d\n", msg.len);
    // Serial.print("Data (HEX) = ");

    // for (uint8_t i = 0; i < msg.len; i++)
    // {
    //     Serial.printf("%02X ", msg.data[i]);
    // }

    // Serial.println();

    mutex_exit(&radioDataMtx);
}

void receiveEvent(int howMany)
{
    Wire.readBytes((uint8_t *)receiveBuf, howMany);
    newDataToParse = true;
}

void requestEvent()
{
    Response res{};
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
        // res.responseData.responseReadConfiguration.config = configuration;
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

    // Serial.printf("I2C address DEC=%d\n", i2cAddress);

    Wire.setSDA(PinConstants::I2C::Port0::SDA);
    Wire.setSCL(PinConstants::I2C::Port0::SCL);
    Wire.onReceive(receiveEvent); // register events
    Wire.onRequest(requestEvent);
    // TODO:
    Wire.begin(0x17); // join i2c bus as slave
}

void initPixels(uint8_t port)
{
    // Serial.println("Pixel start");
    // auto* strip = new CRGB[config->count];

    uint16_t ledCount;
    std::vector<ZoneDefinition> ledZones;
    LedConfiguration config;

    if (port == 0) {
        config = configuration.led0;
    } else {
        config = configuration.led1;
    }

    if (!config.isMatrix) {
        ledCount = config.strip.count;

        if (config.strip.zoneCount != -1) {
            for (uint8_t i = 0; i < config.strip.zoneCount; i++) {
                ledZones.push_back(config.strip.initialZones[i]);
            }
        } else {
            ledZones.push_back(ZoneDefinition{0, ledCount});
        }
    } else {
        ledCount = (config.matrix.width * config.matrix.height) + 1;

        ledZones.push_back(ZoneDefinition{0, 1});
        ledZones.push_back(ZoneDefinition{1, ledCount - 1});
    }

    auto* strip = new CRGB[ledCount];
    pixels[port] = strip;
    zones[port] = std::make_unique<PatternZone>(port, config.brightness, pixels[port], &ledZones);

    if (port == 0)
    {
        FastLED.addLeds<WS2812, PinConstants::LED::Dout0, RGB>(pixels[port], ledCount);
    }
    else
    {
        FastLED.addLeds<WS2812, PinConstants::LED::Dout1, GRB>(pixels[port], ledCount);
    }

    // Serial.printf("zones size=%d\r\n", zones[port]->_zones->size());

    FastLED[port].clearLedData();
    pixels[port][0] = CRGB(255, 127, 31);
    FastLED[port].showLeds();
    delay(1000);
    // Initialize all LEDs to black
    Animation::executePatternSetAll(pixels[port], 0, 0, FastLED[port].size());
    FastLED[port].showLeds();
    // Serial.println("Pixel end");
}