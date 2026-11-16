# MiniRTOS-Monitor

> 基于自研 MiniRTOS 的实时运动状态监测与故障容错系统

![Platform](https://img.shields.io/badge/Platform-STM32F103-blue)
![Status](https://img.shields.io/badge/Status-Early%20Development-orange)
![Application](https://img.shields.io/badge/Application-Real--time%20Monitoring-green)

## 项目简介

MiniRTOS-Monitor 是基于 **[MiniRTOS 自研实时操作系统](https://github.com/JKlovin/MiniRTOS)** 规划的综合型嵌入式应用项目，目标硬件为韦东山 STM32F103 瑞士军刀开发板，配套 MPU6050、OLED 和 W25Q64 SPI Flash。

项目计划实现运动状态实时采集、数据处理与显示、故障事件持久化、串口监控，以及任务健康检测和异常恢复。重点不只在于功能实现，还在于利用真实多任务应用检验 MiniRTOS 的调度、队列、互斥、通知和超时机制。

> **当前进度（2026-11）**：仓库目前处于工程初始化阶段，已包含 STM32CubeMX 配置、Core、Drivers 和 Keil 工程目录。**MPU6050 / OLED / W25Q64 的驱动、应用任务与 MiniRTOS 内核尚未在当前仓库公开实现**；下面的应用架构和功能均为拟定开发目标，不代表已经完成。

## 硬件平台

| 设备 | 作用 | 计划接口 |
| --- | --- | --- |
| STM32F103C8T6 | 主控制器、运行 MiniRTOS | ARM Cortex-M3 |
| MPU6050 | 三轴加速度和三轴角速度采集 | I2C1 |
| OLED | 实时数据显示和系统状态 | I2C1（待确认具体模块与接线） |
| W25Q64 | 故障日志、关键事件保存 | SPI1 |
| UART | 调试、运行统计及故障信息上报 | USART1 |
| SWD | 程序下载与调试 | SWD |

当前提交的 `.ioc` 可确认 STM32F103C8Tx、72 MHz 系统时钟、USART1 和 TIM4 HAL 时间基准配置；**I2C1 / SPI1 目前尚未启用**。具体 GPIO 引脚、器件地址、片选和板上引脚复用关系，待结合实际板卡连线核对后固定在硬件配置文档中。

## 目标系统架构

```text
MPU6050 ──I2C──> Sensor Task ──采样队列──> Process Task
                                               |
                         +---------------------+------------------+
                         |                     |                  |
                    Display Task          Logger Task        Comm Task
                         |                     |                  |
                      OLED                 W25Q64             USART1

                      Monitor Task：任务心跳 / 队列状态 / 故障监测
                                      |
                             MiniRTOS 自研内核
              任务调度 / 队列 / 任务通知 / 互斥锁 / 超时机制
```

### 计划任务划分

| 任务 | 建议优先级（0–4） | 职责 | 初步周期 / 触发 |
| --- | ---: | --- | --- |
| Sensor Task | 4 | MPU6050 数据采集 | 20 ms，后续测量实际抖动 |
| Process Task | 3 | 滤波、状态与故障检测 | 等待采样队列 |
| Monitor Task | 3 | 任务心跳、运行状态监测 | 100 ms |
| Comm Task | 2 | 串口命令与数据上报 | 通知 / 队列驱动 |
| Display Task | 1 | OLED 显示状态 | 100–200 ms |
| Logger Task | 1 | 故障事件写入 Flash | 等待日志队列 |
| Idle Task | 0 | 系统空闲任务 | 始终就绪 |

任务优先级和周期均为**初始设计值**，后续根据 CPU 占用、采样抖动、I²C 传输时间和内存消耗进行调整。MPU6050 与 OLED 计划共享 I²C，总线访问需互斥，并控制整屏刷新对高优先级采样任务造成的影响。

## 计划实现的功能

- **实时采样**：获取 MPU6050 六轴原始数据，进行滤波、姿态或运动状态分析。
- **任务间通信**：使用 MiniRTOS 队列、任务通知与互斥锁，实现采集、处理、显示、日志任务解耦。
- **OLED 显示**：展示实时数据、运行状态和故障提示。
- **故障日志**：将事件编号、时间戳、故障类型及校验信息存储到 W25Q64。
- **系统健康监测**：检测传感器通信失败、队列拥塞、任务心跳超时等情况。
- **性能测试**：测量周期抖动、任务响应延迟和队列吞吐，检验 MiniRTOS 在实际负载下的表现。

## 开发路线

- [x] 创建 GitHub 仓库与 STM32CubeMX / Keil 初始工程
- [ ] 完成瑞士军刀硬件引脚及资源冲突核对
- [ ] 配置 I2C1、SPI1，分别验证 MPU6050、OLED、W25Q64
- [ ] 引入并验证 MiniRTOS 内核，运行至少两个任务
- [ ] 实现采样队列与处理任务
- [ ] 实现 OLED 显示及共享 I²C 总线保护
- [ ] 实现 W25Q64 故障日志读写与断电恢复测试
- [ ] 增加任务心跳、异常处理、运行统计
- [ ] 完成实时性测试、压力测试与演示视频

## 仓库结构（当前）

```text
MiniRTOS-Monitor/
├── Core/                       # STM32 核心程序
├── Drivers/                    # CMSIS / HAL
├── MDK-ARM/                    # Keil 工程
├── MiniRTOS-Monitor.ioc        # CubeMX 工程配置
├── .gitignore
└── keilkill.bat
```

后续计划增加 `MiniRTOS/`（内核副本）、`BSP/`（传感器、OLED、Flash 驱动）、`App/`（应用任务）和 `Docs/`（测试、引脚与性能文档）。

## 构建与运行（当前阶段）

```bash
git clone https://github.com/JKlovin/MiniRTOS-Monitor.git
```

在 STM32CubeMX 中打开 `MiniRTOS-Monitor.ioc`，或在 Keil MDK-ARM 中打开 `MDK-ARM/` 下的工程。目前仓库用于基础工程配置与后续外设移植，**尚不能直接运行完整监测系统**。请在每一阶段完成后更新对应构建步骤和运行截图。

## 与 MiniRTOS 内核仓库的协同

- 内核的通用功能与 Bug 修复优先在 [MiniRTOS](https://github.com/JKlovin/MiniRTOS) 中完成并验证。
- 本项目按已验证的内核提交版本同步代码，记录对应的 commit 哈希。
- 本仓库主要维护板级驱动、应用任务和综合测试，不将应用特有逻辑写入通用内核。

## 项目说明

本项目面向嵌入式系统与 RTOS 学习、实验和工程实践，尚处于开发阶段，不适用于未经验证的安全关键或生产部署场景。
