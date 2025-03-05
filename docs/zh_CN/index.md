---
sd_hide_title: true
---

# 🔎概览

# HeadTracker_ESP32 文档

(welcome)=
## 欢迎使用本项目文档

感谢您对 HeadTracker_ESP32 项目的关注！本文档旨在帮助您快速掌握如何使用由 HeadTracker_ESP32 项目支持的头追，以及如何自制相应的硬件。

:::{note}
文档还在持续完善，欢迎[催更](support/contact.md)🕊️ 。
:::

:::{attention}
本项目还处在测试阶段，会有许多不确定的问题发生，使用时请注意安全！
:::

<!-- (intention)=
## 项目初衷

**你是否体验过这样的飞行时刻？**  
>当指尖轻推摇杆，FPV眼镜中的世界骤然倾斜——  
>穿越林梢时，座舱盖边缘流淌着熔金般的夕阳余晖；  
>高速滚转中，翼尖撕裂云层，将整片大地化作旋转的画布；  
>俯冲降落时，跑道在视野中急速扩张，甚至能听见起落架擦过草叶的细响...  

这就是第一人称航模飞行的魅力，而一个头追可以为你解锁更极致的维度，你将真正解锁全方位的沉浸感。

互联网上已有各路前辈的贡献的各种开源头追方案，有的甚至时间久远无法访问，它们各有各的优缺点和局限性。有的价格便宜但功能单一，有的则相反。

:::{admonition} 主旨
:class: tip
本项目旨在使用较低的成本，实现一些较新的功能，如无线连接功能。同时硬件和结构上追求即插即用，尽力摆脱各种接线和安装困扰，**提升头追的使用体验**，**降低头追的使用门槛**。
::: -->

(features)=
## 功能特点

:::{note}
目前仅有 Nano 版硬件，后续将会持续优化。也欢迎有兴趣的朋友一起交流开发！
:::

(nano-board)=
### Nano版
![Nano Tx]( ../_static/HT_Nano_front.jpg){.bg-warning w=300px align=center}  

**发射端**
- 大小仅有 15 mm * 15 mm * 8 mm 大小（不包含伸出的 Type-C 头）。
- **即插即用**，无需电池，无需提前安装魔术贴（需要有 Type-C 电源输出能力的视频眼镜，如 DJI G2，否则需要另外供电）。
- **双头 Type-C** ，头追使用时，不影响手机与眼镜的连接，依然可以通过手机观看图传画面（ DJI 系列）。
- 使用 [esp-now](https://www.espressif.com/zh-hans/solutions/low-power-solutions/esp-now) 议与接收端进行纯无线连接。
- 蜂鸣器提示音。
- 电容触摸按键。
- 短按回中，长按固定姿态。
- OTA 升级固件。

***

**接收端**
- 大小仅有 34 mm * 17 mm * 9 mm 大小（不包含天线长度）。
- 使用 3.5 mm 耳机线与遥控器教练口进行连接。
- 可共用遥控器上的 2s 电池进行供电，通过平衡头取电。
- PPM 信号输出。


```{toctree}
:hidden:
get-started
```

```{toctree}
:hidden:
:caption: 📖使用手册
:maxdepth: 2

getting-started/index
```

```{toctree}
:hidden:
:caption: 🛠️硬件
:maxdepth: 2

hw-guides/index
```

```{toctree}
:hidden:
:caption: 🤝支持
:maxdepth: 1

support/FAQ
support/download
support/contact
```