
/* Include core modules */
#include "main.h"

#define LM75_ADDRESS 0x9F

/* Private function prototypes -----------------------------------------------*/
//static void SensorsThread(void const * argument);
static void TouchThread(void const * argument);
static void FatfThread (void const * argument);
static void ScreenDrawThread (void const * argument);

void TM_EXTI_Handler_15(void);
void Init_Timer_for_SD(void);

void Write_Data_to_SD (uint16_t Count);
void Write_Tempr_to_SD (uint16_t Count);

Book_control_struct Book_control={0,0,0,0,0};

uint8_t ScreenDraw_flag = 0;

void Init_CE_Gpio(void);

extern Book_struct book_dat;

	TM_L3GD20_t L3GD20_Data;
  char buffer[452];
	char *p_buffer;
	char file_name_tempr[25], file_name_data[25];
	float temp_f, maximum_rotation;
	
	float real_tempr;
	TM_STMPE811_TouchData Coord_811, Statik_Coord;
	uint16_t flag;
	uint8_t temp;
	TM_BMP180_t BMP180_Data;
	
	FRESULT temp_sd_res;
	
	char Screen_buffer[452] __attribute__((at(0xD0001000)));
	uint16_t mark __attribute__((at(0xD0001000 + sizeof(Screen_buffer))));
  //uint16_t foo[2000][2] __attribute__((at(0xD0000000)));
 
	xTaskHandle xTouchThread;
	xSemaphoreHandle  xMutex_LCD, xWatt_1_sec_measure;
	xSemaphoreHandle  xSDcard_write, xSDcard_written_done;
	
	
		/* Fatfs object */
	FATFS FatFs;
	/* File object */
	FIL fil, fil_Tempr;
	
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

//Array for WattMeasuring
#define MAX_COUNT_ARRAY_WATT 299	
#define TIME_FOR_GET_MEASURE 1000 // in ms
uint16_t Watt[MAX_COUNT_ARRAY_WATT+1];
TM_RTC_t Time[MAX_COUNT_ARRAY_WATT+1], Time_div_10[MAX_COUNT_ARRAY_WATT+1];
float Temperature[MAX_COUNT_ARRAY_WATT+1];
uint32_t Presure[MAX_COUNT_ARRAY_WATT+1];
float avarage_temperature;
uint32_t avarage_preshure;
uint16_t Count_Array_Watt, Count_for_SD_Write, Count_Array_Tempr;

uint8_t Book_stage, new_data=0;
#define UP 10
#define DOWN 30

uint16_t i;


