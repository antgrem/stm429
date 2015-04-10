/**
 *	Keil project for NRF24L01+ transceiver
 *
 *	Transmitter code
 *
 *	Before you start, select your target, on the right of the "Load" button
 *
 *	@author		Tilen Majerle
 *	@email		tilen@majerle.eu
 *	@website	http://stm32f4-discovery.com
 *	@ide		Keil uVision 5
 *	@packs		STM32F4xx Keil packs version 2.2.0 or greater required
 *	@stdperiph	STM32F4xx Standard peripheral drivers version 1.4.0 or greater required
 */

/* Include core modules */
#include "stm32f4xx.h"
/* Include my libraries here */
#include "defines.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_rtc.h"
#include "tm_stm32f4_adc.h"
#include "tm_stm32f4_bmp180.h"
//#include "tm_stm32f4_usart.h"
//#include "tm_stm32f4_ili9341.h"
#include "tm_stm32f4_ili9341_ltdc.h"
#include "tm_stm32f4_nrf24l01.h"
#include "tm_stm32f4_i2c.h"
#include "tm_stm32f4_stmpe811.h"
#include <stdio.h>
#include <math.h>
#include "tm_stm32f4_l3gd20.h"
#include "cmsis_os.h"
#include <MPL115A1.h>
#include "tm_stm32f4_fatfs.h"
#include "tm_stm32f4_exti.h"



/* Private function prototypes -----------------------------------------------*/
static void StartThread(void const * argument);
static void SensorsThread(void const * argument);
static void SDCardThread(void const * argument);
static void TouchThread(void const * argument);
void TM_EXTI_Handler_15(void);

void Init_CE_Gpio(void);

	TM_L3GD20_t L3GD20_Data;
  char buffer[100];
	float temp_f, maximum_rotation;
	
	MPL115A1_CoefTypeDef Coef;
	MPL115A1_PressureTypeDef MPL_Data;
	
	float real_tempr;
	TM_STMPE811_TouchData Coord_811, Statik_Coord;
	uint16_t flag;
	uint8_t temp;
	TM_BMP180_t BMP180_Data;
	
	xTaskHandle xTouchThread;
	
	
		/* Fatfs object */
	FATFS FatFs;
	/* File object */
	FIL fil;
	/* Free and total space */
	uint32_t total, free;
	
/* My address */
uint8_t MyAddress[] = {
	0xE7,
	0xE7,
	0xE7,
	0xE7,
	0xE7
};
/* Receiver address */
uint8_t TxAddress[] = {
	0x7E,
	0x7E,
	0x7E,
	0x7E,
	0x7E
};

uint8_t dataOut[32], dataIn[32];
uint8_t nrf_status;
uint32_t BackGround;
TM_RTC_t datatime;
TM_RTC_AlarmTime_t AlarmTime;

