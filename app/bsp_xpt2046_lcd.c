/**
 ******************************************************************************
 * @file    bsp_xpt2046_lcd.c
 * @version V1.0
 * @date    2023-xx-xx
 * @brief   XPT2046触摸屏驱动（简化版）
 ******************************************************************************
 */

#include "bsp_xpt2046_lcd.h"

/******************************* 静态函数声明 ***************************/
static void XPT2046_DelayUS(uint32_t us);
static void XPT2046_WriteCMD(uint8_t cmd);
static uint16_t XPT2046_ReadCMD(void);
static uint16_t XPT2046_ReadAdc(uint8_t channel);
static void XPT2046_ReadAdc_XY(int16_t *x, int16_t *y);


// 添加静态函数声明
static uint8_t XPT2046_ReadAdc_Smooth_XY(XPT2046_Coordinate *pScreenCoordinate);
static uint8_t XPT2046_Calculate_CalibrationFactor(XPT2046_Coordinate *pDisplayCoordinate, XPT2046_Coordinate *pScreenSample, XPT2046_Calibration *pCalibrationFactor);
static void ILI9341_DrawCross(uint16_t usX, uint16_t usY);

// 添加全局变量
XPT2046_TouchPara g_TouchPara;
/**
 * @brief  XPT2046 初始化函数
 * @param  无
 * @retval 无
 */
