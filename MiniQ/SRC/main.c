#include <stdint.h>
#include "stm32f10x.h"
#include "driver/UARTs.h"
#include "driver/delay.h"
#include "driver/ioi2c.h"
#include "driver/mpu6050.h"
#include "driver/hmc5883l.h"
#include "driver/timer.h"
#include "driver/motor.h"
#include "algorithm/imu.h"
#include "algorithm/control.h"


GPIO_InitTypeDef GPIO_InitStructure;
ErrorStatus HSEStartUpStatus;

#define Calc_Speed  100   //100Hz
#define Calc_Time (1000000/Calc_Speed)  //us

char volatile status;
void GPIO_Configuration(void);
void NVIC_Configuration(void);
void WWDG_Configuration(void);

uint32_t system_microsec;

int16_t _hlt;

void out_int16_t(int16_t * data)
{
  char ctemp;
  if(data[0]<0)data[0]=32768-data[0];
	ctemp=data[0]>>8;
	UART1_Put_Char(ctemp);
	ctemp=data[0];
	UART1_Put_Char(ctemp);
}


int main(void)
{
  int16_t data[9];
  char cnt=0;
  int16_t result[3];
  char recv[256];


  SystemInit();
  delay_init(72);
  GPIO_Configuration();
  Initial_UART1(115200L);
  I2C_GPIO_Config();
  NVIC_Configuration();
  
  delay_ms(10);
  Init_MPU6050();
  
  delay_ms(10);
  HMC5883L_Init();
  delay_ms(10);
  
  IMU_init();
  
  Initial_TimerTick();
  system_microsec=micros();
  initMotor();
  initControl();
  status = 0xff;
  
  while(1)
  {
    if(micros()-system_microsec>Calc_Time)
    {
      
      Read_MPU6050_ACC(&data[0]);
      Read_MPU6050_GYRO(&data[3]);
      //HMC5883L_Read(&data[6]);
      
      IMU_getYawPitchRoll(result,data);
      controlLoop();
      UART1_Put_Char(0xff);
      UART1_Put_Char(0xaa);
      
      out_int16_t(&result[0]);
      out_int16_t(&result[1]);
      out_int16_t(&result[2]);
      
      
      if(cnt<=99)++cnt;
      else cnt=0;
      
      if(cnt <=2 ) GPIOB->BSRR = GPIO_Pin_1;
      else GPIOB->BRR  = GPIO_Pin_1;
      
      system_microsec = micros();
    }

    

  }
		     
}




void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE  );
   /* Configure USART1 Tx (PA.09) as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
    
  /* Configure USART1 Rx (PA.10) as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0  |
                                GPIO_Pin_1  |
								GPIO_Pin_2  |
								GPIO_Pin_3  |
								GPIO_Pin_4  |
								GPIO_Pin_5  |
								GPIO_Pin_6  |
								GPIO_Pin_7  |
								GPIO_Pin_8  |
								GPIO_Pin_9  |
								GPIO_Pin_10 |
								GPIO_Pin_11 |
								GPIO_Pin_12 ;			
  GPIO_InitStructure.GPIO_Mode =  GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);


}


void NVIC_Configuration(void)
{ 
  NVIC_InitTypeDef NVIC_InitStructure; 
          /* Enable the USART1 Interrupt */
        NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 7;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

}


void WWDG_Configuration(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);	
  WWDG_SetPrescaler(WWDG_Prescaler_8);	              //  WWDG clock counter = (PCLK1/4096)/8 = 244 Hz (~4 ms)  
  WWDG_SetWindowValue(0x41);		                 // Set Window value to 0x41
  WWDG_Enable(0x50);		       // Enable WWDG and set counter value to 0x7F, WWDG timeout = ~4 ms * 64 = 262 ms 
  WWDG_ClearFlag();			       // Clear EWI flag
  WWDG_EnableIT();			       // Enable EW interrupt
}


void WWDG_IRQHandler(void)
{
  /* Update WWDG counter */
  WWDG_SetCounter(0x50);
	
  /* Clear EWI flag */
  WWDG_ClearFlag();
  
}