int main(void) {
//	TM_NRF24L01_Transmit_Status_t transmissionStatus;
	char str[40];
	uint16_t value, temp;
	TM_STMPE811_State_t Status_stmpe811;
	TM_STMPE811_TouchData Touch_data_stmpe811;
/* L3GD20 Struct */

	
	
	
	/* Initialize system */
	SystemInit();
	
	/* Initialize system and Delay functions */
	TM_DELAY_Init();
	
	/* Initialize onboard leds */
	TM_DISCO_LedInit();
	
	/* Initialize button on board */
	TM_DISCO_ButtonInit(); 
	
    //Initialize RTC with internal 32768Hz clock
    //It's not very accurate
    if (!TM_RTC_Init(TM_RTC_ClockSource_Internal)) {
        //RTC was first time initialized
        //set new time
			datatime.hours = 0;
            datatime.minutes = 59;
            datatime.seconds = 45;
            datatime.year = 15;
            datatime.month = 4;
            datatime.date = 9;
            datatime.day = 6;
			//Set new time
            TM_RTC_SetDateTime(&datatime, TM_RTC_Format_BIN);
    }
    
    //Set wakeup interrupt every 1 second
    TM_RTC_Interrupts(TM_RTC_Int_1s);
	
			/* Set alarm A each day 1 (Monday) in a week */
            /* Alarm will be first triggered 5 seconds later as time is configured for RTC */
            AlarmTime.hours = datatime.hours;
            AlarmTime.minutes = datatime.minutes;
            AlarmTime.seconds = datatime.seconds + 5;
            AlarmTime.alarmtype = TM_RTC_AlarmType_DayInWeek;
            AlarmTime.day = 1;
            
            /* Set RTC alarm A, time in binary format */
            TM_RTC_SetAlarm(TM_RTC_Alarm_A, &AlarmTime, TM_RTC_Format_BIN);
			
			
	 /* Write data to backup register 4 */
     TM_RTC_WriteBackupRegister(4, 0x1244);
	 
    /* Initialize ADC1 on channel 13, this is pin PC3*/
    TM_ADC_Init(ADC1, ADC_Channel_13);
	
	 /* Initialize BMP180 pressure sensor */
    if (TM_BMP180_Init(&BMP180_Data) == TM_BMP180_Result_Ok) {
        /* Init OK */
       // TM_USART_Puts(USART1, "BMP180 configured and ready to use\n\n");
    } else {
        /* Device error */
       // TM_USART_Puts(USART1, "BMP180 error\n\n");
        while (1);
    }
	
	
	/* Attach interrupt on pin PA15 = External line 15 */
	/* Button connected on discovery boards */
	if (TM_EXTI_Attach(GPIOA, GPIO_Pin_15, TM_EXTI_Trigger_Falling) == TM_EXTI_Result_Ok) {
		TM_DISCO_LedOn(LED_RED);
	} 
	
	Init_CE_Gpio();
	

	
	/* Initialize NRF24L01+ on channel 15 and 32bytes of payload */
	/* By default 2Mbps data rate and 0dBm output power */
	/* NRF24L01 goes to RX mode by default */
	TM_NRF24L01_Init(15, 32);
	
	/* Set 2MBps data rate and -18dBm output power */
	TM_NRF24L01_SetRF(TM_NRF24L01_DataRate_2M, TM_NRF24L01_OutputPower_M18dBm);
	
	/* Set my address, 5 bytes */
	TM_NRF24L01_SetMyAddress(MyAddress);
	/* Set TX address, 5 bytes */
	TM_NRF24L01_SetTxAddress(TxAddress);
	
	
	if (TM_L3GD20_Init(TM_L3GD20_Scale_250) == TM_L3GD20_Result_Ok)
	TM_DISCO_LedOff(LED_GREEN | LED_RED);
		
	TM_ILI9341_Init();
	TM_ILI9341_Fill(ILI9341_COLOR_BROWN);
	TM_ILI9341_Rotate(TM_ILI9341_Orientation_Landscape_1);
	/* Put string with black foreground color and blue background with 11x18px font */
	TM_ILI9341_Puts(120, 03, "Sensor's", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BLUE2);
	BackGround = ILI9341_COLOR_BROWN;
	
	if(TM_STMPE811_Init() == TM_STMPE811_State_Ok)
		{
			temp = TM_I2C_Read(STMPE811_I2C, 0x9F, 0x00);
			TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_CTRL, 0x03);
			TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_STA, 0x01);
			TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_EN, 0x01);	
				flag = 0;
			temp = TM_I2C_Read(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_EN);
			sprintf(buffer, "%d", temp);
			TM_ILI9341_Puts(10, 10, buffer, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BLUE2);
		};

		
		
	
	//create thread for taken touch sensor data. it will be susspend after all
