#include "pwm.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "misc.h"
#include "filesystem.h"
#include <math.h>

uint32_t play_time_other = 0;
uint32_t play_time_sec = 0;

short wave[42000];

void PWM_Start(void)
{
    int i;
    for(i=0;i<42000;i++)
        wave[i] = (sin(2.0 * 3.1415926535 * 400.0 * ((double)i / 42000.0) )+1) * 2000 / 2;

    // for(i=0;i<42000;i+= 1000){
    //     fio_printf(1, "%x\r\n", wave[i]);    
    // }


    PWM_RCC_Configuration();
    PWM_GPIO_Configuration();
    //PWM_TIM2_Configuration();
    PWM_TIM1_Configuration();

    while(1){}
}

void  PWM_RCC_Configuration(void)
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
        RCC_PLLConfig(RCC_PLLSource_HSE, 8, 168, 2, 6);
        RCC_PLLCmd(ENABLE);
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        while(RCC_GetSYSCLKSource() != 0x08);
        
    }

    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE , ENABLE );//Enalbe AHB for GPIOE
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_TIM1, ENABLE );//Enable APB for TIM1
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );//Enable APB for TIM2
}


void TIM1_UP_TIM10_IRQHandler(void)
{
    TIM_ClearITPendingBit(TIM1, TIM_FLAG_Update);
    //play_time_other = TIM_GetCounter(TIM2) % 42000;
    play_time_other = (play_time_other + 1) % 42000;
    if(play_time_other == 0) play_time_sec++;
    //TIM1->CCR1 = (sin(2.0 * 3.14159 * 400.0 * ((double)play_time_other / 42000.0) )+1) * 2000 / 2;
    TIM1->CCR1 = wave[play_time_other];
    // USART_SendData(USART1, wave[play_time_other]) ;
    //if(play_time_sec >= 10) TIM1->CCR1 = 0;
}
/*
void TIM2_IRQHandler(void)
{
    TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);

    //play_time_other = (play_time_other + 1) % 42000;
    //if(play_time_other == 0) play_time_sec++;
}*/

void  PWM_TIM2_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

  /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 1999;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;     
    TIM_OCInitStructure.TIM_Pulse = 999; 
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);

    TIM_ARRPreloadConfig(TIM2, ENABLE);

    TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);

 /* TIM2 enable counter */
    TIM_Cmd(TIM2, ENABLE);
}

void  PWM_TIM1_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    //TIM_DeInit(TIM1);

    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

  /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 1999;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

  /* PWM1 Mode configuration: Channel1 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1999;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;

    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    TIM_ClearITPendingBit(TIM1,TIM_IT_Update);
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

 /* TIM4 enable counter */
    TIM_Cmd(TIM1, ENABLE);
}

void PWM_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOE, GPIO_PinSource9, GPIO_AF_TIM1);
}