void XPT2046_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// 1. 使能 GPIO 时钟
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

	// 2. 初始化 SPI CLK 引脚（模拟 SPI）
	GPIO_InitStruct.Pin = XPT2046_SPI_CLK_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(XPT2046_SPI_CLK_PORT, &GPIO_InitStruct);

	// 3. 初始化 MOSI 引脚
	GPIO_InitStruct.Pin = XPT2046_SPI_MOSI_PIN;
	HAL_GPIO_Init(XPT2046_SPI_MOSI_PORT, &GPIO_InitStruct);

	// 4. 初始化 MISO 引脚为输入上拉
	GPIO_InitStruct.Pin = XPT2046_SPI_MISO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(XPT2046_SPI_MISO_PORT, &GPIO_InitStruct);

	// 5. 初始化 CS 引脚
	GPIO_InitStruct.Pin = XPT2046_SPI_CS_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(XPT2046_SPI_CS_PORT, &GPIO_InitStruct);

	// 6. 禁用片选（默认不选中触摸芯片）
	XPT2046_CS_DISABLE();

	// 7. 初始化 PENIRQ 引脚为输入上拉（触摸检测中断/轮询）
	GPIO_InitStruct.Pin = XPT2046_PENIRQ_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(XPT2046_PENIRQ_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief  微秒级延时函数
 * @param  us: 延时时间，单位微秒
 * @retval 无
 */
static void XPT2046_DelayUS(uint32_t us)
{
	uint32_t i;
	for (i = 0; i < us; i++)
	{
		uint8_t uc = 12; // 大约1微秒的延时
		while (uc--)
			;
	}
}

/**
 * @brief  XPT2046 写命令
 * @param  cmd: 命令字节
 * @retval 无
 */
static void XPT2046_WriteCMD(uint8_t cmd)
{
	uint8_t i;

	XPT2046_MOSI_0();
	XPT2046_CLK_LOW();

	for (i = 0; i < 8; i++)
	{
		((cmd >> (7 - i)) & 0x01) ? XPT2046_MOSI_1() : XPT2046_MOSI_0();
		XPT2046_DelayUS(5);
		XPT2046_CLK_HIGH();
		XPT2046_DelayUS(5);
		XPT2046_CLK_LOW();
	}
}

/**
 * @brief  XPT2046 读取数据
 * @param  无
 * @retval 读取到的数据
 */
static uint16_t XPT2046_ReadCMD(void)
{
	uint8_t i;
	uint16_t data = 0;

	XPT2046_MOSI_0();
	XPT2046_CLK_HIGH();

	for (i = 0; i < 12; i++)
	{
		XPT2046_CLK_LOW();
		XPT2046_DelayUS(5);
		data |= (XPT2046_MISO() << (11 - i));
		XPT2046_CLK_HIGH();
		XPT2046_DelayUS(5);
	}

	return data;
}

/**
 * @brief  读取XPT2046指定通道的ADC值
 * @param  channel: 通道命令
 *         - XPT2046_CHANNEL_X: X通道
 *         - XPT2046_CHANNEL_Y: Y通道
 * @retval ADC值
 */
static uint16_t XPT2046_ReadAdc(uint8_t channel)
{
	XPT2046_CS_ENABLE();
	XPT2046_WriteCMD(channel);
	XPT2046_DelayUS(10);
	uint16_t adc = XPT2046_ReadCMD();
	XPT2046_CS_DISABLE();
	return adc;
}

/**
 * @brief  读取XPT2046的X和Y坐标ADC值
 * @param  x: X坐标ADC值指针
 * @param  y: Y坐标ADC值指针
 * @retval 无
 */
static void XPT2046_ReadAdc_XY(int16_t *x, int16_t *y)
{
	*x = XPT2046_ReadAdc(XPT2046_CHANNEL_X);
	XPT2046_DelayUS(1);
	*y = XPT2046_ReadAdc(XPT2046_CHANNEL_Y);
}

/**
 * @brief  检测是否有触摸
 * @param  无
 * @retval 1: 有触摸，0: 无触摸
 */
uint8_t XPT2046_IsTouched(void)
{
	return (XPT2046_PENIRQ_Read() == XPT2046_PENIRQ_ActiveLevel);
}

/**
 * @brief  读取触摸点坐标
 * @param  point: 坐标结构体指针
 * @retval 1: 读取成功，0: 读取失败
 */
uint8_t XPT2046_ReadPoint(XPT2046_Coordinate *point)
{
	int16_t x, y;

	if (!XPT2046_IsTouched())
		return 0;

	// 读取ADC值
	XPT2046_ReadAdc_XY(&x, &y);

	// 简单滤波，多次读取取平均值
	for (uint8_t i = 0; i < 3; i++)
	{
		int16_t x1, y1;
		XPT2046_ReadAdc_XY(&x1, &y1);
		x = (x + x1) / 2;
		y = (y + y1) / 2;
	}

	// 存储坐标
	point->x = x;
	point->y = y;

	return 1;
}



/**
 * @brief  读取触摸点坐标（平滑滤波）
 * @param  pScreenCoordinate: 坐标结构体指针
 * @retval 1: 读取成功，0: 读取失败
 */
static uint8_t XPT2046_ReadAdc_Smooth_XY(XPT2046_Coordinate *pScreenCoordinate)
{
    uint8_t ucCount = 0, i;
    
    int16_t sAD_X, sAD_Y;
    int16_t sBufferArray[2][10] = {{0}, {0}};  // 存储X和Y坐标的多次采样
    
    // 存储采样中的最小值和最大值
    int32_t lX_Min, lX_Max, lY_Min, lY_Max;

    // 循环采样10次
    do                           
    {     
        XPT2046_ReadAdc_XY(&sAD_X, &sAD_Y);  
        
        sBufferArray[0][ucCount] = sAD_X;  
        sBufferArray[1][ucCount] = sAD_Y;
        
        ucCount++;  
        
    } while ((XPT2046_PENIRQ_Read() == XPT2046_PENIRQ_ActiveLevel) && (ucCount < 10));

    // 如果触摸中断
    if (XPT2046_PENIRQ_Read() != XPT2046_PENIRQ_ActiveLevel)
        return 0;

    // 如果成功采样10个点
    if (ucCount == 10)         
    {
        lX_Max = lX_Min = sBufferArray[0][0];
        lY_Max = lY_Min = sBufferArray[1][0];       
        
        for (i = 1; i < 10; i++)
        {
            if (sBufferArray[0][i] < lX_Min)
                lX_Min = sBufferArray[0][i];
            
            else if (sBufferArray[0][i] > lX_Max)
                lX_Max = sBufferArray[0][i];
        }
        
        for (i = 1; i < 10; i++)
        {
            if (sBufferArray[1][i] < lY_Min)
                lY_Min = sBufferArray[1][i];
            
            else if (sBufferArray[1][i] > lY_Max)
                lY_Max = sBufferArray[1][i];
        }
        
        // 去除最大值和最小值后求平均值
        pScreenCoordinate->x = (sBufferArray[0][0] + sBufferArray[0][1] + sBufferArray[0][2] + sBufferArray[0][3] + sBufferArray[0][4] + 
                               sBufferArray[0][5] + sBufferArray[0][6] + sBufferArray[0][7] + sBufferArray[0][8] + sBufferArray[0][9] - lX_Min - lX_Max) >> 3;
        
        pScreenCoordinate->y = (sBufferArray[1][0] + sBufferArray[1][1] + sBufferArray[1][2] + sBufferArray[1][3] + sBufferArray[1][4] + 
                               sBufferArray[1][5] + sBufferArray[1][6] + sBufferArray[1][7] + sBufferArray[1][8] + sBufferArray[1][9] - lY_Min - lY_Max) >> 3; 
        
        return 1;        
    }   
    
    return 0;    
}

/**
 * @brief  计算触摸屏校准系数
 * @param  pDisplayCoordinate: 显示坐标
 * @param  pScreenSample: 触摸采样坐标
 * @param  pCalibrationFactor: 校准系数
 * @retval 1: 成功，0: 失败
 */
static uint8_t XPT2046_Calculate_CalibrationFactor(XPT2046_Coordinate *pDisplayCoordinate, XPT2046_Coordinate *pScreenSample, XPT2046_Calibration *pCalibrationFactor)
{
    uint8_t ucRet = 1;

    /* K＝(X0－X2) (Y1－Y2)－(X1－X2) (Y0－Y2) */
    pCalibrationFactor->Divider = ((pScreenSample[0].x - pScreenSample[2].x) * (pScreenSample[1].y - pScreenSample[2].y)) - 
                                 ((pScreenSample[1].x - pScreenSample[2].x) * (pScreenSample[0].y - pScreenSample[2].y));
    
    if (pCalibrationFactor->Divider == 0)
        ucRet = 0;
    else
    {
        /* A＝((XD0－XD2) (Y1－Y2)－(XD1－XD2) (Y0－Y2))／K */
        pCalibrationFactor->An = ((pDisplayCoordinate[0].x - pDisplayCoordinate[2].x) * (pScreenSample[1].y - pScreenSample[2].y)) - 
                                ((pDisplayCoordinate[1].x - pDisplayCoordinate[2].x) * (pScreenSample[0].y - pScreenSample[2].y));
        
        /* B＝((X0－X2) (XD1－XD2)－(XD0－XD2) (X1－X2))／K */
        pCalibrationFactor->Bn = ((pScreenSample[0].x - pScreenSample[2].x) * (pDisplayCoordinate[1].x - pDisplayCoordinate[2].x)) - 
                                ((pDisplayCoordinate[0].x - pDisplayCoordinate[2].x) * (pScreenSample[1].x - pScreenSample[2].x));
        
        /* C＝(Y0(X2XD1－X1XD2)+Y1(X0XD2－X2XD0)+Y2(X1XD0－X0XD1))／K */
        pCalibrationFactor->Cn = (pScreenSample[2].x * pDisplayCoordinate[1].x - pScreenSample[1].x * pDisplayCoordinate[2].x) * pScreenSample[0].y +
                                (pScreenSample[0].x * pDisplayCoordinate[2].x - pScreenSample[2].x * pDisplayCoordinate[0].x) * pScreenSample[1].y +
                                (pScreenSample[1].x * pDisplayCoordinate[0].x - pScreenSample[0].x * pDisplayCoordinate[1].x) * pScreenSample[2].y;
        
        /* D＝((YD0－YD2) (Y1－Y2)－(YD1－YD2) (Y0－Y2))／K */
        pCalibrationFactor->Dn = ((pDisplayCoordinate[0].y - pDisplayCoordinate[2].y) * (pScreenSample[1].y - pScreenSample[2].y)) - 
                                ((pDisplayCoordinate[1].y - pDisplayCoordinate[2].y) * (pScreenSample[0].y - pScreenSample[2].y));
        
        /* E＝((X0－X2) (YD1－YD2)－(YD0－YD2) (X1－X2))／K */
        pCalibrationFactor->En = ((pScreenSample[0].x - pScreenSample[2].x) * (pDisplayCoordinate[1].y - pDisplayCoordinate[2].y)) - 
                                ((pDisplayCoordinate[0].y - pDisplayCoordinate[2].y) * (pScreenSample[1].x - pScreenSample[2].x));
        
        /* F＝(Y0(X2YD1－X1YD2)+Y1(X0YD2－X2YD0)+Y2(X1YD0－X0YD1))／K */
        pCalibrationFactor->Fn = (pScreenSample[2].x * pDisplayCoordinate[1].y - pScreenSample[1].x * pDisplayCoordinate[2].y) * pScreenSample[0].y +
                                (pScreenSample[0].x * pDisplayCoordinate[2].y - pScreenSample[2].x * pDisplayCoordinate[0].y) * pScreenSample[1].y +
                                (pScreenSample[1].x * pDisplayCoordinate[0].y - pScreenSample[0].x * pDisplayCoordinate[1].y) * pScreenSample[2].y;
    }
    
    return ucRet;
}

/**
 * @brief  在LCD上绘制十字光标
 * @param  usX: X坐标
 * @param  usY: Y坐标
 * @retval 无
 */
static void ILI9341_DrawCross(uint16_t usX, uint16_t usY)
{
    ILI9341_DrawLine(usX - 10, usY, usX + 10, usY);
    ILI9341_DrawLine(usX, usY - 10, usX, usY + 10);    
}

/**
 * @brief  在LCD上绘制圆圈
 * @param  x: 圆心X坐标
 * @param  y: 圆心Y坐标
 * @param  radius: 半径
 * @param  draw: 1表示绘制，0表示擦除(用背景色绘制)
 * @retval 无
 */
static void ILI9341_DrawTouchCircle(uint16_t x, uint16_t y, uint8_t radius, uint8_t draw)
{
    uint16_t textColor, backColor;
    
    // 保存当前颜色设置
    LCD_GetColors(&textColor, &backColor);
    
    if (draw)
    {
        // 绘制圆圈时使用白色
        LCD_SetTextColor(WHITE);
    }
    else
    {
        // 擦除圆圈时使用背景色
        LCD_SetTextColor(backColor);
    }
    
    // 绘制圆圈，使用空心圆(ucFilled=0)
    ILI9341_DrawCircle(x, y, radius, 0);
    
    // 恢复原来的颜色
    LCD_SetColors(textColor, backColor);
}
/**
 * @brief  XPT2046 触摸屏校准
 * @param  LCD_Mode: 指定要校准的液晶扫描模式
 * @retval 校准结果
 *   返回值为以下值之一：
 *     @arg 1: 校准成功
 *     @arg 0: 校准失败
 */
uint8_t XPT2046_Touch_Calibrate(uint8_t LCD_Mode)
{
    uint8_t i;
    
    char cStr[100];
    
    uint16_t usTest_x = 0, usTest_y = 0, usGap_x = 0, usGap_y = 0;
    
    char* pStr = 0;
  
    XPT2046_Coordinate strCrossCoordinate[4], strScreenSample[4];
    
    XPT2046_Calibration CalibrationFactor;
        
    LCD_SetFont(&Font8x16);
    LCD_SetColors(BLUE, BLACK);
  
    // 设置扫描方向，横屏
    ILI9341_GramScan(LCD_Mode);
    
    // 设定十字光标的坐标
    strCrossCoordinate[0].x = LCD_X_LENGTH >> 2;
    strCrossCoordinate[0].y = LCD_Y_LENGTH >> 2;
    
    strCrossCoordinate[1].x = strCrossCoordinate[0].x;
    strCrossCoordinate[1].y = (LCD_Y_LENGTH * 3) >> 2;
    
    strCrossCoordinate[2].x = (LCD_X_LENGTH * 3) >> 2;
    strCrossCoordinate[2].y = strCrossCoordinate[1].y;
    
    strCrossCoordinate[3].x = strCrossCoordinate[2].x;
    strCrossCoordinate[3].y = strCrossCoordinate[0].y;    
    
    for (i = 0; i < 4; i++)
    { 
        ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);       
        
        pStr = "Touch Calibrate ......";        
        // 居中显示
        sprintf(cStr, "%*c%s", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - strlen(pStr)) / 2, ' ', pStr);    
        ILI9341_DispStringLine_EN(LCD_Y_LENGTH >> 1, cStr);            
    
        // 居中显示
        sprintf(cStr, "%*c%d", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - 1) / 2, ' ', i + 1);
        ILI9341_DispStringLine_EN((LCD_Y_LENGTH >> 1) - (((sFONT*)LCD_GetFont())->Height), cStr); 
    
        XPT2046_DelayUS(300000);                     // 适当的延时很重要
        
        ILI9341_DrawCross(strCrossCoordinate[i].x, strCrossCoordinate[i].y);  // 显示校正用的十字光标

        while (!XPT2046_ReadAdc_Smooth_XY(&strScreenSample[i]));               // 读取XPT2046数据到变量
    }
    
    XPT2046_Calculate_CalibrationFactor(strCrossCoordinate, strScreenSample, &CalibrationFactor);     // 用原始参数计算出转换系数
    
    if (CalibrationFactor.Divider == 0) goto Failure;
    
    // 校验触摸屏的准确性，取一个点计算它的坐标
    usTest_x = ((CalibrationFactor.An * strScreenSample[3].x) + (CalibrationFactor.Bn * strScreenSample[3].y) + CalibrationFactor.Cn) / CalibrationFactor.Divider;        // 取一个点计算X值    
    usTest_y = ((CalibrationFactor.Dn * strScreenSample[3].x) + (CalibrationFactor.En * strScreenSample[3].y) + CalibrationFactor.Fn) / CalibrationFactor.Divider;    // 取一个点计算Y值
    
    usGap_x = (usTest_x > strCrossCoordinate[3].x) ? (usTest_x - strCrossCoordinate[3].x) : (strCrossCoordinate[3].x - usTest_x);   // 实际X坐标与计算坐标的绝对差
    usGap_y = (usTest_y > strCrossCoordinate[3].y) ? (usTest_y - strCrossCoordinate[3].y) : (strCrossCoordinate[3].y - usTest_y);   // 实际Y坐标与计算坐标的绝对差
    
    if ((usGap_x > 15) || (usGap_y > 15)) goto Failure;       // 可以通过修改这两个值的大小来调整精度    

    // 校准系数为全局变量
    g_TouchPara.dX_X = (CalibrationFactor.An * 1.0) / CalibrationFactor.Divider;
    g_TouchPara.dX_Y = (CalibrationFactor.Bn * 1.0) / CalibrationFactor.Divider;
    g_TouchPara.dX = (CalibrationFactor.Cn * 1.0) / CalibrationFactor.Divider;
    
    g_TouchPara.dY_X = (CalibrationFactor.Dn * 1.0) / CalibrationFactor.Divider;
    g_TouchPara.dY_Y = (CalibrationFactor.En * 1.0) / CalibrationFactor.Divider;
    g_TouchPara.dY = (CalibrationFactor.Fn * 1.0) / CalibrationFactor.Divider;
  
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);
  
    LCD_SetTextColor(GREEN);
  
    pStr = "Calibrate Succeed";
    // 居中显示    
    sprintf(cStr, "%*c%s", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - strlen(pStr)) / 2, ' ', pStr);    
    ILI9341_DispStringLine_EN(LCD_Y_LENGTH >> 1, cStr);    

    XPT2046_DelayUS(1000000);

    return 1;    
  
