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
#include "main.h"

#define LM75_ADDRESS 0x9F

/* Private function prototypes -----------------------------------------------*/
static void StartThread(void const * argument);
static void SensorsThread(void const * argument);
static void SDCardThread(void const * argument);
static void TouchThread(void const * argument);
void TM_EXTI_Handler_15(void);
void Init_Timer_for_SD(void);

void Write_Data_to_SD (uint16_t Count);
void Write_Tempr_to_SD (uint16_t Count);

void Init_CE_Gpio(void);

	TM_L3GD20_t L3GD20_Data;
  char buffer[100];
	char file_name_tempr[25], file_name_data[25];
	float temp_f, maximum_rotation;
	
	double real_tempr;
	TM_STMPE811_TouchData Coord_811, Statik_Coord;
	uint16_t flag;
	uint8_t temp;
	TM_BMP180_t BMP180_Data;
	
	FRESULT temp_sd_res;
	

//uint16_t foo[2000][2] __attribute__((at(0xD0001000)));
 
	xTaskHandle xTouchThread;
	xSemaphoreHandle  xMutex_LCD, xWatt_1_sec_measure;
	xSemaphoreHandle  xSDcard_write, xSDcard_written_done;
	
	
		/* Fatfs object */
	FATFS FatFs;
	/* File object */
	FIL fil, fil_Tempr;
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

//Array for WattMeasuring
#define MAX_COUNT_ARRAY_WATT 299	
#define TIME_FOR_GET_MEASURE 1000 // in ms
uint16_t Watt[MAX_COUNT_ARRAY_WATT+1];
TM_RTC_t Time[MAX_COUNT_ARRAY_WATT+1], Time_div_10[MAX_COUNT_ARRAY_WATT+1];
double Temperature[MAX_COUNT_ARRAY_WATT+1];
uint32_t Presure[MAX_COUNT_ARRAY_WATT+1];
double avarage_temperature;
uint32_t avarage_preshure;
uint16_t Count_Array_Watt, Count_for_SD_Write, Count_Array_Tempr;
uint32_t time_for_mount, time_for_open, time_for_write;

