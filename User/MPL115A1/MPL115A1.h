#ifndef __MPL115A1_H
#define __MPL115A1_H

#include <stm32f4xx.h>
#include <main.h>

//extern uint8_t UserButtonPressed;

/* MPL115A1 struct */
typedef struct
{
  uint8_t Power_Mode;                         /* Power-down/Active Mode */
  uint8_t Output_DataRate;                    /* OUT data rate 100 Hz / 400 Hz */
  uint8_t Axes_Enable;                        /* Axes enable */
  uint8_t Full_Scale;                         /* Full scale */
  uint8_t Self_Test;                          /* Self test */
}MPL115A1_InitTypeDef;

typedef struct
{
  int16_t a0;
  int16_t b1;
  int16_t b2;
  int16_t c12;
}MPL115A1_CoefTypeDef;

typedef struct
{
  uint16_t Padc;
  uint16_t Tadc;
}MPL115A1_PressureTypeDef;

//adress of register. they are already shifted left by 1
#define P_ADC_MSB   0x80
#define P_ADC_LSB   0x82
#define T_ADC_MSB   0x84
#define T_ADC_LSB   0x86
#define A0_MSB 		0x88	//0x08
#define A0_LSB 		0x8A
#define B1_MSB 		0x8C
#define B1_LSB 		0x8E
#define B2_MSB 		0x90
#define B2_LSB 		0x92
#define C12_MSB 	0x94
#define C12_LSB 	0x96
#define CONVERT 	0x24


/**
  * @brief  MPL115A1 SPI Interface pins
  */
#define MPL115A1_SPI                       SPI3
#define MPL115A1_SPI_CLK                   RCC_APB1Periph_SPI3

#define MPL115A1_SPI_SCK_PIN               GPIO_Pin_3                 /* PB.3 */
#define MPL115A1_SPI_SCK_GPIO_PORT         GPIOB                       /* GPIOB */
#define MPL115A1_SPI_SCK_GPIO_CLK          RCC_AHB1Periph_GPIOB				 /* SPI3 SCK */
#define MPL115A1_SPI_SCK_SOURCE            GPIO_PinSource3
#define MPL115A1_SPI_SCK_AF                GPIO_AF_SPI3

#define MPL115A1_SPI_MISO_PIN              GPIO_Pin_11                  /* PC.11 */
#define MPL115A1_SPI_MISO_GPIO_PORT        GPIOC                       /* GPIOC */
#define MPL115A1_SPI_MISO_GPIO_CLK         RCC_AHB1Periph_GPIOC        /* MISO */
#define MPL115A1_SPI_MISO_SOURCE           GPIO_PinSource11
#define MPL115A1_SPI_MISO_AF               GPIO_AF_SPI3

#define MPL115A1_SPI_MOSI_PIN              GPIO_Pin_12                  /* PC.12 */
#define MPL115A1_SPI_MOSI_GPIO_PORT        GPIOC                       /* GPIOC */
#define MPL115A1_SPI_MOSI_GPIO_CLK         RCC_AHB1Periph_GPIOC
#define MPL115A1_SPI_MOSI_SOURCE           GPIO_PinSource12
#define MPL115A1_SPI_MOSI_AF               GPIO_AF_SPI3

#define MPL115A1_SPI_CS_PIN                GPIO_Pin_7                  /* PC.07 */
#define MPL115A1_SPI_CS_GPIO_PORT          GPIOD                       /* GPIOD */
#define MPL115A1_SPI_CS_GPIO_CLK           RCC_AHB1Periph_GPIOD

#define MPL115A1_SPI_SHDN_PIN              GPIO_Pin_2                  /* PD.02 */
#define MPL115A1_SPI_SHDN_GPIO_PORT        GPIOD                       /* GPIOD */
#define MPL115A1_SPI_SHDN_GPIO_CLK         RCC_AHB1Periph_GPIOD


void MPL115_Init(void);//MPL115A1_InitTypeDef *MPL115A1_InitStruct);
void MPL115_Read_coef(MPL115A1_CoefTypeDef *Coef);
void MPL115_Start_Conversion(void);
void MPL115_Read_Data(MPL115A1_PressureTypeDef *Pressure);
void MPL115_GetPressure(void);
uint8_t mpl115a1_rxtx(uint8_t data);
float MPL115A1_calc_pressure (MPL115A1_CoefTypeDef*  Coef, MPL115A1_PressureTypeDef* Data);


#endif /* __MPL115A1_H */
