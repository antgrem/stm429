#ifndef __SENSORS_H
#define __SENSORS_H

#include "main.h"
#include "tm_stm32f4_bmp180.h"
#include "stm32f4xx.h"

TM_BMP180_Result_t Measure_BMP180(TM_BMP180_t* BMP180_Data, TM_BMP180_Oversampling_t Oversampling);

#endif
