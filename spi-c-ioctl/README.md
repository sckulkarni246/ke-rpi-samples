# Code Samples - SPI on Raspberry Pi using IOCTL

These code samples for SPI loopback and driving an SPI-based OLED display are written for Raspberry Pi boards.
These were tested on Raspberry Pi 4 but should also work on other Raspberry Pi boards (NOT THE PICO!).

## Loopback

### Building

```shell
$ cd <directory containing src files>
$ gcc spi_sysfs_loopback.c -o spi_sysfs_loopback
```

### Hardware Connections

Connect externally pins 19 and 21 of the Raspberry Pi IO Connector - these are the MOSI and MISO pins respectively.
No other connections are needed.

### Running The Sample

```shell
$ ./spi_sysfs_loopback
```

## Driving OLED Display

### Building

```shell
$ cd <directory containing src files>
$ gcc font.c -o font.o -c
$ gcc oled_functions.c -o oled_functions.o -c
$ gcc oled_demo.c -o oled_demo -c
$ gcc oled_demo -o oled_demo.o oled_functions.o font.o 
```

### Hardware Connections

1. Connect externally pins 19, 23 and 24 of the Raspberry Pi IO Connector to the MOSI, SCK and Chip Select pins of the SSD1306 display. 
2. Connect externally pins 3 and 5 of the Raspberry Pi IO Connector to the CMD/Data lines and RESET line of the SSD1306 display.
3. Power the display as indicated in the display datasheet.

### Running The Sample

It is advised to run the example as elevated user since access to GPIO is being sought.

```shell
$ sudo ./oled_demo
```