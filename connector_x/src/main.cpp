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
#include "PatternRunner.h"

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
void initPixels(Adafruit_NeoPixel *pixels, LedConfiguration *config);

Adafruit_NeoPixel *getPixels(uint8_t port);
PatternRunner *getPatternRunner(uint8_t port);

static volatile uint8_t receiveBuf[Pin::I2C::ReceiveBufSize];
static volatile bool newData = false;
static volatile bool newDataToParse = false;

static Configuration config;
static Configurator configurator;
static uint8_t ledPort = 0;

static Adafruit_NeoPixel *pixels[Pin::LED::NumPorts];
static PatternRunner *patternRunners[Pin::LED::NumPorts];

#ifdef ENABLE_RADIO
static PacketRadio *radio;
#endif

static Command command;

static bool systemOn = true;

void setup()
{
    pinMode(Pin::LED::AliveStatus, OUTPUT);
    digitalWrite(Pin::LED::AliveStatus, HIGH);

    Wire1.setSDA(Pin::I2C::Port1::SDA);
    Wire1.setSCL(Pin::I2C::Port1::SCL);
    Wire1.begin();

    EEPROM.begin(Pin::CONFIG::EepromSize);

    Serial.begin(UartBaudRate);

    config = configurator.begin();

    pixels[0] = new Adafruit_NeoPixel(config.led0.count, Pin::LED::Dout0,
                                      NEO_GRB + NEO_KHZ800);
    pixels[1] = new Adafruit_NeoPixel(config.led1.count, Pin::LED::Dout1,
                                      NEO_GRB + NEO_KHZ800);
    patternRunners[0] = new PatternRunner(pixels[0], Animation::patterns);
    patternRunners[1] = new PatternRunner(pixels[1], Animation::patterns);

    // Peripherals
    delay(1000);
    initI2C0();

    SPI1.setTX(Pin::SPI::MOSI);
    SPI1.setRX(Pin::SPI::MISO);
    SPI1.setSCK(Pin::SPI::CLK);
    SPI1.setCS(Pin::SPI::CS0);
    SPI1.begin();

    Serial1.setTX(Pin::UART::TX);
    Serial1.setRX(Pin::UART::RX);
    Serial1.begin(UartBaudRate);

    mutex_init(&i2cCommandMtx);
    mutex_init(&radioDataMtx);

    pinMode(Pin::SPI::CS0, OUTPUT);
    digitalWrite(Pin::SPI::CS0, HIGH);

    for (auto pin : Pin::DIGITALIO::digitalIOMap)
    {
        pinMode(pin.second, OUTPUT);
        digitalWrite(pin.second, LOW);
    }

    initPixels(pixels[0], &config.led0);
    initPixels(pixels[1], &config.led1);

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

Adafruit_NeoPixel *getPixels(uint8_t port)
{
    if (port < Pin::LED::NumPorts)
    {
        return pixels[port];
    }

    return pixels[Pin::LED::DefaultPort];
}

PatternRunner *getPatternRunner(uint8_t port)
{
    if (port < Pin::LED::NumPorts)
    {
        return patternRunners[port];
    }

    return patternRunners[Pin::LED::DefaultPort];
}

void loop()
{
    // If there's new data, process it
    if (newDataToParse)
    {
        Serial.printf("Got new data\n");
        uint8_t buf[Pin::I2C::ReceiveBufSize];
        // Safely copy our new data
        noInterrupts();
        memcpy(buf, (const void *)receiveBuf, Pin::I2C::ReceiveBufSize);
        interrupts();
        CommandParser::parseCommand(buf, Pin::I2C::ReceiveBufSize, &command);

        newDataToParse = false;
        newData = true;
    }

    if (newData)
    {
        newData = false;
        for (int i = 0; i < sizeof(command); i++)
        {
            Serial.printf("%X ", ((uint8_t *)&command)[i]);
        }
        Serial.println();
        switch (command.commandType)
        {
        case CommandType::On:
        {
            // Go back to running the current color and pattern
            for (int i = 0; i < Pin::LED::NumPorts; i++)
            {
                patternRunners[i]->reset();
            }
            systemOn = true;
            break;
        }

        case CommandType::Off:
        {
            // Set LEDs to black and stop running the pattern
            for (uint8_t port = 0; port < 2; port++)
            {
                auto pixels = getPixels(ledPort);
                Animation::executePatternSetAll(*pixels, 0, 0, pixels->numPixels());
                pixels->show();
            }
            systemOn = false;
            break;
        }

        case CommandType::Pattern:
        {
            // To set everything to a certain color, change color then call
            // the 'set all' pattern
            auto runner = getPatternRunner(ledPort);
            uint16_t delay =
                command.commandData.commandPattern.delay == -1
                    ? runner->getPattern(command.commandData.commandPattern.pattern)
                          ->changeDelayDefault
                    : command.commandData.commandPattern.delay;

            runner->setCurrentPattern(command.commandData.commandPattern.pattern,
                                      delay,
                                      command.commandData.commandPattern.oneShot);
            break;
        }

        case CommandType::ChangeColor:
        {
            auto runner = getPatternRunner(ledPort);
            auto colors = command.commandData.commandColor;
            runner->setCurrentColor(
                Adafruit_NeoPixel::Color(colors.red, colors.green, colors.blue));
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
            auto pin = Pin::DIGITALIO::digitalIOMap.at(cfg.port);
            pinMode(pin, cfg.mode);
            break;
        }

        case CommandType::DigitalWrite:
        {
            auto cfg = command.commandData.commandDigitalWrite;
            auto pin = Pin::DIGITALIO::digitalIOMap.at(cfg.port);
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

        default:
            break;
        }

        Serial.print(F("ON="));
        Serial.println(systemOn);
    }

    if (systemOn)
    {
        for (int i = 0; i < Pin::LED::NumPorts; i++)
        {
            patternRunners[i]->update();
        }
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
        auto done = getPatternRunner(ledPort)->patternDone();
        res.responseData.responsePatternDone.done = done;
        break;
    }

    /**
     * @brief Returns uint8_t
     *
     */
    case CommandType::DigitalRead:
    {
        uint8_t value = digitalRead(Pin::DIGITALIO::digitalIOMap.at(
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
        auto runner = getPatternRunner(ledPort);
        uint32_t msg = runner->getCurrentColor();

        res.responseData.responseReadColor.color = msg;
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
    pinMode(Pin::I2C::Port0::AddrSwPin0, INPUT_PULLUP);
    uint8_t addr0Val = digitalRead(Pin::I2C::Port0::AddrSwPin0) == HIGH ? 1 : 0;
    pinMode(Pin::I2C::Port0::AddrSwPin1, INPUT_PULLUP);
    uint8_t addr1Val = digitalRead(Pin::I2C::Port0::AddrSwPin1) == HIGH ? 1 : 0;
    pinMode(Pin::I2C::Port0::AddrSwPin2, INPUT_PULLUP);
    uint8_t addr2Val = digitalRead(Pin::I2C::Port0::AddrSwPin2) == HIGH ? 1 : 0;

    uint8_t i2cAddress =
        Pin::I2C::Port0::BaseAddress |
        (addr2Val << 2) |
        (addr1Val << 1) |
        (addr0Val);

    Serial.printf("I2C address DEC=%d\n", i2cAddress);

    Wire.setSDA(Pin::I2C::Port0::SDA);
    Wire.setSCL(Pin::I2C::Port0::SCL);
    Wire.onReceive(receiveEvent); // register events
    Wire.onRequest(requestEvent);
    Wire.begin(i2cAddress); // join i2c bus as slave
}

void initPixels(Adafruit_NeoPixel *pixels, LedConfiguration *config)
{
    pixels->begin();
    pixels->setBrightness(config->brightness);
    pixels->setPixelColor(0, Adafruit_NeoPixel::Color(255, 127, 31));
    pixels->show();
    delay(1000);
    // Initialize all LEDs to black
    Animation::executePatternSetAll(*pixels, 0, 0, pixels->numPixels());
    pixels->show();
}