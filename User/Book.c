#include "Book.h"


Book_struct book_dat;

char page[320 * 240];

void parser(void)
{
	uint16_t i;
	for (i = 0; i < book_dat.max_char ; i++)
	{
		if (page[i] = buffer[i] == '\n')
		{
			
		}
		else if (page[i] == )
		{
			
		}
	}
}

void Read_page(char *buffer)
{
	FATFS *fs, fs_local;
	FRESULT res;
	FIL fil, dat;
	uint8_t i;
	UINT count;
	FSIZE_t f_size;
	char pos_buff[20];
	
	fs = &fs_local;
	
	i = 0;
	while ((f_mount(fs, "0:", 1) != FR_OK) && (i != 10))
	{
		osDelay(200);
		i++;
	}
	if (i == 10)
	{
		TM_ILI9341_Puts(20, 80, "mount not OK", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_ORANGE);
		return;
	}
			
				res = f_open(&fil, "Readme.txt", FA_READ);
				if (res == FR_OK)
				{
					f_lseek(&fil, book_dat.position);
					f_read(&fil, buffer, book_dat.max_char, &count);
					
					book_dat.position = f_tell(&fil);
					res = f_open(&dat, "Readme.dat", FA_WRITE);
					if (res == FR_OK)
					{
						sprintf(pos_buff, "%ld\n", book_dat.position);
						f_puts(pos_buff, &dat);
						f_close(&dat);
					}
					f_close(&fil);
				}
			
			/* Unmount drive, don't forget this! */
			f_mount(0, "0:", 1);
		
	
}

void Set_parameters_book(void)
{
	FATFS *fs, fs_local;
	FRESULT res;
	FIL fil;
	uint8_t i;
	char buff[20], *p_char;

	fs = &fs_local;
	p_char = NULL;
	
	book_dat.max_char = (ILI9341_HEIGHT / TM_Font_11x18.FontWidth) * (ILI9341_WIDTH / TM_Font_11x18.FontHeight);
	book_dat.height_of_char = TM_Font_11x18.FontHeight;
	book_dat.width_of_char = TM_Font_11x18.FontWidth;
	
	book_dat.char_per_line = ILI9341_HEIGHT / TM_Font_11x18.FontWidth;
	book_dat.row_on_screen = (ILI9341_WIDTH / TM_Font_11x18.FontHeight);
	
	i = 0;
	res = f_mount(fs, "0:", 1);
	while ((res != FR_OK) && (i != MAX_TRY_TO_MOUNT))
	{
		osDelay(500);
		res = f_mount(fs, "0:", 1);
		i++;
	}
	
	if (i == MAX_TRY_TO_MOUNT)
	{
		return;
	}
	
	// fatfs mounted
		res = f_open(&fil, "Readme.dat", FA_READ);
			if (res == FR_NO_FILE)
			{
				res = f_open(&fil, "Readme.dat", FA_OPEN_ALWAYS | FA_WRITE);
				if (res == FR_OK)
				{
					f_printf(&fil, "0\n");
					book_dat.position = 0;
				
					f_close(&fil);
					/* Unmount drive, don't forget this! */
					f_mount(0, "0:", 1);
						return;
					
				}
			}
			else if (res ==FR_OK)
			{
				
				p_char = f_gets(buff, 20, &fil);
				if(p_char)
				{
					book_dat.position = atoi(p_char);
				}
				f_close(&fil);
			}
	
			/* Unmount drive, don't forget this! */
			f_mount(0, "0:", 1);		
	return;
}

