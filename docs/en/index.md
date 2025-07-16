---
sd_hide_title: true
---

# 🔎Overview

# HeadTracker_ESP32 Documentation

> Click [{fas}`language`](https://docs.nineday.cc/projects/headtracker-esp32/en/latest/index.html) in the left sidebar to switch to English.

(welcome)=
## Welcome to the Project

Thank you for your interest in the HeadTracker_ESP32 project! This documentation is designed to help you quickly learn how to use the head tracker supported by the HeadTracker_ESP32 project and how to build the corresponding hardware yourself.

:::{note}
The documentation is still being improved. Feel free to [request updates](support/contact.md)🕊️.
:::

:::{attention}
This project is still in the testing phase, and many uncertainties may arise. Please use it with caution!
:::

<!-- (intention)=
## Project Intention

**Have you ever experienced such flying moments?**  
> When you gently push the stick, the world in your FPV goggles tilts suddenly—  
> As you skim the treetops, the edge of the canopy flows with golden sunset;  
> During high-speed rolls, the wingtips tear through clouds, turning the earth into a spinning canvas;  
> When diving to land, the runway rapidly expands in your view, and you can almost hear the landing gear brushing the grass...  

This is the charm of first-person model flying, and a head tracker can unlock an even more immersive dimension for you.

There are already many open-source head tracker solutions contributed by pioneers on the Internet, some of which are so old that they are no longer accessible. Each has its own advantages and limitations. Some are cheap but simple, others are the opposite.

:::{admonition} Purpose
:class: tip
This project aims to achieve some newer features, such as wireless connectivity, at a lower cost. The hardware and structure strive for plug-and-play, minimizing wiring and installation hassles, **improving the head tracker experience** and **lowering the usage threshold**.
:::
-->

(features)=
## Features

:::{note}
Currently, there are **Nano** and **SE** hardware versions. Further optimizations are planned. Developers and enthusiasts are welcome to join the discussion and development!
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
:alt: Nano Rx

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


(se-board)=
### SE Version

::::{grid} 1 2 2 2

:::{grid-item}
```{figure} ../_static/HT_SE_double.jpg
:figwidth: 91 %
:alt: SE Tx

SE Transmitter
```
:::

:::{grid-item}
```{figure} ../_static/RX_SE_with_box.jpg
:alt: SE Rx

SE Receiver
```
:::

::::

**Transmitter**
- Compact size: only 14.5 mm * 14.5 mm * 4 mm (excluding the protruding Type-C connector).
- **Plug-and-play**, no battery required, no need to pre-install Velcro (requires video goggles with Type-C power output capability, such as DJI G2; otherwise, external power is needed).
- Uses [esp-now](https://www.espressif.com/en/solutions/low-power-solutions/esp-now) protocol for a fully wireless connection with the receiver.
- Power on 3 times in a row to automatically enter **pairing mode** (if powered on for more than 2 seconds, the count will be cleared).
- Capacitive **touch button**.
- **Short press** to recenter, **long press** to lock the current orientation.
- **OTA** firmware updates. After startup, if it remains unconnected for 1 minute, it will automatically open a hotspot and enter OTA mode.

***

**Receiver**
- Compact size: only 23 mm * 23 mm * 9 mm.
- Connects to the remote controller via a 3.5 mm **audio cable to the trainer port**.
- Powered via **Type-C** port (4.5V-16V), can use an adapter to power from the high-frequency head interface in the JR bay.
- Power on 3 times in a row to automatically enter **pairing mode** (if powered on for more than 2 seconds, the count will be cleared).
- **OTA** firmware updates. After startup, if it remains unconnected for 1 minute, it will automatically open a hotspot and enter OTA mode.
- Outputs **PPM** signal.

:::{note}
SE hardware version is not fully released yet. Documentation will be updated later.
:::

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