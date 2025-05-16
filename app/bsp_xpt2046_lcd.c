/**
 ******************************************************************************
 * @file    bsp_xpt2046_lcd.c
 * @version V1.0
 * @date    2023-xx-xx
 * @brief   XPT2046�������������򻯰棩
 ******************************************************************************
 */

#include "bsp_xpt2046_lcd.h"

/******************************* ��̬�������� ***************************/
static void XPT2046_DelayUS(uint32_t us);
static void XPT2046_WriteCMD(uint8_t cmd);
static uint16_t XPT2046_ReadCMD(void);
static uint16_t XPT2046_ReadAdc(uint8_t channel);
static void XPT2046_ReadAdc_XY(int16_t *x, int16_t *y);


// ��Ӿ�̬��������
static uint8_t XPT2046_ReadAdc_Smooth_XY(XPT2046_Coordinate *pScreenCoordinate);
static uint8_t XPT2046_Calculate_CalibrationFactor(XPT2046_Coordinate *pDisplayCoordinate, XPT2046_Coordinate *pScreenSample, XPT2046_Calibration *pCalibrationFactor);
static void ILI9341_DrawCross(uint16_t usX, uint16_t usY);

// ���ȫ�ֱ���
XPT2046_TouchPara g_TouchPara;
/**
 * @brief  XPT2046 ��ʼ������
 * @param  ��
 * @retval ��
 */
