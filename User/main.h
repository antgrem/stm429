#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f4xx.h"
/* Include my libraries here */
#include "defines.h"
#include "tm_stm32f4_disco.h"
#include "tm_stm32f4_delay.h"
#include "tm_stm32f4_rtc.h"
#include "tm_stm32f4_adc.h"
#include "tm_stm32f4_bmp180.h"
#include "tm_stm32f4_ili9341_ltdc.h"
#include "tm_stm32f4_nrf24l01.h"
#include "tm_stm32f4_i2c.h"
#include "tm_stm32f4_stmpe811.h"
#include <stdio.h>
#include <math.h>
#include "tm_stm32f4_l3gd20.h"
#include "cmsis_os.h"
#include "semphr.h"
#include <MPL115A1.h>
#include "tm_stm32f4_fatfs.h"
#include "tm_stm32f4_exti.h"
#include "ff.h"
#include "sensors.h"
#include "Book.h"


#define SCR_REDRAW 10
#define SCR_DRAW_XY 20

typedef struct
{
	uint8_t screen_pressed;
	uint16_t X;
	uint16_t Y;
	uint8_t Buttom_pressed;
	uint16_t time_buttom;
} Book_control_struct;	

#endif