int main(void) {
	
	/* Initialize system */
	
	SystemInit();
	
	/* Initialize system and Delay functions */
	TM_DELAY_Init();
	
	/* Initialize onboard leds */
	TM_DISCO_LedInit();

	/* Initialize button on board */
	TM_DISCO_ButtonInit(); 
	
	Init_CE_Gpio();
	
	if(TM_SDRAM_Init())
		{
			TM_DISCO_LedOn(LED_GREEN);
		}
	
    //Initialize RTC with internal 32768Hz clock
    //It's not very accurate
    if (!TM_RTC_Init(TM_RTC_ClockSource_External)) {
        //RTC was first time initialized
        //set new time
				datatime.hours = 18;
        datatime.minutes = 48;
        datatime.seconds = 00;
        datatime.year = 15;
        datatime.month = 4;
        datatime.date = 19;
        datatime.day = 7;
			//Set new time
        TM_RTC_SetDateTime(&datatime, TM_RTC_Format_BIN);
    }
    
//Set wakeup interrupt every 1 second
//    TM_RTC_Interrupts(TM_RTC_Int_1s);
	
//			/* Set alarm A each day 1 (Monday) in a week */
//            /* Alarm will be first triggered 5 seconds later as time is configured for RTC */
//            AlarmTime.hours = datatime.hours;
//            AlarmTime.minutes = datatime.minutes;
//            AlarmTime.seconds = datatime.seconds + 5;
//            AlarmTime.alarmtype = TM_RTC_AlarmType_DayInWeek;
//            AlarmTime.day = 1;
//            
//            /* Set RTC alarm A, time in binary format */
//            TM_RTC_SetAlarm(TM_RTC_Alarm_A, &AlarmTime, TM_RTC_Format_BIN);
			
			
	/* Write data to backup register 4 */
	TM_RTC_WriteBackupRegister(4, 0x1244);
	 
	/* Initialize ADC1 on channel 13, this is pin PC3*/
	TM_ADC_Init(ADC1, ADC_Channel_13);
		
		
	TM_ILI9341_Init();
	TM_ILI9341_Fill(ILI9341_COLOR_BROWN);
	TM_ILI9341_Rotate(TM_ILI9341_Orientation_Landscape_1);
	BackGround = ILI9341_COLOR_BROWN;
	
	
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
	{
		TM_DISCO_LedOff(LED_GREEN | LED_RED);
	}
	

	if(TM_STMPE811_Init() == TM_STMPE811_State_Ok)
		{
			TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_CTRL, 0x03);
			TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_STA, 0x01);
			TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_EN, 0x01);	
		};
		

		xMutex_LCD = xSemaphoreCreateMutex();
		xWatt_1_sec_measure = xSemaphoreCreateBinary();
		xSDcard_write =  xSemaphoreCreateBinary();
		xSDcard_written_done = xSemaphoreCreateBinary();
		
		if ((xMutex_LCD == NULL) || (xWatt_1_sec_measure == NULL) || (xSDcard_write == NULL))	
			while(1); //Error creation Semaphore
		
		if (xSDcard_written_done == NULL)
			while(1);
	
	//create thread for taken touch sensor data. it will be susspend after all
	osThreadDef(TouchThread, TouchThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
  xTouchThread = osThreadCreate (osThread(TouchThread), NULL);
		
	//create thread for taken touch sensor data. it will be susspend after all
	osThreadDef(ScreenDrawThread, ScreenDrawThread, osPriorityNormal, 0, 1024);
  osThreadCreate (osThread(ScreenDrawThread), NULL);
		
	/* Create Start thread */
  osThreadDef(Fatf_Thread, FatfThread, osPriorityNormal, 0, 1024);
  osThreadCreate (osThread(Fatf_Thread), NULL);
	
  /* Start scheduler */
  osKernelStart(NULL, NULL);

  /* We should never get here as control is now taken by the scheduler */


  while (1)
  {
  }
		

}

static void ScreenDrawThread (void const * argument)
{
	
	for(;;)
	{
		switch(ScreenDraw_flag)
		{
			case SCR_REDRAW:
				TM_ILI9341_Fill(BackGround);
				TM_ILI9341_Puts(0, 0, p_buffer, &TM_Font_11x18 ,ILI9341_COLOR_BLACK, BackGround);
				ScreenDraw_flag = 0;
				break;
			case SCR_DRAW_XY:
				TM_ILI9341_Puts(0, 0, p_buffer, &TM_Font_11x18 ,ILI9341_COLOR_BLACK, BackGround);
				ScreenDraw_flag = 0;
				break;
			default: break;
		}
		
		osDelay(100);
	}
	
	vTaskDelete( NULL );
}

static void FatfThread (void const * argument)
{
	FATFS *fs;
	FRESULT res;
	FIL fil;
	uint8_t i;
	UINT count;

	fs = &FatFs;

	osDelay(1000);
	
	Set_parameters_book();
	
	Read_page(buffer);
	p_buffer = buffer;
	ScreenDraw_flag = SCR_REDRAW;
		
	for(;;)
	{
		osDelay(500);
		TM_DISCO_LedOff(LED_RED);
		osDelay(500);
		TM_DISCO_LedOn(LED_RED);
		
		if (Book_control.screen_pressed== 1)
		{ 
			Book_control.screen_pressed = 0;
			sprintf(buffer, "%03d %03d", Book_control.X, Book_control.Y);//test
			TM_ILI9341_Puts(150, ILI9341_WIDTH - 3*TM_Font_11x18.FontHeight, buffer, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_ORANGE);
			
		}	
			if (Book_control.Buttom_pressed == 1)
			{
				Book_control.Buttom_pressed = 0;
				sprintf(buffer, "%03d", Book_control.time_buttom);//test
				TM_ILI9341_Puts(20, ILI9341_WIDTH - 3*TM_Font_11x18.FontHeight, buffer, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_ORANGE);	
			}
			
		
	}
	
	vTaskDelete( NULL );
}
static void TouchThread(void const * argument)
{
	uint8_t first_pressd=0;
		
		for(;;)
			{
			//waiting for press
			while (TM_STMPE811_ReadTouch(&Coord_811) != TM_STMPE811_State_Pressed)
			{
				osDelay(100); 
				
				if ((first_pressd == 0) && (TM_DISCO_ButtonPressed()))
					{
						first_pressd = 1;
						Book_control.time_buttom = 0;
					}
				if (first_pressd == 1)
				{
					if (TM_DISCO_ButtonPressed())
					{
						Book_control.time_buttom++;
					}
					else 
					{
						first_pressd = 0;
						Book_control.Buttom_pressed = 1;
					}
				}
			}//wait for screen pressed
			
			Book_control.X = Coord_811.x;
			Book_control.Y = Coord_811.y;
			Book_control.screen_pressed = 1;
							
			// waiting for released
			while (TM_STMPE811_ReadTouch(&Coord_811) != TM_STMPE811_State_Released)
				osDelay(50);
			}
		
		vTaskDelete( NULL );
}



