---
sd_hide_title: true
---

# 🔎Overview

# HeadTracker_ESP32 Documentation

(welcome)=
## Welcome to the project

Thank you for your interest in the HeadTracker_ESP32 project! This documentation aims to help you quickly learn how to use the head tracker supported by the HeadTracker_ESP32 project and how to make the hardware yourself.

:::{note}
The documentation is still being improved, feel free to [urge updates](support/contact.md)🕊️.
:::

:::{attention}
This project is still in the testing phase, and many uncertain issues may occur. Please pay attention to safety when using it!
:::

<!-- (intention)=
## Project Intention

**Have you ever experienced such flying moments?**  
>When the fingertips gently push the joystick, the world in the FPV goggles suddenly tilts—  
>When flying through the treetops, the edge of the canopy flows with molten gold-like sunset glow;  
>During high-speed rolls, the wingtip tears through the clouds, turning the entire earth into a rotating canvas;  
>When diving to land, the runway rapidly expands in the field of vision, and you can even hear the faint sound of the landing gear brushing against the grass...

This is the charm of first-person model flying, and a head tracker can unlock a more extreme dimension for you, allowing you to truly unlock a full range of immersion.

There are already various open-source head tracker solutions contributed by predecessors on the internet, some of which are even so old that they are inaccessible. Each has its own advantages and limitations. Some are cheap but have limited functionality, while others are the opposite.

:::{admonition} Main Purpose
:class: tip
This project aims to achieve some newer features at a lower cost, such as wireless connectivity. At the same time, the hardware and structure strive for plug-and-play, trying to eliminate various wiring and installation troubles, **improving the head tracker usage experience**, and **lowering the threshold for using head trackers**.
::: -->

(features)=
## Features

:::{note}
Currently, only the Nano version hardware is available, and it will continue to be optimized in the future. Friends who are interested are also welcome to join the development!
:::

(nano-board)=
### Nano Version
![Nano Tx]( ../_static/HT_Nano_front.jpg){.bg-warning w=300px align=center}  

**Transmitter**
- Size is only 15 mm * 15 mm * 8 mm (excluding the protruding Type-C head).
- **Plug and play**, no battery required, no need to pre-install Velcro (requires video goggles with Type-C power output capability, such as DJI G2, otherwise additional power supply is needed).
- **Dual Type-C** heads, using the head tracker does not affect the connection between the phone and the goggles, you can still watch the video transmission through the phone (DJI series).
- Uses [esp-now](https://www.espressif.com/en/solutions/low-power-solutions/esp-now) protocol for pure wireless connection with the receiver.
- Buzzer sound prompts.
- Capacitive touch button.
- Short press to center, long press to fix the attitude.
- OTA firmware upgrade.

***

**Receiver**
- Size is only 34 mm * 17 mm * 9 mm (excluding antenna length).
- Uses a 3.5 mm headphone cable to connect to the trainer port of the remote controller.
- Can share the 2s battery on the remote controller for power supply, drawing power through the balance head.
- PPM signal output.

## Acknowledgements

This project draws inspiration from the following projects, with special thanks to:

1. [dlktdr/HeadTracker](https://github.com/dlktdr/HeadTracker)


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