void XPT2046_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// 1. ʹ�� GPIO ʱ��
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

	// 2. ��ʼ�� SPI CLK ���ţ�ģ�� SPI��
	GPIO_InitStruct.Pin = XPT2046_SPI_CLK_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(XPT2046_SPI_CLK_PORT, &GPIO_InitStruct);

	// 3. ��ʼ�� MOSI ����
	GPIO_InitStruct.Pin = XPT2046_SPI_MOSI_PIN;
	HAL_GPIO_Init(XPT2046_SPI_MOSI_PORT, &GPIO_InitStruct);

	// 4. ��ʼ�� MISO ����Ϊ��������
	GPIO_InitStruct.Pin = XPT2046_SPI_MISO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(XPT2046_SPI_MISO_PORT, &GPIO_InitStruct);

	// 5. ��ʼ�� CS ����
	GPIO_InitStruct.Pin = XPT2046_SPI_CS_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(XPT2046_SPI_CS_PORT, &GPIO_InitStruct);

	// 6. ����Ƭѡ��Ĭ�ϲ�ѡ�д���оƬ��
	XPT2046_CS_DISABLE();

	// 7. ��ʼ�� PENIRQ ����Ϊ������������������ж�/��ѯ��
	GPIO_InitStruct.Pin = XPT2046_PENIRQ_GPIO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(XPT2046_PENIRQ_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * @brief  ΢�뼶��ʱ����
 * @param  us: ��ʱʱ�䣬��λ΢��
 * @retval ��
 */
static void XPT2046_DelayUS(uint32_t us)
{
	uint32_t i;
	for (i = 0; i < us; i++)
	{
		uint8_t uc = 12; // ��Լ1΢�����ʱ
		while (uc--)
			;
	}
}

/**
 * @brief  XPT2046 д����
 * @param  cmd: �����ֽ�
 * @retval ��
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
 * @brief  XPT2046 ��ȡ����
 * @param  ��
 * @retval ��ȡ��������
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
 * @brief  ��ȡXPT2046ָ��ͨ����ADCֵ
 * @param  channel: ͨ������
 *         - XPT2046_CHANNEL_X: Xͨ��
 *         - XPT2046_CHANNEL_Y: Yͨ��
 * @retval ADCֵ
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
 * @brief  ��ȡXPT2046��X��Y����ADCֵ
 * @param  x: X����ADCֵָ��
 * @param  y: Y����ADCֵָ��
 * @retval ��
 */
static void XPT2046_ReadAdc_XY(int16_t *x, int16_t *y)
{
	*x = XPT2046_ReadAdc(XPT2046_CHANNEL_X);
	XPT2046_DelayUS(1);
	*y = XPT2046_ReadAdc(XPT2046_CHANNEL_Y);
}

/**
 * @brief  ����Ƿ��д���
 * @param  ��
 * @retval 1: �д�����0: �޴���
 */
uint8_t XPT2046_IsTouched(void)
{
	return (XPT2046_PENIRQ_Read() == XPT2046_PENIRQ_ActiveLevel);
}

/**
 * @brief  ��ȡ����������
 * @param  point: ����ṹ��ָ��
 * @retval 1: ��ȡ�ɹ���0: ��ȡʧ��
 */
uint8_t XPT2046_ReadPoint(XPT2046_Coordinate *point)
{
	int16_t x, y;

	if (!XPT2046_IsTouched())
		return 0;

	// ��ȡADCֵ
	XPT2046_ReadAdc_XY(&x, &y);

	// ���˲�����ζ�ȡȡƽ��ֵ
	for (uint8_t i = 0; i < 3; i++)
	{
		int16_t x1, y1;
		XPT2046_ReadAdc_XY(&x1, &y1);
		x = (x + x1) / 2;
		y = (y + y1) / 2;
	}

	// �洢����
	point->x = x;
	point->y = y;

	return 1;
}



/**
 * @brief  ��ȡ���������꣨ƽ���˲���
 * @param  pScreenCoordinate: ����ṹ��ָ��
 * @retval 1: ��ȡ�ɹ���0: ��ȡʧ��
 */
static uint8_t XPT2046_ReadAdc_Smooth_XY(XPT2046_Coordinate *pScreenCoordinate)
{
    uint8_t ucCount = 0, i;
    
    int16_t sAD_X, sAD_Y;
    int16_t sBufferArray[2][10] = {{0}, {0}};  // �洢X��Y����Ķ�β���
    
    // �洢�����е���Сֵ�����ֵ
    int32_t lX_Min, lX_Max, lY_Min, lY_Max;

    // ѭ������10��
    do                           
    {     
        XPT2046_ReadAdc_XY(&sAD_X, &sAD_Y);  
        
        sBufferArray[0][ucCount] = sAD_X;  
        sBufferArray[1][ucCount] = sAD_Y;
        
        ucCount++;  
        
    } while ((XPT2046_PENIRQ_Read() == XPT2046_PENIRQ_ActiveLevel) && (ucCount < 10));

    // ��������ж�
    if (XPT2046_PENIRQ_Read() != XPT2046_PENIRQ_ActiveLevel)
        return 0;

    // ����ɹ�����10����
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
        
        // ȥ�����ֵ����Сֵ����ƽ��ֵ
        pScreenCoordinate->x = (sBufferArray[0][0] + sBufferArray[0][1] + sBufferArray[0][2] + sBufferArray[0][3] + sBufferArray[0][4] + 
                               sBufferArray[0][5] + sBufferArray[0][6] + sBufferArray[0][7] + sBufferArray[0][8] + sBufferArray[0][9] - lX_Min - lX_Max) >> 3;
        
        pScreenCoordinate->y = (sBufferArray[1][0] + sBufferArray[1][1] + sBufferArray[1][2] + sBufferArray[1][3] + sBufferArray[1][4] + 
                               sBufferArray[1][5] + sBufferArray[1][6] + sBufferArray[1][7] + sBufferArray[1][8] + sBufferArray[1][9] - lY_Min - lY_Max) >> 3; 
        
        return 1;        
    }   
    
    return 0;    
}

/**
 * @brief  ���㴥����У׼ϵ��
 * @param  pDisplayCoordinate: ��ʾ����
 * @param  pScreenSample: ������������
 * @param  pCalibrationFactor: У׼ϵ��
 * @retval 1: �ɹ���0: ʧ��
 */
