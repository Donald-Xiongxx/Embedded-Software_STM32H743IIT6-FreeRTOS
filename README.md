# STM32H743IITx + FreeRTOS Embedded Project

## Copyright & Usage Terms
Copyright (c) [2026] [Donald-Xiongxx]. All rights reserved.  

his project is for **non-commercial use only**; 
modification and redistribution are permitted, with this original copyright notice retained in all copies.

## Project Info

This repository demonstrates how to port **FreeRTOS** to an **STM32H743IITx**-based project.

At the time of project creation, the *ARM_CM7* FreeRTOS port was not available in *STM32CubeMX*. 
Therefore, this project can also be initiated from a bare-metal STM32H743 project as an alternative approach.

### Key Specifications
- **MCU Model**: STM32H743IIT6
- **RTOS**: FreeRTOS (v11.1.0, https://github.com/FreeRTOS/FreeRTOS-Kernel/tree/V11.1.0)
- **Porting Note**: ported from `portable/GCC/` for ARM Compiler 6 (Keil MDK)
- **Development Env**: Keil *MDK-ARM* v5.43a, *STM32CubeMX* v6.16.1
- **Core Library**: STM32H7xx HAL Library

## 中文简介

本仓库展示了如何将 **FreeRTOS** 实时操作系统移植到基于 **STM32H743IITx** 系列芯片的项目中。

在本项目创建时，*STM32CubeMX* 尚未提供适配该芯片的 *ARM_CM7* 架构 FreeRTOS 移植层。
因此，本项目直接从 STM32H743 裸机工程出发，完成 FreeRTOS 的移植与适配，已在Keil中编译通过。