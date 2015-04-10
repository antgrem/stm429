// !!! SPI READ_function writen by spi3

#include <MPL115A1.h>

extern __IO uint8_t UserButtonPressed;

#define mpl115a1_rx() mpl115a1_rxtx(0xff)
#define mpl115a1_tx(data) mpl115a1_rxtx(data)

#define mpl115a1_select() GPIO_ResetBits(MPL115A1_SPI_CS_GPIO_PORT, MPL115A1_SPI_CS_PIN)
#define mpl115a1_deselect() GPIO_SetBits(MPL115A1_SPI_CS_GPIO_PORT, MPL115A1_SPI_CS_PIN)


void MPL115_Init(void)//(MPL115A1_InitTypeDef *MPL115A1_InitStruct)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef  SPI_InitStructure;

  /* Enable the SPI periph */
  RCC_APB1PeriphClockCmd(MPL115A1_SPI_CLK, ENABLE);

  /* Enable SCK, MOSI and MISO GPIO clocks */
  RCC_AHB1PeriphClockCmd(MPL115A1_SPI_SCK_GPIO_CLK | MPL115A1_SPI_MISO_GPIO_CLK | MPL115A1_SPI_MOSI_GPIO_CLK, ENABLE);

  /* Enable CS  GPIO clock */
  RCC_AHB1PeriphClockCmd(MPL115A1_SPI_CS_GPIO_CLK, ENABLE);
  

  GPIO_PinAFConfig(MPL115A1_SPI_SCK_GPIO_PORT, MPL115A1_SPI_SCK_SOURCE, MPL115A1_SPI_SCK_AF);
  GPIO_PinAFConfig(MPL115A1_SPI_MISO_GPIO_PORT, MPL115A1_SPI_MISO_SOURCE, MPL115A1_SPI_MISO_AF);
  GPIO_PinAFConfig(MPL115A1_SPI_MOSI_GPIO_PORT, MPL115A1_SPI_MOSI_SOURCE, MPL115A1_SPI_MOSI_AF);
	

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

  /* SPI SCK pin configuration */
  GPIO_InitStructure.GPIO_Pin = MPL115A1_SPI_SCK_PIN;
  GPIO_Init(MPL115A1_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);

  /* SPI  MOSI pin configuration */
  GPIO_InitStructure.GPIO_Pin =  MPL115A1_SPI_MOSI_PIN;
  GPIO_Init(MPL115A1_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);

  /* SPI MISO pin configuration */
  GPIO_InitStructure.GPIO_Pin = MPL115A1_SPI_MISO_PIN;
  GPIO_Init(MPL115A1_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

  /* SPI configuration -------------------------------------------------------*/
  SPI_I2S_DeInit(MPL115A1_SPI);
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_Init(MPL115A1_SPI, &SPI_InitStructure);

  /* Enable SPI1  */
  SPI_Cmd(MPL115A1_SPI, ENABLE);

  /* Configure GPIO PIN for Lis Chip select */
  GPIO_InitStructure.GPIO_Pin = MPL115A1_SPI_CS_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(MPL115A1_SPI_CS_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_Init(MPL115A1_SPI_SHDN_GPIO_PORT, &GPIO_InitStructure);

  /* Deselect : Chip Select high */
  GPIO_SetBits(MPL115A1_SPI_CS_GPIO_PORT, MPL115A1_SPI_CS_PIN);
	
	/* turn on MPL115 */
	//GPIO_ResetBits(MPL115A1_SPI_SHDN_GPIO_PORT, MPL115A1_SPI_SHDN_PIN);
	GPIO_SetBits(MPL115A1_SPI_SHDN_GPIO_PORT, MPL115A1_SPI_SHDN_PIN);
  

}

void MPL115_Read_coef(MPL115A1_CoefTypeDef *Coef)
{	uint8_t temp_m, temp_l;
	
  mpl115a1_select();
  
  mpl115a1_tx(A0_MSB);
  temp_m = mpl115a1_rx();
  
  mpl115a1_tx(A0_LSB);
  temp_l = mpl115a1_rx();
  
  Coef->a0 = (temp_m<<8) | (temp_l);

  mpl115a1_tx(B1_MSB);
  temp_m = mpl115a1_rx();
  
  mpl115a1_tx(B1_LSB);
  temp_l = mpl115a1_rx();
  
  Coef->b1 = (temp_m<<8) | (temp_l);
  
  mpl115a1_tx(B2_MSB);
  temp_m = mpl115a1_rx();
  
  mpl115a1_tx(B2_LSB);
  temp_l = mpl115a1_rx();
  
  Coef->b2 = (temp_m<<8) | (temp_l);

  mpl115a1_tx(C12_MSB);
  temp_m = mpl115a1_rx();
  
  mpl115a1_tx(C12_LSB);
  temp_l = mpl115a1_rx();
  
  Coef->c12 = (temp_m<<8) | (temp_l);
  
  mpl115a1_deselect();
}

void MPL115_Start_Conversion (void)
{
  mpl115a1_select();
  
  mpl115a1_tx(CONVERT);
  mpl115a1_rx();
  
  mpl115a1_deselect();
}

void MPL115_Read_Data(MPL115A1_PressureTypeDef *Pressure)
{ uint8_t temp_m, temp_l;
	
  mpl115a1_select();
  
  mpl115a1_tx(P_ADC_MSB);
  temp_m = mpl115a1_rx();
  
  mpl115a1_tx(P_ADC_LSB);
  temp_l = mpl115a1_rx();
  
  Pressure->Padc = (temp_m<<8) | (temp_l);

  mpl115a1_tx(T_ADC_MSB);
  temp_m = mpl115a1_rx();
  
  mpl115a1_tx(T_ADC_LSB);
  temp_l = mpl115a1_rx();
  
  Pressure->Tadc = (temp_m<<8) | (temp_l);
  
  mpl115a1_deselect();
}



float MPL115A1_calc_pressure (MPL115A1_CoefTypeDef*  Coef, MPL115A1_PressureTypeDef* Data)
{
float r1,r2,Pressure, Temp;

    r1 = (float)Coef->c12/(16777216)*(float)Data->Tadc/64;
    r1 = (float)Coef->b1/8192+r1;
    r1 =  r1*(float)Data->Padc/64;
    r1 = (float)Coef->a0/8+r1;
    r2 = (float)Coef->b2/16384*(float)Data->Tadc/64;

    Temp = r1+r2;
    Pressure = (float)Temp * (float)0.06354+50;
		Pressure *= (float)7.5;

return Pressure;
}


uint8_t mpl115a1_rxtx(uint8_t data)
{
	// send data
    MPL115A1_SPI->DR = data;
    
    // wait while send
    while(!(MPL115A1_SPI->SR & SPI_SR_RXNE));
    
    // recieve data
    return MPL115A1_SPI->DR;
}





//-----------------------
