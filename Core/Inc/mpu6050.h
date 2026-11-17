#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* MPU6050的7位地址为0x68，HAL使用左移一位的地址 */
#define MPU6050_ADDR (0x68U << 1)

/* 原始传感器数据 */
typedef struct
{
    int16_t AccelX;
    int16_t AccelY;
    int16_t AccelZ;

    int16_t Temperature;

    int16_t GyroX;
    int16_t GyroY;
    int16_t GyroZ;

} MPU6050_Data_t;

/* 初始化传感器 */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);

/* 读取连续14字节传感器数据 */
HAL_StatusTypeDef MPU6050_ReadData(MPU6050_Data_t *Data);

#endif