int main(void) {
	
	/* Initialize system */
	
	SystemInit();
	
	Count_Array_Watt = 0;
	Count_Array_Tempr = 0;
	
	/* Initialize system and Delay functions */
	TM_DELAY_Init();
	
	/* Initialize onboard leds */
	TM_DISCO_LedInit();
	
	/* Initialize button on board */
	TM_DISCO_ButtonInit(); 
	
	Init_CE_Gpio();
	
//	Init_Timer_for_SD();
	
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
	/* Put string with black foreground color and blue background with 11x18px font */
	TM_ILI9341_Puts(120, 03, "Sensor's", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_BLUE2);
	BackGround = ILI9341_COLOR_BROWN;
	
	TM_ADC_Read(ADC1, ADC_Channel_13);
	
	
	/* Attach interrupt on pin PA15 = External line 15 */
	/* Button connected on discovery boards */
//	if (TM_EXTI_Attach(GPIOA, GPIO_Pin_15, TM_EXTI_Trigger_Falling) == TM_EXTI_Result_Ok) {
//		TM_DISCO_LedOn(LED_RED);
//	} 

	
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
		

	if(TM_STMPE811_Init() == TM_STMPE811_State_Ok)
		{
			if (TM_I2C_Read(STMPE811_I2C, LM75_ADDRESS, 0x00) == 0)
			{
				TM_ILI9341_Puts(60, 40, "Error init LM75", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_RED);
			}
						
			TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_CTRL, 0x03);
			TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_STA, 0x01);
			TM_I2C_Write(STMPE811_I2C, STMPE811_ADDRESS, STMPE811_INT_EN, 0x01);	
			
		/* Initialize BMP180 pressure sensor */
    if (TM_BMP180_Init(&BMP180_Data) == TM_BMP180_Result_Ok) {
        /* Init OK */
       // TM_USART_Puts(USART1, "BMP180 configured and ready to use\n\n");
    } else {
        /* Device error */
       	TM_ILI9341_Puts(120, 20, "Error init BMP180", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_RED);
    }
		
		};
		

		
 
			if (f_mount(&FatFs, "0:", 1) == FR_OK) 
				{
//							//Get time
//						TM_RTC_GetDateTime(&datatime, TM_RTC_Format_BIN);
//				//	sprintf(buffer, "0:F%02d_%02d_%04d.txt", datatime.date, datatime.month, datatime.year);
//						sprintf(file_name_data, "0:%04d_%02d_%02d_Watt.txt", datatime.year+2000, datatime.month, datatime.date);
//					temp_sd_res = f_open(&fil, (TCHAR*) file_name_data, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
//							if (temp_sd_res != FR_OK) 
//								{
//									if (f_open(&fil, file_name_data, FA_CREATE_NEW | FA_READ | FA_WRITE) == FR_OK)
//										{//write redline
//											sprintf(buffer, "Data\t\tTime\t\tVoltage\tCurrent\tWatt\n");
//											if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
//												
//												/* If we put more than 0 characters (everything OK) */
//												if (f_puts(buffer, &fil) > 0) {
//													if (TM_FATFS_DriveSize(&total, &free) == FR_OK) {
//														/* Data for drive size are valid */
//														/* Close file, don't forget this! */
//														f_close(&fil);
//													}
//											}
//										}
//									}
//								else f_close(&fil);//file exists, was openned and must be closed
//									
//							sprintf(file_name_tempr, "0:%04d_%02d_%02d_Tempr_Pr.txt", datatime.year+2000, datatime.month, datatime.date);
//							temp_sd_res = f_open(&fil, (TCHAR*) file_name_tempr, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
//							if (temp_sd_res != FR_OK) 
//								{
//									if (f_open(&fil, file_name_tempr, FA_CREATE_NEW | FA_READ | FA_WRITE) == FR_OK)
//										{//write redline
//											sprintf(buffer, "Data\t\tTime\t\tTempr\tPresure\n");
//											if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
//												
//												/* If we put more than 0 characters (everything OK) */
//												if (f_puts(buffer, &fil) > 0) {
//													if (TM_FATFS_DriveSize(&total, &free) == FR_OK) {
//														/* Data for drive size are valid */
//														/* Close file, don't forget this! */
//														f_close(&fil);
//													}
//											}
//										}
//									}
//								else f_close(&fil);//file exists, was openned and must be closed
					
					
					temp_sd_res = f_open(&fil, "0:Tempr.txt", FA_OPEN_EXISTING | FA_READ | FA_WRITE);
					if (temp_sd_res != FR_OK) 
						{
							if (f_open(&fil, "0:Tempr.txt", FA_CREATE_NEW | FA_READ | FA_WRITE) == FR_OK)
								{//write redline
									sprintf(buffer, "Data\t\tTime\t\tVoltage\tTempr\tPresure\n");
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
						else f_close(&fil);//file exists, was openned and must be closed
							
							/* Unmount drive, don't forget this! */
							f_mount(0, "0:", 1);
					
				}//end mount SD
			


		xMutex_LCD = xSemaphoreCreateMutex();
		xWatt_1_sec_measure = xSemaphoreCreateBinary();
		xSDcard_write =  xSemaphoreCreateBinary();
		xSDcard_written_done = xSemaphoreCreateBinary();
		
		if ((xMutex_LCD == NULL) || (xWatt_1_sec_measure == NULL) || (xSDcard_write == NULL))	
			while(1); //Error creation Semaphore
		
		if (xSDcard_written_done == NULL)
			while(1);
	
//create thread for taken touch sensor data. it will be susspend after all
//	osThreadDef(TouchThread, TouchThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
//  xTouchThread = osThreadCreate (osThread(TouchThread), NULL);
		
		
//	osThreadDef(SensorsThread, SensorsThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
//  osThreadCreate (osThread(SensorsThread), NULL);
	
//	osThreadDef(SD_Thread, SDCardThread, osPriorityAboveNormal, 0, configMINIMAL_STACK_SIZE);
//  osThreadCreate (osThread(SD_Thread), NULL);	

  /* Create Start thread */
  osThreadDef(USER_Thread, StartThread, osPriorityNormal, 0, 1024);//configMINIMAL_STACK_SIZE);
  osThreadCreate (osThread(USER_Thread), NULL);
	
  /* Start scheduler */
  osKernelStart(NULL, NULL);

  /* We should never get here as control is now taken by the scheduler */


  while (1)
  {
  }
		

}


static void TouchThread(void const * argument)
{

		for(;;)
			{
			//waiting for press
			while (TM_STMPE811_ReadTouch(&Coord_811) != TM_STMPE811_State_Pressed)
				osDelay(5); 
					
			/* Draw pixel on touch location */
			TM_ILI9341_DrawCircle(Coord_811.y,(ILI9341_WIDTH - Coord_811.x), 2, ILI9341_COLOR_GREEN);

				
			/* Touch valid */
			sprintf(buffer, "X: %03d  Y: %03d", Coord_811.x, Coord_811.y);
			TM_ILI9341_Puts(20, 80, buffer, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_ORANGE);
				
			// waiting for released
			while (TM_STMPE811_ReadTouch(&Coord_811) != TM_STMPE811_State_Released)
				osDelay(50);
			}
		
		vTaskDelete( NULL );
}


static void StartThread(void const * argument)
{
//Thread for measure data and write it to SD
	portTickType xLastWakeTime;
	uint16_t  ADC_Value, Max_ADC, ADC_Vbat;
	uint8_t tempr[2], i;
	TM_BMP180_Oversampling_t BMP180_Oversampling;
	double Voltage_ADC, Current_ADC; 
	double Voltage_Battery;
	
	xLastWakeTime = xTaskGetTickCount();
	Max_ADC = 0;
	Voltage_ADC = Current_ADC = 0.0;
	
	osDelay(1900);

  for(;;)
  {
			
  TM_DISCO_LedOn(LED_GREEN);
			
	//Get time
  TM_RTC_GetDateTime(&datatime, TM_RTC_Format_BIN);
	
	//Read ADC1 channel 13
	ADC_Value = TM_ADC_Read(ADC1, ADC_Channel_13), TM_ADC_Read(ADC1, ADC_Channel_13), TM_ADC_Read(ADC1, ADC_Channel_13);
		
	ADC_Vbat = TM_ADC_ReadVbat(ADC1), TM_ADC_ReadVbat(ADC1), TM_ADC_ReadVbat(ADC1), TM_ADC_ReadVbat(ADC1);
	Voltage_Battery = (double) TM_ADC_ReadVbat(ADC1) * 4 * 3300. / 4096.;// value in mV. for stm32f429 V = Vbat/4
	
	
//	Voltage_ADC = (float)ADC_Value * 0.805664f * 1.8f;
	Voltage_ADC = (double)ADC_Value * 1.45f;
	Current_ADC = Voltage_ADC / 30.0f;
	
	TM_I2C_ReadMulti(STMPE811_I2C, LM75_ADDRESS, 0x00, tempr, 2); // Read temperature from LM75
	real_tempr = (double)tempr[0] + 0.125*(tempr[1]>>5);
		if (tempr[0] == 0) 
			{	//we have error on bus I2C
				TM_ILI9341_Puts(60, 40, "Error init LM75", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_RED);			
				GPIOA->MODER &= ~(0x00030000);
				GPIOA->MODER |= 0x00010000;//set SCL like output
				for (i=9; i; i--) 
				{
					TM_GPIO_SetPinHigh(GPIOA, GPIO_PinSource8);
					osDelay(1);
					TM_GPIO_SetPinLow(GPIOA, GPIO_PinSource8);
					osDelay(1);
				}
				GPIOA->MODER |= 0x00030000;//set SCL like AF
				
			}

	/* Start temperature conversion */
		TM_BMP180_StartTemperature(&BMP180_Data);
		
		/* Wait delay in microseconds */
		/* You can do other things here instead of delay */
		osDelay(5);
		
		/* Read temperature first */
		TM_BMP180_ReadTemperature(&BMP180_Data);

		BMP180_Oversampling = TM_BMP180_Oversampling_HighResolution;
		
		/* Start pressure conversion at ultra high resolution */
		TM_BMP180_StartPressure(&BMP180_Data, BMP180_Oversampling);
		
		/* Wait delay in microseconds */
		/* You can do other things here instead of delay */
		if (BMP180_Oversampling == TM_BMP180_Oversampling_UltraLowPower)
			osDelay(5);
		else if (BMP180_Oversampling == TM_BMP180_Oversampling_Standard)
			osDelay(8);
		else if (BMP180_Oversampling == TM_BMP180_Oversampling_HighResolution)
			osDelay(14);
		else	osDelay(26);
		
		/* Read pressure value */
		TM_BMP180_ReadPressure(&BMP180_Data);
	
		avarage_preshure = (avarage_preshure + BMP180_Data.Pressure)>>1;
		avarage_temperature = (avarage_temperature + real_tempr)/2.0f;
		
		if (((datatime.minutes % 10) == 0) && (datatime.seconds == 0))
		{
			Temperature[Count_Array_Tempr] = avarage_temperature;
			Presure[Count_Array_Tempr] = avarage_preshure;
			Time_div_10[Count_Array_Tempr] = datatime;
			Count_Array_Tempr++;
		}
		
		Time[Count_Array_Watt] = datatime;
		Watt[Count_Array_Watt] = ADC_Value;
		
			
		Max_ADC = (Max_ADC < ADC_Value) ? ADC_Value : Max_ADC;

			
	if( xMutex_LCD != NULL )
		{
			// See if we can obtain the semaphore.  If the semaphore is not available
			// wait 10 ticks to see if it becomes free.	
			if (xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE)
			{
					// We were able to obtain the semaphore and can now access the shared resource.
					
					sprintf(buffer, "%02d.%02d.%04d \t%02d:%02d:%02d",datatime.date, datatime.month, datatime.year + 2000, datatime.hours, datatime.minutes, datatime.seconds);
					TM_ILI9341_Puts(10, 120, buffer, &TM_Font_11x18, 0x0000, BackGround);
				  
					sprintf(buffer, "i = %u ADC = %u Max = %u",  Count_Array_Watt, ADC_Value, Max_ADC);
					TM_ILI9341_Puts(10, 140, buffer, &TM_Font_11x18, 0x0000, BackGround);
				
					sprintf(buffer, "T = %.2f  B = %u", real_tempr, BMP180_Data.Pressure);
					TM_ILI9341_Puts(10, 160, buffer, &TM_Font_11x18, 0x0000, BackGround);
				
					sprintf(buffer, "V = %.2f mV I = %.3f mA", Voltage_ADC, Current_ADC);
					TM_ILI9341_Puts(10, 180, buffer, &TM_Font_11x18, 0x0000, BackGround);
				
					sprintf(buffer, "Vbat = %.2f mV, ADC_bat = %d", Voltage_Battery, ADC_Vbat);
					TM_ILI9341_Puts(10, 200, buffer, &TM_Font_11x18, 0x0000, BackGround);
				  // We have finished accessing the shared resource.  Release the semaphore.
					xSemaphoreGive( xMutex_LCD );
			}
		}

	Count_Array_Watt++;

		
		TM_DISCO_LedOff(LED_GREEN);
		
//	if (Count_Array_Watt > MAX_COUNT_ARRAY_WATT)
//		{
//			Write_Data_to_SD (Count_Array_Watt - 1);
//			TM_DISCO_LedOn(LED_RED);
//			Count_Array_Watt = 0;
//			Max_ADC = 0;
//		}
		
	if (Count_Array_Watt > MAX_COUNT_ARRAY_WATT)
		Max_ADC = (TM_DISCO_LedOn(LED_RED), Write_Data_to_SD (Count_Array_Watt - 1), Count_Array_Watt = 0,  0);
			
	
	if (Count_Array_Tempr > MAX_COUNT_ARRAY_WATT)
		{
			TM_DISCO_LedOn(LED_RED);
			Write_Tempr_to_SD (Count_Array_Tempr - 1);
			Count_Array_Tempr = 0;
			
		}
		
	if (TM_DISCO_ButtonPressed())
		{
			TM_DISCO_LedOn(LED_RED);
			Write_Data_to_SD (Count_Array_Watt - 1);
			
			sprintf(buffer, "m = %u o = %u w = %u",  time_for_mount, time_for_open, time_for_write);
			TM_ILI9341_Puts(10, 80, buffer, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_RED);
			
			Write_Tempr_to_SD (Count_Array_Tempr - 1);
			
			sprintf(buffer, "Count_Array_Watt = %u Count_Array_Tempr = %u",  Count_Array_Watt, Count_Array_Tempr);
			TM_ILI9341_Puts(10, 60, buffer, &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_RED);
			
			Count_Array_Watt = 0;
			Count_Array_Tempr = 0;
		}
	
		
		osDelay(TIME_FOR_GET_MEASURE);
		//		osDelayUntil(xLastWakeTime, 1000);
				
			if( xMutex_LCD != NULL )
		{
			// See if we can obtain the semaphore.  If the semaphore is not available
			// wait 10 ticks to see if it becomes free.	
			if (xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE)
			{
					// We were able to obtain the semaphore and can now access the shared resource.
					
					sprintf(buffer, "                           ");
					TM_ILI9341_Puts(10, 200, buffer, &TM_Font_11x18, 0x0000, BackGround);
					TM_ILI9341_Puts(10, 140, buffer, &TM_Font_11x18, 0x0000, BackGround);
					TM_ILI9341_Puts(10, 160, buffer, &TM_Font_11x18, 0x0000, BackGround);
					TM_ILI9341_Puts(10, 180, buffer, &TM_Font_11x18, 0x0000, BackGround);
				  TM_ILI9341_Puts(60, 40, buffer, &TM_Font_11x18, BackGround, BackGround);//error massege LM75
					
				  // We have finished accessing the shared resource.  Release the semaphore.
					xSemaphoreGive( xMutex_LCD );
			}
		}


  }

	vTaskDelete( NULL );
}


static void SDCardThread(void const * argument)
{
	//thread for work with CD_Card by SPI4

//	uint16_t time=0;
//	uint32_t i;

	if (f_mount(&FatFs, "0:", 1) == FR_OK) {
						
		temp_sd_res = f_open(&fil, "0:Tempr.txt", FA_OPEN_EXISTING | FA_READ | FA_WRITE);
		if (temp_sd_res != FR_OK) 
			{
				if (f_open(&fil, "0:Tempr.txt", FA_CREATE_NEW | FA_READ | FA_WRITE) == FR_OK)
					{//write redline
						sprintf(buffer, "Data\tTime\t\tVol\tTempr\tPresure\n");
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
//				//file existing and open
//								sprintf(buffer, "------------------------\n");
//								if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
//									
//									/* If we put more than 0 characters (everything OK) */
//									if (f_puts(buffer, &fil) > 0) 
//										{
//											if (TM_FATFS_DriveSize(&total, &free) == FR_OK) {
//												/* Data for drive size are valid */
//												/* Close file, don't forget this! */
//												
//											}
//										}
//					f_close(&fil);
//				}
			
//				sprintf(buffer, "0:Hello.txt");	
//			if (f_open(&fil, buffer, FA_CREATE_ALWAYS | FA_READ | FA_WRITE) == FR_OK)
//				{
//					f_close(&fil);
				}
				
				/* Unmount drive, don't forget this! */
				f_mount(0, "0:", 1);
				
			}//end mount SD
	
  for(;;)
  {
		xSemaphoreTake(xSDcard_write, portMAX_DELAY);
		
		Write_Data_to_SD (Count_for_SD_Write);

		TM_DISCO_LedOff(LED_RED);
		
		xSemaphoreGive(xSDcard_written_done);
}
	
		vTaskDelete( NULL );
}



static void SensorsThread(void const * argument)
{
	uint8_t tempr[2];
	uint16_t  ADC_Value;
	TM_BMP180_Oversampling_t BMP180_Oversampling;
	
	osDelay(1900);
 
  /* Infinite loop */
  for(;;)
  {
		
		/* Start temperature conversion */
        TM_BMP180_StartTemperature(&BMP180_Data);
        
        /* Wait delay in microseconds */
        /* You can do other things here instead of delay */
        osDelay(5);
        
        /* Read temperature first */
        TM_BMP180_ReadTemperature(&BMP180_Data);
		
				BMP180_Oversampling = TM_BMP180_Oversampling_HighResolution;
        
        /* Start pressure conversion at ultra high resolution */
        TM_BMP180_StartPressure(&BMP180_Data, BMP180_Oversampling);
        
        /* Wait delay in microseconds */
        /* You can do other things here instead of delay */
        if (BMP180_Oversampling == TM_BMP180_Oversampling_UltraLowPower)
					osDelay(5);
				else if (BMP180_Oversampling == TM_BMP180_Oversampling_Standard)
					osDelay(8);
				else if (BMP180_Oversampling == TM_BMP180_Oversampling_HighResolution)
					osDelay(14);
				else	osDelay(26);
        
        /* Read pressure value */
        TM_BMP180_ReadPressure(&BMP180_Data);
        
				
		if( xMutex_LCD != NULL )
			{
        // See if we can obtain the semaphore.  If the semaphore is not available
        // wait 10 ticks to see if it becomes free.	
        if( xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE )
        {
            // We were able to obtain the semaphore and can now access the shared resource.
						sprintf(buffer, "T=%2.3f Pr=%6d A=%3.2f", BMP180_Data.Temperature, BMP180_Data.Pressure, BMP180_Data.Altitude);
						TM_ILI9341_Puts(10, 40, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);
					
            // We have finished accessing the shared resource.  Release the semaphore.
            xSemaphoreGive( xMutex_LCD );
        }
        else
        {
            // We could not obtain the semaphore and can therefore not access
            // the shared resource safely.
        }
			}
			
	//Get time
  TM_RTC_GetDateTime(&datatime, TM_RTC_Format_BIN);
	
	//Read ADC1 channel 13
	ADC_Value = TM_ADC_Read(ADC1, ADC_Channel_13);
    
    
    
			
		if( xMutex_LCD != NULL )
		{
			// See if we can obtain the semaphore.  If the semaphore is not available
			// wait 10 ticks to see if it becomes free.	
			if( xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE )
			{
					// We were able to obtain the semaphore and can now access the shared resource.
					//Format time
					sprintf(buffer, "%02d.%02d.%04d %02d:%02d:%02d  ADC: %u\n", datatime.date, datatime.month, datatime.year + 2000, datatime.hours, datatime.minutes, datatime.seconds, ADC_Value);
					TM_ILI9341_Puts(10, 180, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);
				
					// We have finished accessing the shared resource.  Release the semaphore.
					xSemaphoreGive( xMutex_LCD );
			}
		}
	
		TM_I2C_ReadMulti(STMPE811_I2C, LM75_ADDRESS, 0x00, tempr, 2); // Read temperature from LM75
		real_tempr = (float)tempr[0] + 0.125*(tempr[1]>>5);
			
		if( xMutex_LCD != NULL )
			{
        // See if we can obtain the semaphore.  If the semaphore is not available
        // wait 10 ticks to see if it becomes free.	
        if( xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE )
        {
            // We were able to obtain the semaphore and can now access the shared resource.
						sprintf(buffer, "T_LM75 = %.3f", real_tempr);
						TM_ILI9341_Puts(10, 60, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_GREEN);
					
            // We have finished accessing the shared resource.  Release the semaphore.
            xSemaphoreGive( xMutex_LCD );
        }
        else
        {
            // We could not obtain the semaphore and can therefore not access
            // the shared resource safely.
        }
			}			
			

		
/* Read acceleration data */
      TM_L3GD20_Read(&L3GD20_Data);
			temp_f = sqrt(L3GD20_Data.X*L3GD20_Data.X + L3GD20_Data.Y*L3GD20_Data.Y + L3GD20_Data.Z*L3GD20_Data.Z);
			if (maximum_rotation < temp_f)
			maximum_rotation = temp_f;
	
		if( xMutex_LCD != NULL )
			{
        // See if we can obtain the semaphore.  If the semaphore is not available
        // wait 10 ticks to see if it becomes free.	
        if( xSemaphoreTake( xMutex_LCD, ( portTickType ) 50 ) == pdTRUE )
        {
            // We were able to obtain the semaphore and can now access the shared resource.
							sprintf(buffer, "X = %4d, Y = %4d, Z = %4d", L3GD20_Data.X, L3GD20_Data.Y, L3GD20_Data.Z);
							TM_ILI9341_Puts(10, 80, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_RED);

							sprintf(buffer, "M = %.3f, MAX = %.3f", temp_f, maximum_rotation);
							TM_ILI9341_Puts(10, 100, buffer, &TM_Font_11x18, 0x0000, ILI9341_COLOR_BLUE);
					
            // We have finished accessing the shared resource.  Release the semaphore.
            xSemaphoreGive( xMutex_LCD );
        }
        else
        {
            // We could not obtain the semaphore and can therefore not access
            // the shared resource safely.
        }
			}		

			if (TM_DISCO_ButtonPressed()) maximum_rotation = 0;
						
		osDelay(300);
	}
	
		vTaskDelete( NULL );
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
	
	TIM_Cmd(TIM5, ENABLE);	
}


void Write_Data_to_SD (uint16_t Count)
{
	uint16_t i;
	
		TIM5->ARR = 0x0F4240;
	
			if (f_mount(&FatFs, "0:", 1) == FR_OK) {
				
				time_for_mount = 0x0F4240 - TIM5->ARR;
				TIM5->ARR = 0x0F4240;
				
				/* Try to open file */
				if (f_open(&fil, file_name_data, FA_OPEN_ALWAYS | FA_READ | FA_WRITE) == FR_OK) 
					{
					time_for_open = 0x0F4240 - TIM5->ARR;
					TIM5->ARR = 0x0F4240;
					if(f_lseek(&fil, f_size(&fil)) == FR_OK) //move to the end of file
						{
						for (i=0; i<=Count; i++)
							{
								// We were able to obtain the semaphore and can now access the shared resource.
								datatime = Time[i];
								sprintf(buffer, "%02d.%02d.%04d\t%02d:%02d:%02d\t%.2f\t%u\n", datatime.date, datatime.month, datatime.year + 2000, datatime.hours, datatime.minutes, datatime.seconds, Temperature[i], Presure[i]);
								
								
								/* If we put more than 0 characters (everything OK) */
								if (f_puts(buffer, &fil) > 0) {
									if (TM_FATFS_DriveSize(&total, &free) == FR_OK) {
										/* Data for drive size are valid */
										
									}
								}
							}
						};
					time_for_write = 0x0F4240 - TIM5->ARR;
					TIM5->ARR = 0x0F4240;
									
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
	
			TIM5->ARR = 0x0F4240;
			if (f_mount(&FatFs, "0:", 1) == FR_OK) 
				{
					
				time_for_mount = 0x0F4240 - TIM5->ARR;
				TIM5->ARR = 0x0F4240;
					
				/* Try to open file */
				if (f_open(&fil, file_name_tempr, FA_OPEN_ALWAYS | FA_READ | FA_WRITE) == FR_OK) 
					{
					time_for_open = 0x0F4240 - TIM5->ARR;
					TIM5->ARR = 0x0F4240;
						
					if(f_lseek(&fil, f_size(&fil)) == FR_OK){//move to the end of file
					for (i=0; i<=Count; i++)
						{
							// We were able to obtain the semaphore and can now access the shared resource.
							datatime = Time[i];
							sprintf(buffer, "%02d.%02d.%04d\t%02d:%02d:%02d\t%u\n", datatime.date, datatime.month, datatime.year + 2000, datatime.hours, datatime.minutes, datatime.seconds, Watt[i]);
							
							
							/* If we put more than 0 characters (everything OK) */
							if (f_puts(buffer, &fil) > 0) {
								if (TM_FATFS_DriveSize(&total, &free) == FR_OK) {
									/* Data for drive size are valid */
									
								}
							}
						}
					
					time_for_write = 0x0F4240 - TIM5->ARR;
					TIM5->ARR = 0x0F4240;

					};
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