static uint8_t XPT2046_Calculate_CalibrationFactor(XPT2046_Coordinate *pDisplayCoordinate, XPT2046_Coordinate *pScreenSample, XPT2046_Calibration *pCalibrationFactor)
{
    uint8_t ucRet = 1;

    /* K��(X0��X2) (Y1��Y2)��(X1��X2) (Y0��Y2) */
    pCalibrationFactor->Divider = ((pScreenSample[0].x - pScreenSample[2].x) * (pScreenSample[1].y - pScreenSample[2].y)) - 
                                 ((pScreenSample[1].x - pScreenSample[2].x) * (pScreenSample[0].y - pScreenSample[2].y));
    
    if (pCalibrationFactor->Divider == 0)
        ucRet = 0;
    else
    {
        /* A��((XD0��XD2) (Y1��Y2)��(XD1��XD2) (Y0��Y2))��K */
        pCalibrationFactor->An = ((pDisplayCoordinate[0].x - pDisplayCoordinate[2].x) * (pScreenSample[1].y - pScreenSample[2].y)) - 
                                ((pDisplayCoordinate[1].x - pDisplayCoordinate[2].x) * (pScreenSample[0].y - pScreenSample[2].y));
        
        /* B��((X0��X2) (XD1��XD2)��(XD0��XD2) (X1��X2))��K */
        pCalibrationFactor->Bn = ((pScreenSample[0].x - pScreenSample[2].x) * (pDisplayCoordinate[1].x - pDisplayCoordinate[2].x)) - 
                                ((pDisplayCoordinate[0].x - pDisplayCoordinate[2].x) * (pScreenSample[1].x - pScreenSample[2].x));
        
        /* C��(Y0(X2XD1��X1XD2)+Y1(X0XD2��X2XD0)+Y2(X1XD0��X0XD1))��K */
        pCalibrationFactor->Cn = (pScreenSample[2].x * pDisplayCoordinate[1].x - pScreenSample[1].x * pDisplayCoordinate[2].x) * pScreenSample[0].y +
                                (pScreenSample[0].x * pDisplayCoordinate[2].x - pScreenSample[2].x * pDisplayCoordinate[0].x) * pScreenSample[1].y +
                                (pScreenSample[1].x * pDisplayCoordinate[0].x - pScreenSample[0].x * pDisplayCoordinate[1].x) * pScreenSample[2].y;
        
        /* D��((YD0��YD2) (Y1��Y2)��(YD1��YD2) (Y0��Y2))��K */
        pCalibrationFactor->Dn = ((pDisplayCoordinate[0].y - pDisplayCoordinate[2].y) * (pScreenSample[1].y - pScreenSample[2].y)) - 
                                ((pDisplayCoordinate[1].y - pDisplayCoordinate[2].y) * (pScreenSample[0].y - pScreenSample[2].y));
        
        /* E��((X0��X2) (YD1��YD2)��(YD0��YD2) (X1��X2))��K */
        pCalibrationFactor->En = ((pScreenSample[0].x - pScreenSample[2].x) * (pDisplayCoordinate[1].y - pDisplayCoordinate[2].y)) - 
                                ((pDisplayCoordinate[0].y - pDisplayCoordinate[2].y) * (pScreenSample[1].x - pScreenSample[2].x));
        
        /* F��(Y0(X2YD1��X1YD2)+Y1(X0YD2��X2YD0)+Y2(X1YD0��X0YD1))��K */
        pCalibrationFactor->Fn = (pScreenSample[2].x * pDisplayCoordinate[1].y - pScreenSample[1].x * pDisplayCoordinate[2].y) * pScreenSample[0].y +
                                (pScreenSample[0].x * pDisplayCoordinate[2].y - pScreenSample[2].x * pDisplayCoordinate[0].y) * pScreenSample[1].y +
                                (pScreenSample[1].x * pDisplayCoordinate[0].y - pScreenSample[0].x * pDisplayCoordinate[1].y) * pScreenSample[2].y;
    }
    
    return ucRet;
}

/**
 * @brief  ��LCD�ϻ���ʮ�ֹ��
 * @param  usX: X����
 * @param  usY: Y����
 * @retval ��
 */
static void ILI9341_DrawCross(uint16_t usX, uint16_t usY)
{
    ILI9341_DrawLine(usX - 10, usY, usX + 10, usY);
    ILI9341_DrawLine(usX, usY - 10, usX, usY + 10);    
}

/**
 * @brief  ��LCD�ϻ���ԲȦ
 * @param  x: Բ��X����
 * @param  y: Բ��Y����
 * @param  radius: �뾶
 * @param  draw: 1��ʾ���ƣ�0��ʾ����(�ñ���ɫ����)
 * @retval ��
 */