Failure:
  
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH); 
  
    LCD_SetTextColor(RED);
  
    pStr = "Calibrate fail";    
    // 居中显示    
    sprintf(cStr, "%*c%s", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - strlen(pStr)) / 2, ' ', pStr);    
    ILI9341_DispStringLine_EN(LCD_Y_LENGTH >> 1, cStr);    

    pStr = "try again";
    // 居中显示        
    sprintf(cStr, "%*c%s", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - strlen(pStr)) / 2, ' ', pStr);    
    ILI9341_DispStringLine_EN((LCD_Y_LENGTH >> 1) + (((sFONT*)LCD_GetFont())->Height), cStr);
	// lcdSprintf((LCD_Y_LENGTH >> 1) + (((sFONT *)LCD_GetFont())->Height),
	// 		   ((LCD_X_LENGTH / (((sFONT *)LCD_GetFont())->Width)) - strlen(pStr)) / 2,
	// 		   "%s", pStr);
		XPT2046_DelayUS(1000000);

	return 0;    
}

/**
 * @brief  获取触摸点的坐标
 * @param  displayPtr: 转换后的显示坐标
 * @param  para: 校准参数
 * @retval 1: 成功，0: 失败
 */
uint8_t XPT2046_Get_TouchedPoint(XPT2046_Coordinate *displayPtr, XPT2046_TouchPara *para)
{
    XPT2046_Coordinate screenPtr;
    
    if (!XPT2046_ReadAdc_Smooth_XY(&screenPtr))
        return 0;
    
    // 将触摸屏坐标转换为显示坐标
    displayPtr->x = para->dX_X * screenPtr.x + para->dX_Y * screenPtr.y + para->dX;
    displayPtr->y = para->dY_X * screenPtr.x + para->dY_Y * screenPtr.y + para->dY;
    
    return 1;
}

