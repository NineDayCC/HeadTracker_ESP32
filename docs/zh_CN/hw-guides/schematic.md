# 原理图

:::{note}
获取工程文件，请跳转至[立创开源广场](http://oshwhub.com/nineday/headtracker_esp32-nano-wu-xian-tou-zhui)
:::

## Nano 发射端

### 图纸

```{image} ../../_static/schematic_HT_nano.png
:alt: schematic_HT_nano
:class: bg-primary
:width: 100%
:align: center
```

### 引脚


:::{table} 引脚连接表
:name: io-list
:width: 450px
:widths: auto
:align: center

| 名称 | No. | 网络 | 功能 |
| :---: | :---: | :---: | :---: |
| GPIO25 | 14 | BUZZ | 蜂鸣器控制输出 |
| GPIO27 | 16 | TOUCH | 电容触摸输入 |
| GPIO14 | 17 | LED | 指示灯输出 |
| GPIO13 | 20 | SDO | IMU SPI MISO |
| GPIO15 | 21 | SDI | IMU SPI MOSI |
| GPIO2 | 22 | SCK | IMU SPI SCK |
| GPIO0 | 23 | CS | IMU SPI CS / 烧录短接口 |
| GPIO20 | 27 | BUTTON | 微动开关输入 |
| U0RXD | 40 | RX | 烧录串口RX |
| U0TXD | 41 | TX | 烧录串口TX |
:::

:::{note}
ESP32/ESP8266 通过串口进行烧录时，需要将烧录短接口与GND短接。
:::

## Nano 接收端

### 图纸

```{image} ../../_static/schematic_Receiver_ppm.png
:alt: schematic_Receiver_ppm
:class: bg-primary
:width: 100%
:align: center
```

### 引脚

:::{table} 引脚连接表
:width: 450px
:widths: auto
:align: center

| 名称 | No. | 网络 | 功能 |
| :---: | :---: | :---: | :---: |
| GPIO12 | 6 | PPM | PPM信号输出 |
| GPIO13 | 7 | BIND | 微动开关输入 |
| GPIO0 | 12 | DOWNLOAD | 烧录短接口 |
| GPIO4 | 13 | LED | 指示灯输出 |
| RXD | 15 | RX | 烧录串口RX |
| TXD | 16 | TX | 烧录串口TX |
:::