static void ILI9341_DrawTouchCircle(uint16_t x, uint16_t y, uint8_t radius, uint8_t draw)
{
    uint16_t textColor, backColor;
    
    // ���浱ǰ��ɫ����
    LCD_GetColors(&textColor, &backColor);
    
    if (draw)
    {
        // ����ԲȦʱʹ�ð�ɫ
        LCD_SetTextColor(WHITE);
    }
    else
    {
        // ����ԲȦʱʹ�ñ���ɫ
        LCD_SetTextColor(backColor);
    }
    
    // ����ԲȦ��ʹ�ÿ���Բ(ucFilled=0)
    ILI9341_DrawCircle(x, y, radius, 0);
    
    // �ָ�ԭ������ɫ
    LCD_SetColors(textColor, backColor);
}
/**
 * @brief  XPT2046 ������У׼
 * @param  LCD_Mode: ָ��ҪУ׼��Һ��ɨ��ģʽ
 * @retval У׼���
 *   ����ֵΪ����ֵ֮һ��
 *     @arg 1: У׼�ɹ�
 *     @arg 0: У׼ʧ��
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
  
    // ����ɨ�跽�򣬺���
    ILI9341_GramScan(LCD_Mode);
    
    // �趨ʮ�ֹ�������
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
        // ������ʾ
        sprintf(cStr, "%*c%s", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - strlen(pStr)) / 2, ' ', pStr);    
        ILI9341_DispStringLine_EN(LCD_Y_LENGTH >> 1, cStr);            
    
        // ������ʾ
        sprintf(cStr, "%*c%d", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - 1) / 2, ' ', i + 1);
        ILI9341_DispStringLine_EN((LCD_Y_LENGTH >> 1) - (((sFONT*)LCD_GetFont())->Height), cStr); 
    
        XPT2046_DelayUS(300000);                     // �ʵ�����ʱ����Ҫ
        
        ILI9341_DrawCross(strCrossCoordinate[i].x, strCrossCoordinate[i].y);  // ��ʾУ���õ�ʮ�ֹ��

        while (!XPT2046_ReadAdc_Smooth_XY(&strScreenSample[i]));               // ��ȡXPT2046���ݵ�����
    }
    
    XPT2046_Calculate_CalibrationFactor(strCrossCoordinate, strScreenSample, &CalibrationFactor);     // ��ԭʼ���������ת��ϵ��
    
    if (CalibrationFactor.Divider == 0) goto Failure;
    
    // У�鴥������׼ȷ�ԣ�ȡһ���������������
    usTest_x = ((CalibrationFactor.An * strScreenSample[3].x) + (CalibrationFactor.Bn * strScreenSample[3].y) + CalibrationFactor.Cn) / CalibrationFactor.Divider;        // ȡһ�������Xֵ    
    usTest_y = ((CalibrationFactor.Dn * strScreenSample[3].x) + (CalibrationFactor.En * strScreenSample[3].y) + CalibrationFactor.Fn) / CalibrationFactor.Divider;    // ȡһ�������Yֵ
    
    usGap_x = (usTest_x > strCrossCoordinate[3].x) ? (usTest_x - strCrossCoordinate[3].x) : (strCrossCoordinate[3].x - usTest_x);   // ʵ��X�������������ľ��Բ�
    usGap_y = (usTest_y > strCrossCoordinate[3].y) ? (usTest_y - strCrossCoordinate[3].y) : (strCrossCoordinate[3].y - usTest_y);   // ʵ��Y�������������ľ��Բ�
    
    if ((usGap_x > 15) || (usGap_y > 15)) goto Failure;       // ����ͨ���޸�������ֵ�Ĵ�С����������    

    // У׼ϵ��Ϊȫ�ֱ���
    g_TouchPara.dX_X = (CalibrationFactor.An * 1.0) / CalibrationFactor.Divider;
    g_TouchPara.dX_Y = (CalibrationFactor.Bn * 1.0) / CalibrationFactor.Divider;
    g_TouchPara.dX = (CalibrationFactor.Cn * 1.0) / CalibrationFactor.Divider;
    
    g_TouchPara.dY_X = (CalibrationFactor.Dn * 1.0) / CalibrationFactor.Divider;
    g_TouchPara.dY_Y = (CalibrationFactor.En * 1.0) / CalibrationFactor.Divider;
    g_TouchPara.dY = (CalibrationFactor.Fn * 1.0) / CalibrationFactor.Divider;
  
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);
  
    LCD_SetTextColor(GREEN);
  
    pStr = "Calibrate Succeed";
    // ������ʾ    
    sprintf(cStr, "%*c%s", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - strlen(pStr)) / 2, ' ', pStr);    
    ILI9341_DispStringLine_EN(LCD_Y_LENGTH >> 1, cStr);    

    XPT2046_DelayUS(1000000);

    return 1;    
  
Failure:
  
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH); 
  
    LCD_SetTextColor(RED);
  
    pStr = "Calibrate fail";    
    // ������ʾ    
    sprintf(cStr, "%*c%s", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - strlen(pStr)) / 2, ' ', pStr);    
    ILI9341_DispStringLine_EN(LCD_Y_LENGTH >> 1, cStr);    

    pStr = "try again";
    // ������ʾ        
    sprintf(cStr, "%*c%s", ((LCD_X_LENGTH / (((sFONT*)LCD_GetFont())->Width)) - strlen(pStr)) / 2, ' ', pStr);    
    ILI9341_DispStringLine_EN((LCD_Y_LENGTH >> 1) + (((sFONT*)LCD_GetFont())->Height), cStr);
	// lcdSprintf((LCD_Y_LENGTH >> 1) + (((sFONT *)LCD_GetFont())->Height),
	// 		   ((LCD_X_LENGTH / (((sFONT *)LCD_GetFont())->Width)) - strlen(pStr)) / 2,
	// 		   "%s", pStr);
		XPT2046_DelayUS(1000000);

	return 0;    
}

/**
 * @brief  ��ȡ�����������
 * @param  displayPtr: ת�������ʾ����
 * @param  para: У׼����
 * @retval 1: �ɹ���0: ʧ��
 */
