#ifndef __BSP_XPT2046_LCD_H
#define __BSP_XPT2046_LCD_H

#include "bsp_system.h"

/******************************* PENIRQ 引脚配置 ***************************/
#define XPT2046_PENIRQ_GPIO_PORT             GPIOF
#define XPT2046_PENIRQ_GPIO_PIN              GPIO_PIN_9

#define XPT2046_PENIRQ_ActiveLevel           0
#define XPT2046_PENIRQ_Read()                HAL_GPIO_ReadPin(XPT2046_PENIRQ_GPIO_PORT, XPT2046_PENIRQ_GPIO_PIN)

/******************************* 模拟 SPI GPIO 定义（HAL方式） ***************************/
#define XPT2046_SPI_CS_PORT                  GPIOF
#define XPT2046_SPI_CS_PIN                   GPIO_PIN_10

#define XPT2046_SPI_CLK_PORT                 GPIOG
#define XPT2046_SPI_CLK_PIN                  GPIO_PIN_7

#define XPT2046_SPI_MOSI_PORT                GPIOF
#define XPT2046_SPI_MOSI_PIN                 GPIO_PIN_11

#define XPT2046_SPI_MISO_PORT                GPIOF
#define XPT2046_SPI_MISO_PIN                 GPIO_PIN_6

// 模拟 SPI 控制宏（HAL）
#define XPT2046_CS_ENABLE()                  HAL_GPIO_WritePin(XPT2046_SPI_CS_PORT, XPT2046_SPI_CS_PIN, GPIO_PIN_RESET)
#define XPT2046_CS_DISABLE()                 HAL_GPIO_WritePin(XPT2046_SPI_CS_PORT, XPT2046_SPI_CS_PIN, GPIO_PIN_SET)

#define XPT2046_CLK_HIGH()                   HAL_GPIO_WritePin(XPT2046_SPI_CLK_PORT, XPT2046_SPI_CLK_PIN, GPIO_PIN_SET)
#define XPT2046_CLK_LOW()                    HAL_GPIO_WritePin(XPT2046_SPI_CLK_PORT, XPT2046_SPI_CLK_PIN, GPIO_PIN_RESET)

#define XPT2046_MOSI_1()                     HAL_GPIO_WritePin(XPT2046_SPI_MOSI_PORT, XPT2046_SPI_MOSI_PIN, GPIO_PIN_SET)
#define XPT2046_MOSI_0()                     HAL_GPIO_WritePin(XPT2046_SPI_MOSI_PORT, XPT2046_SPI_MOSI_PIN, GPIO_PIN_RESET)

#define XPT2046_MISO()                       HAL_GPIO_ReadPin(XPT2046_SPI_MISO_PORT, XPT2046_SPI_MISO_PIN)

/******************************* 校准等参数定义 ***************************/
#define XPT2046_CHANNEL_X                    0x90
#define XPT2046_CHANNEL_Y                    0xD0
#define XPT2046_THRESHOLD_CalDiff            2

/******************************* 数据结构定义 ***************************/
typedef struct {
    int16_t x;
    int16_t y;
} XPT2046_Coordinate;

typedef struct {
    float An, Bn, Cn, Dn, En, Fn, Divider;
} XPT2046_Calibration;

typedef struct {
    float dX_X, dX_Y, dX, dY_X, dY_Y, dY;
} XPT2046_TouchPara;

/******************************* 函数声明 ***************************/
void XPT2046_Init(void);
uint8_t XPT2046_ReadPoint(XPT2046_Coordinate* point);
uint8_t XPT2046_IsTouched(void);
uint8_t XPT2046_Touch_Calibrate(uint8_t LCD_Mode);
uint8_t XPT2046_Get_TouchedPoint(XPT2046_Coordinate* displayPtr, XPT2046_TouchPara* para);
uint8_t XPT2046_TouchDetect_WithCircle(void);



extern XPT2046_TouchPara g_TouchPara;
#endif /* __BSP_XPT2046_LCD_H */
