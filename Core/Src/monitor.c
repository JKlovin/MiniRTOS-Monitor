
#include "monitor.h"

#include "main.h"
#include "oled.h"
#include "mpu6050.h"
#include "sensor_data.h"

#include "minirtos_task.h"
#include "minirtos_event.h"
#include "minirtos_queue.h"

#include <stdio.h>
#include <stdint.h>

/*---------------------------------------------
 * 配置参数
 *--------------------------------------------*/
#define SENSOR_PERIOD_TICK     50U
#define OLED_PERIOD_TICK       500U

/* 1：开启串口性能输出；0：关闭 */
#define MONITOR_DEBUG          1U

/*---------------------------------------------
 * 系统资源
 *--------------------------------------------*/
extern MiniQueue_t SensorQueue;
extern MiniMutex_t I2C_Mutex;

/* 最新一帧处理完成的数据 */
static SensorPacket_t LatestPacket;
static uint8_t LatestPacketValid = 0U;

/*---------------------------------------------
 * 运行统计
 *--------------------------------------------*/
typedef struct
{
    uint32_t SampleCount;
    uint32_t QueueDropCount;
    uint32_t OverrunCount;

    uint32_t MaxMutexWait;

    uint32_t MinActualInterval;
    uint32_t MaxActualInterval;

} MonitorStats_t;

static MonitorStats_t MonitorStats =
{
    .MinActualInterval = UINT32_MAX
};

/*---------------------------------------------
 * 获取统计信息快照
 *--------------------------------------------*/
static void Monitor_GetStats(MonitorStats_t *Stats)
{
    if (Stats == NULL)
    {
        return;
    }

    Mini_EnterCritical();

    *Stats = MonitorStats;

    Mini_ExitCritical();
}

/*---------------------------------------------
 * LED任务
 *--------------------------------------------*/
void LED_Handle(void *arg)
{
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        MiniRTOS_delay(5000U);
    }
}

/*---------------------------------------------
 * MPU6050传感器采样任务
 *--------------------------------------------*/
void Sensor_Handle(void *arg)
{
    SensorPacket_t Packet;
    HAL_StatusTypeDef Status;

    uint32_t LastWakeTime = MiniRTOS_GetTick();

    uint32_t WaitStart;
    uint32_t ActualTick;
    uint32_t MutexWait;

    uint32_t PreviousActualTick = 0U;
    uint32_t ActualInterval;

    uint8_t FirstSample = 1U;

    while (1)
    {
        /* 记录获取I2C互斥锁之前的时间 */
        WaitStart = MiniRTOS_GetTick();

        MutexSignal_Take(&I2C_Mutex);

        /* 记录开始访问传感器的时间 */
        ActualTick = MiniRTOS_GetTick();

        /* 读取MPU6050 */
        Status = MPU6050_ReadData(&Packet.Sensor);

        MutexSignal_Release(&I2C_Mutex);

        /* 统计I2C互斥锁最大等待时间 */
        MutexWait = ActualTick - WaitStart;

        if (MutexWait > MonitorStats.MaxMutexWait)
        {
            MonitorStats.MaxMutexWait = MutexWait;
        }

        if (Status == HAL_OK)
        {
            /* 统计实际读取开始时间的间隔 */
            if (FirstSample == 0U)
            {
                ActualInterval =
                    ActualTick - PreviousActualTick;

                if (ActualInterval <
                    MonitorStats.MinActualInterval)
                {
                    MonitorStats.MinActualInterval =
                        ActualInterval;
                }

                if (ActualInterval >
                    MonitorStats.MaxActualInterval)
                {
                    MonitorStats.MaxActualInterval =
                        ActualInterval;
                }
            }
            else
            {
                FirstSample = 0U;
            }

            PreviousActualTick = ActualTick;

            /* 生成传感器数据包 */
            Packet.Tick = ActualTick;

            Packet.Sequence =
                ++MonitorStats.SampleCount;

            /* 非阻塞队列发送 */
            if (Queue_TrySend(&SensorQueue, &Packet) == 0U)
            {
                MonitorStats.QueueDropCount++;
            }
        }

        /* 固定周期延时 */
        if (MiniRTOS_DelayUntil(
                &LastWakeTime,
                SENSOR_PERIOD_TICK) == 1U)
        {
            MonitorStats.OverrunCount++;
        }
    }
}

/*---------------------------------------------
 * 数据处理任务
 *--------------------------------------------*/
void Process_Handle(void *arg)
{
    SensorPacket_t Packet;
    MonitorStats_t Stats;

    uint32_t ReceiveCount = 0U;

    while (1)
    {
        /* 阻塞等待传感器数据 */
        if (Queue_Receive(&SensorQueue, &Packet) == 1U)
        {
            ReceiveCount++;

            /* 更新最新的数据快照 */
            Mini_EnterCritical();

            LatestPacket = Packet;
            LatestPacketValid = 1U;

            Mini_ExitCritical();

#if MONITOR_DEBUG
            /* 每接收20帧打印一次统计信息 */
            if ((ReceiveCount % 20U) == 0U)
            {
                Monitor_GetStats(&Stats);

                printf(
                    "Sample:%lu Drop:%lu Overrun:%lu\r\n",
                    (unsigned long)Stats.SampleCount,
                    (unsigned long)Stats.QueueDropCount,
                    (unsigned long)Stats.OverrunCount
                );

                printf(
                    "Mutex:%lu Actual:%lu/%lu\r\n",
                    (unsigned long)Stats.MaxMutexWait,
                    (unsigned long)Stats.MinActualInterval,
                    (unsigned long)Stats.MaxActualInterval
                );
            }
#endif
        }
        else
        {
            /* 非法调用等错误，避免持续空转 */
            MiniRTOS_delay(1U);
        }
    }
}

/*---------------------------------------------
 * OLED数据显示任务
 *--------------------------------------------*/
void OLED_Handle(void *arg)
{
    SensorPacket_t Packet;
    uint8_t Valid;

    char Buffer[24];

    uint32_t LastWakeTime = MiniRTOS_GetTick();

    while (1)
    {
        /* 读取最新传感器数据 */
        Mini_EnterCritical();

        Packet = LatestPacket;
        Valid = LatestPacketValid;

        Mini_ExitCritical();

        OLED_Clear();

        OLED_DrawString(0, 0, "MiniRTOS Monitor");

        if (Valid != 0U)
        {
            snprintf(Buffer, sizeof(Buffer),
                     "Seq:%lu",
                     (unsigned long)Packet.Sequence);
            OLED_DrawString(0, 16, Buffer);

            snprintf(Buffer, sizeof(Buffer),
                     "AX:%d", Packet.Sensor.AccelX);
            OLED_DrawString(0, 28, Buffer);

            snprintf(Buffer, sizeof(Buffer),
                     "AY:%d", Packet.Sensor.AccelY);
            OLED_DrawString(0, 40, Buffer);

            snprintf(Buffer, sizeof(Buffer),
                     "AZ:%d", Packet.Sensor.AccelZ);
            OLED_DrawString(0, 52, Buffer);
        }
        else
        {
            OLED_DrawString(0, 24, "No Sensor Data");
        }

        /* 保护共享I2C总线 */
        MutexSignal_Take(&I2C_Mutex);

        OLED_Update();

        MutexSignal_Release(&I2C_Mutex);

        MiniRTOS_DelayUntil(
            &LastWakeTime,
            OLED_PERIOD_TICK
        );
    }
}
