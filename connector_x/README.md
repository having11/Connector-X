# Connector-X

## Upload

To upload:

1. Head to the Platformio tab on the left extension pane.
2. Hold down the `BOOTSEL` button on the Pico
3. While holding the `BOOTSEL` button, click the `RST` button on the PCB
4. Release the `RST` button and THEN the `BOOTSEL` button in that order
5. Take note of the drive identifier that pops up
6. Set the `upload_port` value to that drive in [the project config file](platformio.ini)
7. Click `pico > General > Upload` in the Platformio tab