/**
 * @brief  触摸状态监测（带圆圈显示）
 * @param  无
 * @retval 触摸状态
 *   返回值：0-无触摸，1-有触摸
 */
uint8_t XPT2046_TouchDetect_WithCircle(void)
{
    static uint8_t lastTouchState = 0;
    static XPT2046_Coordinate lastPoint = {0, 0};
    uint8_t currentTouchState;
    XPT2046_Coordinate currentPoint;
    
    // 检测当前触摸状态
    currentTouchState = XPT2046_IsTouched();
    
    // 如果有触摸
    if (currentTouchState)
    {
        // 获取校准后的触摸坐标
        if (XPT2046_Get_TouchedPoint(&currentPoint, &g_TouchPara))
        {
            // 确保坐标在屏幕范围内
            if (currentPoint.x < 0) currentPoint.x = 0;
            if (currentPoint.y < 0) currentPoint.y = 0;
            if (currentPoint.x >= LCD_X_LENGTH) currentPoint.x = LCD_X_LENGTH - 1;
            if (currentPoint.y >= LCD_Y_LENGTH) currentPoint.y = LCD_Y_LENGTH - 1;
            
            // 如果是刚开始触摸或者触摸位置变化较大
            if (!lastTouchState || 
                (abs(currentPoint.x - lastPoint.x) > 5) || 
                (abs(currentPoint.y - lastPoint.y) > 5))
            {
                // 如果之前有触摸，先擦除旧圆圈
                if (lastTouchState)
                {
                    ILI9341_DrawTouchCircle(lastPoint.x, lastPoint.y, 15, 0);
                }
                
                // 绘制新圆圈，确保圆圈在触摸点正中心
                ILI9341_DrawTouchCircle(currentPoint.x, currentPoint.y, 15, 1);
                
                // 更新上次触摸点
                lastPoint.x = currentPoint.x;
                lastPoint.y = currentPoint.y;
            }
        }
    }
    // 如果无触摸但上次有触摸
    else if (lastTouchState)
    {
        // 擦除上次的圆圈
        ILI9341_DrawTouchCircle(lastPoint.x, lastPoint.y, 15, 0);
    }
    
    // 更新上次触摸状态
    lastTouchState = currentTouchState;
    
    return currentTouchState;
}