//static void SensorsThread(void const * argument)
//{
//	uint8_t tempr[2];
//	uint16_t  ADC_Value;
//	
//	osDelay(1900);
// 
//  /* Infinite loop */
//  for(;;)
//  {
//		Measure_BMP180(&BMP180_Data, TM_BMP180_Oversampling_HighResolution);
//				
//		if( xMutex_LCD != NULL )
//			{
//        // See if we can obtain the semaphore.  If the semaphore is not available
//        // wait 10 ticks to see if it becomes free.	
//        if( xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE )
//        {
//            // We were able to obtain the semaphore and can now access the shared resource.
//						sprintf(buffer, "T=%2.3f Pr=%6d A=%3.2f", BMP180_Data.Temperature, BMP180_Data.Pressure, BMP180_Data.Altitude);
//						TM_ILI9341_Puts(10, 40, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);
//					
//            // We have finished accessing the shared resource.  Release the semaphore.
//            xSemaphoreGive( xMutex_LCD );
//        }
//        else
//        {
//            // We could not obtain the semaphore and can therefore not access
//            // the shared resource safely.
//        }
//			}
//			
//			
//		if( xMutex_LCD != NULL )
//		{
//			// See if we can obtain the semaphore.  If the semaphore is not available
//			// wait 10 ticks to see if it becomes free.	
//			if( xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE )
//			{
//					// We were able to obtain the semaphore and can now access the shared resource.
//					//Format time
//					sprintf(buffer, "%02d.%02d.%04d %02d:%02d:%02d  ADC: %u\n", datatime.date, datatime.month, datatime.year + 2000, datatime.hours, datatime.minutes, datatime.seconds, ADC_Value);
//					TM_ILI9341_Puts(10, 180, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);
//				
//					// We have finished accessing the shared resource.  Release the semaphore.
//					xSemaphoreGive( xMutex_LCD );
//			}
//		}
//	
//		TM_I2C_ReadMulti(STMPE811_I2C, LM75_ADDRESS, 0x00, tempr, 2); // Read temperature from LM75
//		real_tempr = (float)tempr[0] + 0.125*(tempr[1]>>5);
//			
//		if( xMutex_LCD != NULL )
//			{
//        // See if we can obtain the semaphore.  If the semaphore is not available
//        // wait 10 ticks to see if it becomes free.	
//        if( xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE )
//        {
//            // We were able to obtain the semaphore and can now access the shared resource.
//						sprintf(buffer, "T_LM75 = %.3f", real_tempr);
//						TM_ILI9341_Puts(10, 60, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_GREEN);
//					
//            // We have finished accessing the shared resource.  Release the semaphore.
//            xSemaphoreGive( xMutex_LCD );
//        }
//        else
//        {
//            // We could not obtain the semaphore and can therefore not access
//            // the shared resource safely.
//        }
//			}			
//			

//		
///* Read acceleration data */
//      TM_L3GD20_Read(&L3GD20_Data);
//			temp_f = sqrt(L3GD20_Data.X*L3GD20_Data.X + L3GD20_Data.Y*L3GD20_Data.Y + L3GD20_Data.Z*L3GD20_Data.Z);
//			if (maximum_rotation < temp_f)
//			maximum_rotation = temp_f;
//	
//		if( xMutex_LCD != NULL )
//			{
//        // See if we can obtain the semaphore.  If the semaphore is not available
//        // wait 10 ticks to see if it becomes free.	
//        if( xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE )
//        {
//            // We were able to obtain the semaphore and can now access the shared resource.
//							sprintf(buffer, "X = %4d, Y = %4d, Z = %4d", L3GD20_Data.X, L3GD20_Data.Y, L3GD20_Data.Z);
//							TM_ILI9341_Puts(10, 80, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);

