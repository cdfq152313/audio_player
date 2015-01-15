#include "stm32_init.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "misc.h"

void RCC_Configuration(void)
{
    ErrorStatus HSEStartUpStatus;
    RCC_DeInit();

    RCC_HSEConfig(RCC_HSE_ON);
    HSEStartUpStatus = RCC_WaitForHSEStartUp();
    if(HSEStartUpStatus == SUCCESS)
    {
      RCC_HCLKConfig(RCC_SYSCLK_Div1);
      RCC_PCLK2Config(RCC_HCLK_Div1);
      RCC_PCLK1Config(RCC_HCLK_Div2);
      //RCC_PLLConfig(RCC_PLLSource_HSE, 8, 168, 2, 6);
      RCC_PLLConfig(RCC_PLLSource_HSE, 5, 441, 4, 6);
      RCC_PLLCmd(ENABLE);
      while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
      RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
      while(RCC_GetSYSCLKSource() != 0x08);
      
    }

}