uint8_t XPT2046_Get_TouchedPoint(XPT2046_Coordinate *displayPtr, XPT2046_TouchPara *para)
{
    XPT2046_Coordinate screenPtr;
    
    if (!XPT2046_ReadAdc_Smooth_XY(&screenPtr))
        return 0;
    
    // ������������ת��Ϊ��ʾ����
    displayPtr->x = para->dX_X * screenPtr.x + para->dX_Y * screenPtr.y + para->dX;
    displayPtr->y = para->dY_X * screenPtr.x + para->dY_Y * screenPtr.y + para->dY;
    
    return 1;
}

/**
 * @brief  ����״̬��⣨��ԲȦ��ʾ��
 * @param  ��
 * @retval ����״̬
 *   ����ֵ��0-�޴�����1-�д���
 */
uint8_t XPT2046_TouchDetect_WithCircle(void)
{
    static uint8_t lastTouchState = 0;
    static XPT2046_Coordinate lastPoint = {0, 0};
    uint8_t currentTouchState;
    XPT2046_Coordinate currentPoint;
    
    // ��⵱ǰ����״̬
    currentTouchState = XPT2046_IsTouched();
    
    // ����д���
    if (currentTouchState)
    {
        // ��ȡУ׼��Ĵ�������
        if (XPT2046_Get_TouchedPoint(&currentPoint, &g_TouchPara))
        {
            // ȷ����������Ļ��Χ��
            if (currentPoint.x < 0) currentPoint.x = 0;
            if (currentPoint.y < 0) currentPoint.y = 0;
            if (currentPoint.x >= LCD_X_LENGTH) currentPoint.x = LCD_X_LENGTH - 1;
            if (currentPoint.y >= LCD_Y_LENGTH) currentPoint.y = LCD_Y_LENGTH - 1;
            
            // ����Ǹտ�ʼ�������ߴ���λ�ñ仯�ϴ�
            if (!lastTouchState || 
                (abs(currentPoint.x - lastPoint.x) > 5) || 
                (abs(currentPoint.y - lastPoint.y) > 5))
            {
                // ���֮ǰ�д������Ȳ�����ԲȦ
                if (lastTouchState)
                {
                    ILI9341_DrawTouchCircle(lastPoint.x, lastPoint.y, 15, 0);
                }
                
                // ������ԲȦ��ȷ��ԲȦ�ڴ�����������
                ILI9341_DrawTouchCircle(currentPoint.x, currentPoint.y, 15, 1);
                
                // �����ϴδ�����
                lastPoint.x = currentPoint.x;
                lastPoint.y = currentPoint.y;
            }
        }
    }
    // ����޴������ϴ��д���
    else if (lastTouchState)
    {
        // �����ϴε�ԲȦ
        ILI9341_DrawTouchCircle(lastPoint.x, lastPoint.y, 15, 0);
    }
    
    // �����ϴδ���״̬
    lastTouchState = currentTouchState;
    
    return currentTouchState;
}
