(get-started)=
# 🚀Getting Started

(preparation)=
## 1. Preparation

To use the head tracker supported by this project, you need to prepare the following items:

- **Head Tracker Transmitter**
- **Head Tracker Receiver**
- **3.5 mm Headphone Cable/Trainer Cable**

:::{seealso}
For specific requirements, please refer to [Hardware Requirements](getting-started/hardware-required.md)
:::

::::{note}
In addition to the items related to the head tracker in this project, you generally need some other accessories to achieve full head tracking functionality. This may vary depending on your equipment. Here are some references:

:::{admonition} Other Accessories Reference
:class: tip dropdown
- **Video Goggles** (e.g., DJI Goggles 2, DJI FPV Goggles V2, etc.)
- **Camera + Video Transmission** (e.g., DJI O3, DJI O4 Air Unit)
- **Gimbal** (e.g., Servo Gimbal for mounting the camera)
- **Remote Controller** (needs to support PPM trainer signal input)
- **Aircraft** (model aircraft for mounting the gimbal, usually a fixed-wing)
:::

::::


(update)=
## 2. Firmware Update

The head tracker supports [OTA](getting-started/flashing-firmware.md#ota) via WiFi for firmware updates.

You need a computer or mobile phone that can connect to WiFi.

By connecting to the hotspot created by the head tracker, you can upload the firmware to the head tracker to update it.

:::{attention}
Ensure that the transmitter and receiver versions are consistent when using the head tracker, otherwise it may not work properly.
:::


(install)=
## 3. Installation

#### Transmitter

For video goggles with Type-C power output, the head tracker transmitter can be **directly plugged into the Type-C port** to work.

```{image} _static/TX_install.jpg
:alt: TX_install
:class: bg-primary
:width: 50%
:align: center
```
:::{hint}
You can use a Type-C to Type-C cable to connect your phone to the video goggles and check if the phone shows charging to determine if there is power output.
:::

#### Receiver

The installation method for different remote controllers may vary.

Some remote controllers can place the receiver **inside the battery compartment**, sharing the 2s battery with the remote controller.  

Remote controllers with insufficient space need to choose other methods such as external mounting and power supply based on their own space.

:::{seealso}
For more installation methods, please refer to [Installation](getting-started/installing.md)
:::


(binding)=
## 4. Binding

The transmitter and receiver only need to be paired once, and no repeated pairing is required afterward (OTA does not affect the original pairing).

**Pairing method:**  
**1.** Power on the transmitter and receiver (order does not matter)  
**2.** Long press the micro switch on the transmitter until the buzzer starts beeping intermittently  
**3.** Long press the micro switch on the receiver to complete the pairing  

After successful pairing, the transmitter will emit a long beep.

:::{seealso}
For more information on pairing, please refer to [Binding](getting-started/binding.md)
:::

(setup)=
## 5. Remote Controller Setup

The setup method for each remote controller is different, but generally, you need to set the following points:
1. Enable PPM input on the remote controller trainer port and set the remote controller to 'Master'
2. Adjust the remote controller channels, mapping channels 6, 7, and 8 of the trainer channel to the channels you need

:::{note}
The axis corresponding to the channel will vary depending on the installation direction of different video goggles. You need to test and set your channels according to the actual movement direction.
:::

## 6. Usage

After setting up the remote controller configuration, simply plug the head tracker transmitter into the Type-C port of the video goggles to start using it.

### Centering
**Short press** the capacitive touch button area, accompanied by a *short beep*, to center.

### Locking
Sometimes you need to *pause motion analysis* and lock the current attitude angle output.  

At this time, you can **long press** the capacitive touch area, accompanied by a *long beep*, to lock the current angle.

To unlock, simply short press to trigger centering, and the lock will be canceled.

:::{attention}
After use, do not forget to turn off the receiver power to avoid long-term power consumption of the remote controller.
:::
