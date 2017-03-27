
#include "sensors.h"



TM_BMP180_Result_t Measure_BMP180(TM_BMP180_t* BMP180_Data, TM_BMP180_Oversampling_t Oversampling)
{
	TM_BMP180_Result_t Result;
		/* Start temperature conversion */
        TM_BMP180_StartTemperature(BMP180_Data);
        
        /* Wait delay in microseconds */
        /* You can do other things here instead of delay */
        osDelay(5);
        
        /* Read temperature first */
        TM_BMP180_ReadTemperature(BMP180_Data);
		    
        /* Start pressure conversion at ultra high resolution */
        TM_BMP180_StartPressure(BMP180_Data, Oversampling);
        
        /* Wait delay in microseconds */
        /* You can do other things here instead of delay */
        if (Oversampling == TM_BMP180_Oversampling_UltraLowPower)
					osDelay(5);
				else if (Oversampling == TM_BMP180_Oversampling_Standard)
					osDelay(8);
				else if (Oversampling == TM_BMP180_Oversampling_HighResolution)
					osDelay(14);
				else	osDelay(26);
        
        /* Read pressure value */
        Result = TM_BMP180_ReadPressure(BMP180_Data);
        
				return Result;
}

//void Create_new_files(void)
//{
//	
//	// 
//			if (f_mount(&FatFs, "0:", 1) == FR_OK) 
//				{
//							//Get time
//						TM_RTC_GetDateTime(&datatime, TM_RTC_Format_BIN);
//				//	sprintf(buffer, "0:F%02d_%02d_%04d.txt", datatime.date, datatime.month, datatime.year);
//						sprintf(file_name_data, "0:%04d_%02d_%02d_Watt.txt", datatime.year+2000, datatime.month, datatime.date);
//					temp_sd_res = f_open(&fil, (TCHAR*) file_name_data, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
//							if (temp_sd_res != FR_OK) 
//								{
//									if (f_open(&fil, file_name_data, FA_CREATE_NEW | FA_READ | FA_WRITE) == FR_OK)
//										{//write redline
//											sprintf(buffer, "Data\t\tTime\t\tVoltage\tTempr\tPresure\n");
//											if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
//												
//												/* If we put more than 0 characters (everything OK) */
//												if (f_puts(buffer, &fil) > 0) {
//													
//														f_close(&fil);
//													
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
//											sprintf(buffer, "Data\t\tTime\t\tVoltage\tTempr\tPresure\n");
//											if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
//												
//												/* If we put more than 0 characters (everything OK) */
//												if (f_puts(buffer, &fil) > 0) {
//													
//														f_close(&fil);
//													
//											}
//										}
//									}
//								else f_close(&fil);//file exists, was openned and must be closed
//					
//					
//					temp_sd_res = f_open(&fil, "0:Tempr.txt", FA_OPEN_EXISTING | FA_READ | FA_WRITE);
//					if (temp_sd_res != FR_OK) 
//						{
//							if (f_open(&fil, "0:Tempr.txt", FA_CREATE_NEW | FA_READ | FA_WRITE) == FR_OK)
//								{//write redline
//									sprintf(buffer, "Data\t\tTime\t\tVoltage\tTempr\tPresure\n");
//									if(f_lseek(&fil, f_size(&fil)) == FR_OK){};
//										
//										/* If we put more than 0 characters (everything OK) */
//										if (f_puts(buffer, &fil) > 0) {
//											
//												f_close(&fil);
//											
//									}
//								}
//							}
//						else f_close(&fil);//file exists, was openned and must be closed
//							
//							/* Unmount drive, don't forget this! */
//							f_mount(0, "0:", 1);
//					
//				}//end mount SD
//			

//}
