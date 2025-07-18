# SE Schematic

:::{note}
To obtain the EDA project files, please visit JLC's [oshwhub](https://oshwhub.com/nineday/headtracker_esp32-se-ban-wu-xian-tou-zhui).
:::

## SE Transmitter

### Diagram

```{image} ../../_static/schematic_HT_SE.png
:alt: schematic_HT_SE
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
| GPIO1   | 5   | LED    | Green LED               |
| GPIO3   | 8   | TOUCH  | Capacitive touch input  |
| GPIO4   | 9   | CS     | IMU SPI CS              |
| GPIO5   | 10  | SCK    | IMU SPI SCK             |
| GPIO6   | 12  | SDO    | IMU SPI MISO            |
| GPIO7   | 13  | SDI    | IMU SPI MOSI            |
| GPIO9   | 15  | BOOT   | Programming short interface |
| GPIO10  | 16  | BUZZ   | Blue LED                |
| GPIO18  | 25  | D-     | Micro switch input      |
| GPIO19  | 26  | D+     | Micro switch input      |
| U0RXD   | 27  | RX     | Programming serial RX   |
| U0TXD   | 28  | TX     | Programming serial TX   |
:::

## SE Receiver

### Diagram

```{image} ../../_static/schematic_RX_SE.png
:alt: schematic_RX_SE
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
| GPIO4   | 9   | LED    | Green LED               |
| GPIO9   | 15  | BOOT   | Programming short interface |
| GPIO10  | 16  | PPM    | PPM output              |
| GPIO18  | 25  | D-     | Micro switch input      |
| GPIO19  | 26  | D+     | Micro switch input      |
| U0RXD   | 27  | RX     | Programming serial RX   |
| U0TXD   | 28  | TX     | Programming serial TX   |
:::

