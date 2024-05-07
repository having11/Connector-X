# Connector-X

A drop-in, highly-configurable IO board compatible with the NI/FRC RoboRIO

- [Connector-X](#connector-x)
  - [The concept](#the-concept)
  - [Hardware features](#hardware-features)
  - [Attaching to your RIO](#attaching-to-your-rio)
  - [What are ports and zones?](#what-are-ports-and-zones)
  - [It can show images](#it-can-show-images)
  - [Command structure](#command-structure)
  - [Test sequences](#test-sequences)
  - [Expansion](#expansion)

## The concept

Have you ever wanted to leverage the power and extensibility of the Arduino ecosystem on other hardware but realized it's not supported? This was the case for FRC team 5690 SubZero Robotics in the 2023 when we were attempting to add sensors and LEDs to our robot. Born from this struggle was a very simple Arduino Nano-based board that supported a single LED strip, limited LED patterns, and a duty-cycle output for lidar. As one might imagine, this was quite limited, so the 2023 off-season was spent designing and then redesigning a brand new system called the Connector-X (Connect Anything).

The new system is meant to be far faster and more flexible, as its Raspberry Pi Pico processor allows it to act as both a peripheral for a RoboRIO and a central device for connecting sensors.

## Hardware features

Starting along the top of the Connector-X is a pair of buttons that allow for reconfiguring the device and resetting it if needed. The DIP switch to the right was used to set the I2C address, but recent changes to the firmware have made it unnecessary. Lastly, the 3.3V regulator and Schottky diode help regulate incoming power and prevent damage if the inputs are accidentally reversed.

The DC barrel jack is meant for connecting 5V, center-positive power from a buck converter or other source that can supply a sufficient amount of current (> 5A preferred). Two LED ports sit below it, and each one has a starting, test WS2812B LED to help debug LED strip problems. The center has headers for 1 x SPI, 2 x I2C, and 3 x GPIO. Status indicators below the Pico display Power, Pico functionality, and I2C activity.

## Attaching to your RIO

At the bottom of the board is a male 2 x 17 header and a female 2 x 17 header, both of which conform to and pass-through the RIO's MXP connector standard. The Connector-X itself is only electrically connected to the I2C `SDA`, I2C `SCL`, and `GND` pins, meaning that a power issue on the board will not affect the RIO.

## What are ports and zones?

As mentioned before, the two ports each connect to a single LED strip, but that leads to the question of how different patterns can be easily shown on the same strip. So rather than connecting a strip per-port, the Connector-X allows for the strip to be subdivided into virtual zones that range from a single pixel all the way to the entire strip's length. To specify one, simply declare the offset (inclusive) and the size of the zone while ensuring it stays within the total number of LEDs present in the strip. The controlling device then sends commands to select the desired port and zone by index when settings colors/patterns.

## It can show images

Within the `Configuration`, either a `strip` or `matrix` configuration can be set. In the case of a `matrix`, the port is automatically split into two zones with the first being the single test LED and the second as the matrix itself. Currently, there are seven patterns that make use of the matrix, and they generally work by accessing images as follows:

1. A folder is created within the [data](./connector_x/data/) directory
2. 16-bit RGB bitmaps are placed into the folder. Names should be numeric, start at 0, and have the exact same number of images as the number of states specified in the [Pattern configuration](./connector_x/include/Patterns.h)
3. Selecting the pattern from a controlling device causes the Connector-X to cycle through them with the delay specified in the config or the command

## Command structure

## Test sequences

## Expansion

V3 coming soon...