//	osThreadDef(TouchThread, TouchThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
//  xTouchThread = osThreadCreate (osThread(TouchThread), NULL);
		
		
	osThreadDef(SensorsThread, SensorsThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
  osThreadCreate (osThread(SensorsThread), NULL);
	
//	osThreadDef(SD_Thread, SDCardThread, osPriorityAboveNormal, 0, configMINIMAL_STACK_SIZE);
//  osThreadCreate (osThread(SD_Thread), NULL);	

  /* Create Start thread */
  osThreadDef(USER_Thread, StartThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
  osThreadCreate (osThread(USER_Thread), NULL);
	
  /* Start scheduler */
  osKernelStart(NULL, NULL);

  /* We should never get here as control is now taken by the scheduler */


  while (1)
  {
  }
		

	
//===============================
//		if (LM75_Init(100000))
//		while(1);
//	


//		

//    value = LM75_ReadReg(0x00);
//		TM_ILI9341_Puts(60, 03, "werfdfs", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BLUE2);
//	
//	value = LM75_ReadConf();

//	value = LM75_ReadReg(0x02);

//	value = LM75_ReadReg(0x03);

//	LM75_Shutdown(DISABLE);

//    temp = LM75_Temperature();
//    UART_SendInt(temp / 10); UART_SendChar('.');
//    temp %= 10;
//    if (temp < 0) temp *= -1;
//    UART_SendInt(temp % 10); UART_SendStr("C\n");
	
//=======================================	
	
//	/* Initialize NRF24L01+ on channel 15 and 32bytes of payload */
//	/* By default 2Mbps data rate and 0dBm output power */
//	/* NRF24L01 goes to RX mode by default */
//	TM_NRF24L01_Init(15, 32);
//	
//	/* Set 2MBps data rate and -18dBm output power */
//	TM_NRF24L01_SetRF(TM_NRF24L01_DataRate_2M, TM_NRF24L01_OutputPower_M18dBm);
//	
//	/* Set my address, 5 bytes */
//	TM_NRF24L01_SetMyAddress(MyAddress);
//	/* Set TX address, 5 bytes */
//	TM_NRF24L01_SetTxAddress(TxAddress);
//	
//	/* Reset counter */
//	TM_DELAY_SetTime(2001);
//	while (1) {
//		/* Every 2 seconds */
//		if (TM_DELAY_Time() > 2000) {
//			/* Fill data with something */
//			sprintf((char *)dataOut, "abcdefghijklmnoszxABCDEFCBDA");
//			/* Display on USART */
//			TM_USART_Puts(USART1, "pinging: ");
//			/* Reset time, start counting microseconds */
//			TM_DELAY_SetTime(0);
//			/* Transmit data, goes automatically to TX mode */
//			TM_NRF24L01_Transmit(dataOut);
//			
//			/* Turn on led to indicate sending */
//			TM_DISCO_LedOn(LED_GREEN);
//			/* Wait for data to be sent */
//			do {
//				transmissionStatus = TM_NRF24L01_GetTransmissionStatus();
//			} while (transmissionStatus == TM_NRF24L01_Transmit_Status_Sending);
//			/* Turn off led */
//			TM_DISCO_LedOff(LED_GREEN);
//			
//			/* Go back to RX mode */
//			TM_NRF24L01_PowerUpRx();
//			
//			/* Wait received data, wait max 100ms, if time is larger, then data were probably lost */
//			while (!TM_NRF24L01_DataReady() && TM_DELAY_Time() < 100);
//			
//			/* Format time */
//			sprintf(str, "%d ms", TM_DELAY_Time());
//			/* Show ping time */
//			TM_USART_Puts(USART1, str);
//			
//			/* Get data from NRF2L01+ */
//			TM_NRF24L01_GetData(dataIn);
//			
//			/* Check transmit status */
//			if (transmissionStatus == TM_NRF24L01_Transmit_Status_Ok) {
//				/* Transmit went OK */
//				TM_USART_Puts(USART1, ": OK\n");
//				sprintf(str, "Type of MS: %c", (char)dataIn[0]);
//				TM_ILI9341_Puts(05, 23, str, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BLUE2);
//				sprintf(str, "Counter: %d", (char)dataIn[1]);
//				TM_ILI9341_Puts(05, 43, str, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BLUE2);
//				
//				sprintf(str, "Data: %s", (dataIn+2));
//				TM_ILI9341_Puts(05, 63, str, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BLUE2);
//				
//				sprintf(str, "Data: %s", (dataIn+2));
//				TM_ILI9341_Puts(05, 63, str, &TM_Font_11x18, BackGround, BackGround);
//			} else if (transmissionStatus == TM_NRF24L01_Transmit_Status_Lost) {
//				/* Message was LOST */
//				TM_USART_Puts(USART1, ": LOST\n");
//			} else {
//				/* This should never happen */
//				TM_USART_Puts(USART1, ": SENDING\n");
//			}
//		}
//	}

}


static void TouchThread(void const * argument)
{
	uint8_t count = 0;
	

		for(;;)
			{
					while (TM_STMPE811_ReadTouch(&Coord_811) != TM_STMPE811_State_Pressed)
						osDelay(5);
					
			/* Draw pixel on touch location */
			TM_ILI9341_DrawCircle(Coord_811.y,(ILI9341_WIDTH - Coord_811.x), 2, ILI9341_COLOR_GREEN);

				
				/* Touch valid */
				sprintf(buffer, "X: %03d  Y: %03d", Coord_811.x, Coord_811.y);
				TM_ILI9341_Puts(20, 80, buffer, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_ORANGE);
					
					while (TM_STMPE811_ReadTouch(&Coord_811) != TM_STMPE811_State_Released)
						osDelay(50);
				
		
			}
		
		vTaskDelete( NULL );
}


static void StartThread(void const * argument)
{
//Thread show all works good
	portTickType xLastWakeTime;
	
	xLastWakeTime = xTaskGetTickCount();

  for(;;)
  {
    		 		  
   	GPIO_ToggleBits(GPIOG, GPIO_Pin_13);
		
		osDelay(1000);
//		osDelayUntil(xLastWakeTime, 1000);
  }

}


static void SDCardThread(void const * argument)
{
	//thread for work with CD_Card by SPI4
	FRESULT temp_sd_res;
	uint16_t time=0;

	if (f_mount(&FatFs, "0:", 1) == FR_OK) {
		/* Mounted OK, turn on RED LED */
			TM_DISCO_LedOn(LED_RED);	
				
		temp_sd_res = f_open(&fil, "0:Tempr.txt", FA_OPEN_EXISTING | FA_READ | FA_WRITE);
		if (temp_sd_res != FR_OK) 
			{
				if (f_open(&fil, "0:Tempr.txt", FA_CREATE_NEW | FA_READ | FA_WRITE) == FR_OK)
					{//write redline
						sprintf(buffer, "Numb\tTemp\tX\tY\tZ\n");
						if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
							
							/* If we put more than 0 characters (everything OK) */
							if (f_puts(buffer, &fil) > 0) {
								if (TM_FATFS_DriveSize(&total, &free) == FR_OK) {
									/* Data for drive size are valid */
									/* Close file, don't forget this! */
									f_close(&fil);
								}
					}
				}
			
				}
			else
			{
				//file existing and open
								sprintf(buffer, "------------------------\n");
								if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
									
									/* If we put more than 0 characters (everything OK) */
									if (f_puts(buffer, &fil) > 0) 
										{
											if (TM_FATFS_DriveSize(&total, &free) == FR_OK) {
												/* Data for drive size are valid */
												/* Close file, don't forget this! */
												
											}
										}
					f_close(&fil);
				}
			
//				sprintf(buffer, "0:Hello.txt");	
//			if (f_open(&fil, buffer, FA_CREATE_ALWAYS | FA_READ | FA_WRITE) == FR_OK)
//				{
//					f_close(&fil);
//				}
				
				/* Unmount drive, don't forget this! */
				f_mount(0, "0:", 1);
				
			}
	
  for(;;)
  {
		
		
		if (f_mount(&FatFs, "0:", 1) == FR_OK) {
				/* Mounted OK, turn on RED LED */
			TM_DISCO_LedOn(LED_RED);	
				
				/* Try to open file */
				if (f_open(&fil, "0:Tempr.txt", FA_OPEN_ALWAYS | FA_READ | FA_WRITE) == FR_OK) {
					/* File opened, turn off RED and turn on GREEN led */
			//		TM_DISCO_LedOn(LED_GREEN);
			//		TM_DISCO_LedOff(LED_RED);
					
					
					sprintf(buffer, "%d\t%.3f\t%d\t%d\t%d\n", time++, real_tempr, L3GD20_Data.X, L3GD20_Data.Y, L3GD20_Data.Z);
					if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
					
					/* If we put more than 0 characters (everything OK) */
					if (f_puts(buffer, &fil) > 0) {
						if (TM_FATFS_DriveSize(&total, &free) == FR_OK) {
							/* Data for drive size are valid */
							
						}
						
						/* Turn on both leds */
			//			TM_DISCO_LedOn(LED_GREEN | LED_RED);
					}
					
					/* Close file, don't forget this! */
					f_close(&fil);
				}
				
				/* Unmount drive, don't forget this! */
				f_mount(0, "0:", 1);
			}	
		TM_DISCO_LedOff(LED_RED);
   	osDelay(5000); 
  }

}



static void SensorsThread(void const * argument)
{
	uint8_t tempr[2];

	
	MPL115_Init();

	MPL115_Read_coef(&Coef);
	
	osDelay(1900);
 
  /* Infinite loop */
  for(;;)
  {
		
		/* Start temperature conversion */
        TM_BMP180_StartTemperature(&BMP180_Data);
        
        /* Wait delay in microseconds */
        /* You can do other things here instead of delay */
        osDelay(BMP180_Data.Delay);
        
        /* Read temperature first */
        TM_BMP180_ReadTemperature(&BMP180_Data);
        
        /* Start pressure conversion at ultra high resolution */
        TM_BMP180_StartPressure(&BMP180_Data, TM_BMP180_Oversampling_UltraHighResolution);
        
        /* Wait delay in microseconds */
        /* You can do other things here instead of delay */
        osDelay(BMP180_Data.Delay);
        
        /* Read pressure value */
        TM_BMP180_ReadPressure(&BMP180_Data);
        
        /* Format data and print to USART */
        sprintf(buffer, "Temp: %2.3f degrees\nPressure: %6d Pascals\nAltitude at current pressure: %3.2f meters\n\n",
            BMP180_Data.Temperature,
            BMP180_Data.Pressure,
            BMP180_Data.Altitude
        );
			
			MPL115_Start_Conversion();
		
			osDelay(2);
		
			MPL115_Read_Data(&MPL_Data);
		
			temp_f = MPL115A1_calc_pressure(&Coef, &MPL_Data);
		
			TM_I2C_ReadMulti(STMPE811_I2C, 0x9F, 0x00, tempr, 2);
			real_tempr = (float)tempr[0] + 0.125*(tempr[1]>>5);
		
			/* Read data */
      TM_L3GD20_Read(&L3GD20_Data);
			temp_f = sqrt(L3GD20_Data.X*L3GD20_Data.X + L3GD20_Data.Y*L3GD20_Data.Y + L3GD20_Data.Z*L3GD20_Data.Z);
		
/* Display data on LCD */
				
        sprintf(buffer, "X rotation: %4d", L3GD20_Data.X);
        TM_ILI9341_Puts(10, 40, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);
        sprintf(buffer, "Y rotation: %4d", L3GD20_Data.Y);
        TM_ILI9341_Puts(10, 60, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);
        sprintf(buffer, "Z rotation: %4d", L3GD20_Data.Z);
        TM_ILI9341_Puts(10, 80, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);
			  sprintf(buffer, "Mod rotation: %.3f", temp_f);
        TM_ILI9341_Puts(10, 100, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_BLUE);
			
			if (maximum_rotation < temp_f)
				maximum_rotation = temp_f; 
			sprintf(buffer, "MAX rotation: %f", maximum_rotation);
      TM_ILI9341_Puts(10, 120, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_BLUE);

			sprintf(buffer, "Temperature: %.3f", real_tempr);
			TM_ILI9341_Puts(10, 200, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_GREEN);			

			if (TM_DISCO_ButtonPressed()) maximum_rotation = 0;
		
						
		osDelay(300);
	}
}

void Init_CE_Gpio(void)
{
 // 
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_5 | GPIO_Pin_4 | GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
	
  GPIO_SetBits(GPIOD, GPIO_Pin_7);
	GPIO_SetBits(GPIOD, GPIO_Pin_5);

}

//Custom request handler function
//Called on wakeup interrupt
void TM_RTC_RequestHandler() 
{
	uint16_t  ADC_Value;
    //Get time
    TM_RTC_GetDateTime(&datatime, TM_RTC_Format_BIN);
	
	//Read ADC1 channel 13
	ADC_Value = TM_ADC_Read(ADC1, ADC_Channel_13);
    
    //Format time
    sprintf(buffer, "%02d.%02d.%04d %02d:%02d:%02d  ADC: %u\n",
                datatime.date,
                datatime.month,
                datatime.year + 2000,
                datatime.hours,
                datatime.minutes,
                datatime.seconds,
                ADC_Value
    );
    //Send to USART
    TM_ILI9341_Puts(10, 250, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);

}

/* Custom request handler function */
/* Called on alarm A interrupt */
void TM_RTC_AlarmAHandler(void) {
    /* Show user to USART */
    TM_ILI9341_Puts(10, 230, "Alarm A triggered\n", &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);
    
    /* Disable Alarm so it will not trigger next week at the same time */
    //TM_RTC_DisableAlarm(TM_RTC_Alarm_A);
}



void TM_EXTI_Handler_15(void)
{
		TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_STA, 0x01);
		TM_DISCO_LedToggle(LED_RED);
}
