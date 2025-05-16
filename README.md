流程：

移植ILI9341；

配置CUBEMX FSMC

移植xpt2046触摸库。



相关文件在app里，如不想学习这个移植过程，直接使用即可，遇到问题可发issues.



# 使用流程：

修改API：void ILI9341_BackLed_Control ( FunctionalState enumState )中的引脚PORT与PIN；

修改API:   void ILI9341_Rst ( void )中的引脚PORT与PIN;

main函数：

```
ILI9341_Init();
XPT2046_Init();
ILI9341_GramScan(6);
// 执行触摸屏校准（每次启动时都校准）
// 参数为LCD扫描模式，根据您的LCD配置选择合适的模式（0-7）
while(!XPT2046_Touch_Calibrate(6));  // 假设使用模式6
LCD_SetFont(&Font16x24);
LCD_SetColors(GREEN,BLACK);
```



# 移植流程



基于野火 28-液晶显示 中的 1_液晶显示 修改而来。如果不想修改各种函数API，直接用官方HAL源码。

## bsp_ili9341_lcd.c修改内容

去除初始化：static void ILI9341_GPIO_Config ( void )。第107行

```
/**
  * @brief  初始化ILI9341的IO引脚
  * @param  无
  * @retval 无
  */
//static void ILI9341_GPIO_Config ( void )
//{
//	GPIO_InitTypeDef GPIO_InitStructure;

//	/* 使能FSMC对应相应管脚时钟*/
//	RCC_APB2PeriphClockCmd ( 	
//													/*控制信号*/
//													ILI9341_CS_CLK|ILI9341_DC_CLK|ILI9341_WR_CLK|
//													ILI9341_RD_CLK	|ILI9341_BK_CLK|ILI9341_RST_CLK|
//													/*数据信号*/
//													ILI9341_D0_CLK|ILI9341_D1_CLK|	ILI9341_D2_CLK | 
//													ILI9341_D3_CLK | ILI9341_D4_CLK|ILI9341_D5_CLK|
//													ILI9341_D6_CLK | ILI9341_D7_CLK|ILI9341_D8_CLK|
//													ILI9341_D9_CLK | ILI9341_D10_CLK|ILI9341_D11_CLK|
//													ILI9341_D12_CLK | ILI9341_D13_CLK|ILI9341_D14_CLK|
//													ILI9341_D15_CLK	, ENABLE );
//		
//	
//	/* 配置FSMC相对应的数据线,FSMC-D0~D15 */	
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_AF_PP;
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D0_PIN;
//	GPIO_Init ( ILI9341_D0_PORT, & GPIO_InitStructure );

//	GPIO_InitStructure.GPIO_Pin = ILI9341_D1_PIN;
//	GPIO_Init ( ILI9341_D1_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D2_PIN;
//	GPIO_Init ( ILI9341_D2_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D3_PIN;
//	GPIO_Init ( ILI9341_D3_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D4_PIN;
//	GPIO_Init ( ILI9341_D4_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D5_PIN;
//	GPIO_Init ( ILI9341_D5_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D6_PIN;
//	GPIO_Init ( ILI9341_D6_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D7_PIN;
//	GPIO_Init ( ILI9341_D7_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D8_PIN;
//	GPIO_Init ( ILI9341_D8_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D9_PIN;
//	GPIO_Init ( ILI9341_D9_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D10_PIN;
//	GPIO_Init ( ILI9341_D10_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D11_PIN;
//	GPIO_Init ( ILI9341_D11_PORT, & GPIO_InitStructure );

//	GPIO_InitStructure.GPIO_Pin = ILI9341_D12_PIN;
//	GPIO_Init ( ILI9341_D12_PORT, & GPIO_InitStructure );	
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D13_PIN;
//	GPIO_Init ( ILI9341_D13_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D14_PIN;
//	GPIO_Init ( ILI9341_D14_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_D15_PIN;
//	GPIO_Init ( ILI9341_D15_PORT, & GPIO_InitStructure );
//	

//	
//	/* 配置FSMC相对应的控制线
//	 * FSMC_NOE   :LCD-RD
//	 * FSMC_NWE   :LCD-WR
//	 * FSMC_NE1   :LCD-CS
//	 * FSMC_A16  	:LCD-DC
//	 */
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_AF_PP;
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_RD_PIN; 
//	GPIO_Init (ILI9341_RD_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_WR_PIN; 
//	GPIO_Init (ILI9341_WR_PORT, & GPIO_InitStructure );
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_CS_PIN; 
//	GPIO_Init ( ILI9341_CS_PORT, & GPIO_InitStructure );  
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_DC_PIN; 
//	GPIO_Init ( ILI9341_DC_PORT, & GPIO_InitStructure );
//	

//  /* 配置LCD复位RST控制管脚*/
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_RST_PIN; 
//	GPIO_Init ( ILI9341_RST_PORT, & GPIO_InitStructure );
//	
//	
//	/* 配置LCD背光控制管脚BK*/
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
//	
//	GPIO_InitStructure.GPIO_Pin = ILI9341_BK_PIN; 
//	GPIO_Init ( ILI9341_BK_PORT, & GPIO_InitStructure );
//}
```

去除FSMC模式配置。第222行

