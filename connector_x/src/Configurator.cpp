#include "Configurator.h"

Configuration Configurator::begin()
{
    pinMode(PinConstants::CONFIG::ConfigSetupBtn, INPUT_PULLUP);
    pinMode(PinConstants::CONFIG::ConfigLed, OUTPUT);

    if (!checkIfValid() ||
        !digitalRead(PinConstants::CONFIG::ConfigSetupBtn))
    {
        digitalWrite(PinConstants::CONFIG::ConfigLed, HIGH);
        while (!Serial)
            ;
        delay(1000);
        Serial.println("Entering configurator...");
        createConfig();
    }

    digitalWrite(PinConstants::CONFIG::ConfigLed, LOW);

    return readConfig();
}

void Configurator::createConfig()
{
    Configuration config;

    while (Serial.available())
        Serial.read();

    Serial.println(
        "Enter your selections with line ending set to NO LINE ENDING only");
    Serial.print("Team number: ");
    while (!Serial.available())
        ;
    config.teamNumber = Serial.parseInt();
    Serial.println(config.teamNumber);
    Serial.print("LED0 count: ");
    while (!Serial.available())
        ;
    config.led0.count = Serial.parseInt();
    Serial.println(config.led0.count);
    Serial.print("LED0 brightness (1 - 100): ");
    while (!Serial.available())
        ;
    config.led0.brightness = Serial.parseInt();
    Serial.println(config.led0.brightness);
    Serial.print("LED1 count: ");
    while (!Serial.available())
        ;
    config.led1.count = Serial.parseInt();
    Serial.println(config.led1.count);
    Serial.print("LED1 brightness (1 - 100): ");
    while (!Serial.available())
        ;
    config.led1.brightness = Serial.parseInt();
    Serial.println(config.led1.brightness);

    Serial.println("Storing config:");
    Serial.println(toString(config).c_str());
    storeConfig(config);
    Serial.println("Done");
}

std::string Configurator::toString(Configuration config)
{
    std::string str = "LED 0:\n";
    str += "\tCount: " + std::to_string(config.led0.count) + "\n";
    str += "\tBrightness: " + std::to_string(config.led0.brightness) + "\n";
    str += "LED 1:\n";
    str += "\tCount: " + std::to_string(config.led1.count) + "\n";
    str += "\tBrightness: " + std::to_string(config.led1.brightness) + "\n";

    return str;
}