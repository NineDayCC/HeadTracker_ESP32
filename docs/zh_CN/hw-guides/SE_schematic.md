# SE 原理图

:::{note}
获取工程文件，请跳转至[立创开源广场](https://oshwhub.com/nineday/headtracker_esp32-se-ban-wu-xian-tou-zhui)
:::

## SE 发射端

### 图纸

```{image} ../../_static/schematic_HT_SE.png
:alt: schematic_HT_SE
:class: bg-primary
:width: 100%
:align: center
```

### 引脚


:::{table} 引脚连接表
:name: io-list-SE
:width: 450px
:widths: auto
:align: center

| 名称 | No. | 网络 | 功能 |
| :---: | :---: | :---: | :---: |
| GPIO1 | 5 | LED | 绿色 LED |
| GPIO3 | 8 | TOUCH | 电容触摸输入 |
| GPIO4 | 9 | CS | IMU SPI CS |
| GPIO5 | 10 | SCK | IMU SPI SCK |
| GPIO6 | 12 | SDO | IMU SPI MIS0 |
| GPIO7 | 13 | SDI | IMU SPI MOSI |
| GPIO9 | 15 | BOOT | 烧录短接口 |
| GPIO10 | 16 | BUZZ | 蓝色 LED |
| GPIO18 | 25 | D- | 微动开关输入 |
| GPIO19 | 26 | D+ | 微动开关输入 |
| U0RXD | 27 | RX | 烧录串口RX |
| U0TXD | 28 | TX | 烧录串口TX |
:::

## SE 接收端

### 图纸

```{image} ../../_static/schematic_RX_SE.png
:alt: schematic_RX_SE
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
| GPIO4 | 9 | LED | 绿色 LED |
| GPIO9 | 15 | BOOT | 烧录短接口 |
| GPIO10 | 16 | PPM | PPM 输出 |
| GPIO18 | 25 | D- | 微动开关输入 |
| GPIO19 | 26 | D+ | 微动开关输入 |
| U0RXD | 27 | RX | 烧录串口RX |
| U0TXD | 28 | TX | 烧录串口TX |
:::