```
/**
  * @brief  LCD  FSMC 模式配置
  * @param  无
  * @retval 无
  */
//static void ILI9341_FSMC_Config ( void )
//{
//	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure = {0};
//	FSMC_NORSRAMTimingInitTypeDef  readWriteTiming = {0}; 	
//	
//	/* 使能FSMC时钟*/
//	RCC_AHBPeriphClockCmd ( RCC_AHBPeriph_FSMC, ENABLE );

//	//地址建立时间（ADDSET）为1个HCLK 2/72M=28ns
//	readWriteTiming.FSMC_AddressSetupTime      = 0x01;	 //地址建立时间
//	//数据保持时间（DATAST）+ 1个HCLK = 5/72M=70ns	
//	readWriteTiming.FSMC_DataSetupTime         = 0x04;	 //数据建立时间
//	//选择控制的模式
//	//模式B,异步NOR FLASH模式，与ILI9341的8080时序匹配
//	readWriteTiming.FSMC_AccessMode            = FSMC_AccessMode_B;	
//	
//	/*以下配置与模式B无关*/
//	//地址保持时间（ADDHLD）模式A未用到
//	readWriteTiming.FSMC_AddressHoldTime       = 0x00;	 //地址保持时间
//	//设置总线转换周期，仅用于复用模式的NOR操作
//	readWriteTiming.FSMC_BusTurnAroundDuration = 0x00;
//	//设置时钟分频，仅用于同步类型的存储器
//	readWriteTiming.FSMC_CLKDivision           = 0x00;
//	//数据保持时间，仅用于同步型的NOR	
//	readWriteTiming.FSMC_DataLatency           = 0x00;	

//	
//	FSMC_NORSRAMInitStructure.FSMC_Bank                  = FSMC_Bank1_NORSRAMx;
//	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux        = FSMC_DataAddressMux_Disable;
//	FSMC_NORSRAMInitStructure.FSMC_MemoryType            = FSMC_MemoryType_NOR;
//	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth       = FSMC_MemoryDataWidth_16b;
//	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode       = FSMC_BurstAccessMode_Disable;
//	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity    = FSMC_WaitSignalPolarity_Low;
//	FSMC_NORSRAMInitStructure.FSMC_WrapMode              = FSMC_WrapMode_Disable;
//	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive      = FSMC_WaitSignalActive_BeforeWaitState;
//	FSMC_NORSRAMInitStructure.FSMC_WriteOperation        = FSMC_WriteOperation_Enable;
//	FSMC_NORSRAMInitStructure.FSMC_WaitSignal            = FSMC_WaitSignal_Disable;
//	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode          = FSMC_ExtendedMode_Disable;
//	FSMC_NORSRAMInitStructure.FSMC_WriteBurst            = FSMC_WriteBurst_Disable;
//	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming;
//	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct     = &readWriteTiming;  
//	
//	FSMC_NORSRAMInit ( & FSMC_NORSRAMInitStructure ); 
//	
//	
//	/* 使能 FSMC_Bank1_NORSRAM4 */
//	FSMC_NORSRAMCmd ( FSMC_Bank1_NORSRAMx, ENABLE );  
//		
//		
//}
```

修改初始化函数：void ILI9341_Init ( void )。第607行

注释config。

```
/**
 * @brief  ILI9341初始化函数，如果要用到lcd，一定要调用这个函数
 * @param  无
 * @retval 无
 */
void ILI9341_Init ( void )
{
//	ILI9341_GPIO_Config ();
//	ILI9341_FSMC_Config ();
	
	ILI9341_BackLed_Control ( ENABLE );      //点亮LCD背光灯
	ILI9341_Rst ();
	ILI9341_REG_Config ();
	
	//设置默认扫描方向，其中 6 模式为大部分液晶例程的默认显示方向  
	ILI9341_GramScan(LCD_SCAN_MODE);
}
```

修改背光函数：void ILI9341_BackLed_Control(FunctionalState enumState)。第629行

ILI9341_BK_PORT和PIN改成你对应的引脚即可。

```
void ILI9341_BackLed_Control(FunctionalState enumState)
{
	if (enumState)
		HAL_GPIO_WritePin(ILI9341_BK_PORT, ILI9341_BK_PIN, GPIO_PIN_RESET);
	else
		HAL_GPIO_WritePin(ILI9341_BK_PORT, ILI9341_BK_PIN, GPIO_PIN_SET);
}
```

修改复位函数：void ILI9341_Rst(void)。第682行

ILI9341_RST_PORT和PIN改成你对应的引脚即可。

```
void ILI9341_Rst(void)
{
	HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_RESET); // 低电平复位

	ILI9341_Delay(0xAFF);

	HAL_GPIO_WritePin(ILI9341_RST_PORT, ILI9341_RST_PIN, GPIO_PIN_SET);

	ILI9341_Delay(0xAFF);
}
```

以上是主要修改内容

其他函数API自行修改为hal库

## BSP_ILI9341_LCD.h修改内容

CLK   PORT   PIN 全部删除或注释。仅保留如下（修改后的）

```
///******************************* ILI9341 显示屏8080通讯引脚定义 ***************************/

//复位引脚
#define      ILI9341_RST_CLK               RCC_APB2Periph_GPIOG 
#define      ILI9341_RST_PORT              GPIOG
#define      ILI9341_RST_PIN               GPIO_PIN_11

//背光引脚
#define      ILI9341_BK_CLK                RCC_APB2Periph_GPIOG    
#define      ILI9341_BK_PORT               GPIOG
#define      ILI9341_BK_PIN                GPIO_PIN_6

```

## CUBEMX配置

根据原理图

![image-20250516205243075](\\assets\image-20250516205243075.png)

RS对应引脚为FSMC_A23(这个关乎能否读取到数据)

CUBEMX具体配置如下：

![image-20250516205356112](\\assets\image-20250516205356112.png)