---
sd_hide_title: true
---

# 🔎Overview

# HeadTracker_ESP32 Documentation

> 点击左侧边栏的 [{fas}`language`](https://docs.nineday.cc/projects/headtracker-esp32/zh-cn/latest/index.html) 以切换至中文.

(welcome)=
## Welcome to the Project

Thank you for your interest in the HeadTracker_ESP32 project! This documentation is designed to help you quickly learn how to use the head tracker supported by the HeadTracker_ESP32 project and how to build the corresponding hardware yourself.

:::{note}
The documentation is still being improved. Feel free to [request updates](support/contact.md)🕊️.
:::

:::{attention}
This project is still in the testing phase, and many uncertainties may arise. Please use it with caution!
:::

(features)=
## Features

:::{note}
Currently, only the Nano version hardware is available. Future optimizations are planned. Developers and enthusiasts are welcome to join the discussion and development!
:::

(nano-board)=
### Nano Version

::::{grid} 1 2 2 2

:::{grid-item}
```{figure} ../_static/HT_Nano_side.png
:figwidth: 80 %
:alt: Nano Tx

Nano Transmitter
```
:::

:::{grid-item}
```{figure} ../_static/PCB_RX_top.jpg
:alt: Nano Receiver

Nano Receiver
```
:::

::::

**Transmitter**
- Compact size: only 15 mm * 15 mm * 8 mm (excluding the protruding Type-C connector).
- **Plug-and-play**, no battery required, no need to pre-install Velcro (requires video goggles with Type-C power output capability, such as DJI G2; otherwise, external power is needed).
- **Dual Type-C** design allows simultaneous use of the head tracker and connection between your phone and goggles, enabling you to **view the video feed on your phone** (DJI series).
- Uses [esp-now](https://www.espressif.com/en/solutions/low-power-solutions/esp-now) protocol for a fully wireless connection with the receiver.
- Built-in **buzzer** for audio feedback.
- Capacitive **touch button**.
- **Short press** to recenter, **long press** to lock the current orientation.
- **OTA** firmware updates.

***

**Receiver**
- Compact size: only 34 mm * 17 mm * 9 mm (excluding antenna length).
- Connects to the transmitter via a 3.5 mm **audio cable to the trainer port** of the remote controller.
- Can share power from the remote controller's 2s battery via the balance connector.
- Outputs **PPM** signal.

## Acknowledgments

This project draws inspiration from the following projects. Special thanks to:

1. [{fab}`github` dlktdr/HeadTracker](https://github.com/dlktdr/HeadTracker)


```{toctree}
:hidden:
get-started
```

```{toctree}
:hidden:
:caption: 📖User Manual
:maxdepth: 2

getting-started/index
```

```{toctree}
:hidden:
:caption: 🛠️Hardware
:maxdepth: 2

hw-guides/index
```

```{toctree}
:hidden:
:caption: 🤝Support
:maxdepth: 1

support/FAQ
support/download
support/contact
```