//							sprintf(buffer, "M = %.3f, MAX = %.3f", temp_f, maximum_rotation);
//							TM_ILI9341_Puts(10, 100, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_BLUE);
//					
//            // We have finished accessing the shared resource.  Release the semaphore.
//            xSemaphoreGive( xMutex_LCD );
//        }
//        else
//        {
//            // We could not obtain the semaphore and can therefore not access
//            // the shared resource safely.
//        }
//			}		

//			if (TM_DISCO_ButtonPressed()) maximum_rotation = 0;
//						
//		osDelay(300);
//	}
//	
//		vTaskDelete( NULL );
//}




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
	
  GPIO_SetBits(GPIOD, GPIO_Pin_7);// CE for MPL115A1
	GPIO_SetBits(GPIOD, GPIO_Pin_5);// CSN for NRF24L01

}

//Custom request handler function
//Called on wakeup interrupt
void TM_RTC_RequestHandler() 
{


}

/* Custom request handler function */
/* Called on alarm A interrupt */
void TM_RTC_AlarmAHandler(void) 
{
    TM_DISCO_LedToggle(LED_RED);
    
		TM_DISCO_LedOn(LED_RED | LED_GREEN);
    /* Disable Alarm so it will not trigger next week at the same time */
    TM_RTC_DisableAlarm(TM_RTC_Alarm_A);
}



void TM_EXTI_Handler_15(void)
{
		TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_STA, 0x01);
		TM_DISCO_LedToggle(LED_RED);
}

void Init_Timer_for_SD(void)
{
	TIM_TimeBaseInitTypeDef tim;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
	
	TIM_TimeBaseStructInit(&tim);
	tim.TIM_Prescaler = 84-1; //1MHz
	tim.TIM_Period = 0x0F4240;//1s
	TIM_TimeBaseInit(TIM5, &tim);
	
	TIM_Cmd(TIM6, ENABLE);	
}


void Write_Data_to_SD (uint16_t Count)
{
	uint16_t i;
			if (f_mount(&FatFs, "0:", 1) == FR_OK) {
								
				/* Try to open file */
				if (f_open(&fil, file_name_data, FA_OPEN_ALWAYS | FA_READ | FA_WRITE) == FR_OK) 
					{
			
					for (i=0; i<=Count; i++)
						{
							// We were able to obtain the semaphore and can now access the shared resource.
							datatime = Time[i];
							sprintf(buffer, "%02d.%02d.%04d\t%02d:%02d:%02d\t%u\t%.2f\t%u\n", datatime.date, datatime.month, datatime.year + 2000, datatime.hours, datatime.minutes, datatime.seconds, Watt[i], Temperature[i], Presure[i]);
							if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
							
							/* If we put more than 0 characters (everything OK) */
							if (f_puts(buffer, &fil) > 0) {
								
							}
						}
				
					/* Close file, don't forget this! */
					f_close(&fil);
				}
				
				/* Unmount drive, don't forget this! */
				f_mount(0, "0:", 1);
			}	
}



void Write_Tempr_to_SD (uint16_t Count)
{
	uint16_t i;
			if (f_mount(&FatFs, "0:", 1) == FR_OK) {
								
				/* Try to open file */
				if (f_open(&fil, file_name_tempr, FA_OPEN_ALWAYS | FA_READ | FA_WRITE) == FR_OK) 
					{
			
					for (i=0; i<=Count; i++)
						{
							// We were able to obtain the semaphore and can now access the shared resource.
							datatime = Time[i];
							sprintf(buffer, "%02d.%02d.%04d\t%02d:%02d:%02d\t%u\t%.2f\t%u\n", datatime.date, datatime.month, datatime.year + 2000, datatime.hours, datatime.minutes, datatime.seconds, Watt[i], Temperature[i], Presure[i]);
							if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
							
							/* If we put more than 0 characters (everything OK) */
							if (f_puts(buffer, &fil) > 0) {
								
							}
						}
				
					/* Close file, don't forget this! */
					f_close(&fil);
				}
				
				/* Unmount drive, don't forget this! */
				f_mount(0, "0:", 1);
			}	
}


//trash 
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


//-------------nothing down ----------------------------

