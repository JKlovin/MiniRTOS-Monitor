#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <stdint.h>
#include "mpu6050.h"

typedef struct
{
    uint32_t Tick;           /* 采样时间戳 */
    uint32_t Sequence;       /* 采样序号 */

    MPU6050_Data_t Sensor;   /* 六轴数据和温度 */

} SensorPacket_t;

#endif


