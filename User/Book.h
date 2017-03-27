#ifndef	__BOOK_H
#define __BOOK_H

#include "stm32f4xx.h"
#include "tm_stm32f4_ili9341_ltdc.h"
#include "ff.h"
#include "cmsis_os.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_TRY_TO_MOUNT 10

typedef struct 
{
	uint16_t max_char;
	uint16_t height_of_char;
	uint16_t width_of_char;
	uint16_t char_per_line;
	uint16_t row_on_screen;
	FSIZE_t position;
	
}Book_struct;

void Set_parameters_book(void);
void Read_page(char *buffer);
void parser(void);

#endif
