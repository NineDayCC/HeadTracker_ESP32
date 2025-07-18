# Nano Schematic

:::{note}
To obtain the EDA project files, please visit JLC's [oshwhub](http://oshwhub.com/nineday/headtracker_esp32-nano-wu-xian-tou-zhui).
:::

## Nano Transmitter

### Diagram

```{image} ../../_static/schematic_HT_nano.png
:alt: schematic_HT_nano
:class: bg-primary
:width: 100%
:align: center
```

### Pins

:::{table} Pin Connection Table
:name: io-list-Nano
:width: 450px
:widths: auto
:align: center

| Name    | No. | Net    | Function                        |
| :---:   | :---: | :---: | :---:                          |
| GPIO25  | 14  | BUZZ   | Buzzer control output           |
| GPIO27  | 16  | TOUCH  | Capacitive touch input          |
| GPIO14  | 17  | LED    | Indicator light output          |
| GPIO13  | 20  | SDO    | IMU SPI MISO                    |
| GPIO15  | 21  | SDI    | IMU SPI MOSI                    |
| GPIO2   | 22  | SCK    | IMU SPI SCK                     |
| GPIO0   | 23  | CS     | IMU SPI CS / Programming short interface |
| GPIO20  | 27  | BUTTON | Micro switch input              |
| U0RXD   | 40  | RX     | Programming serial RX           |
| U0TXD   | 41  | TX     | Programming serial TX           |
:::

:::{note}
When programming the ESP32/ESP8266 via serial, the programming short interface needs to be connected to GND.
:::

## Nano Receiver

### Diagram

```{image} ../../_static/schematic_Receiver_ppm.png
:alt: schematic_Receiver_ppm
:class: bg-primary
:width: 100%
:align: center
```

### Pins

:::{table} Pin Connection Table
:width: 450px
:widths: auto
:align: center

| Name    | No. | Net    | Function                |
| :---:   | :---: | :---: | :---:                  |
| GPIO12  | 6   | PPM    | PPM signal output       |
| GPIO13  | 7   | BIND   | Micro switch input      |
| GPIO0   | 12  | DOWNLOAD | Programming short interface |
| GPIO4   | 13  | LED    | Indicator light output  |
| RXD     | 15  | RX     | Programming serial RX   |
| TXD     | 16  | TX     | Programming serial TX   |
:::
