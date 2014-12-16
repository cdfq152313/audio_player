#include "pwm.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "misc.h"
#include "filesystem.h"
#include <math.h>
#include "ff.h"
uint32_t play_time_other = 0;
uint32_t play_time_sec = 0;

#define BUF_LENGTH 2000

int i;
short buf[BUF_LENGTH];
short buf2[BUF_LENGTH];
int br;
int chunck2_size = 0;
const int btr = BUF_LENGTH * sizeof(uint8_t);
uint8_t cur_buf = 0;
uint8_t update_buf = 0;
uint32_t cur_point = 0;
FIL fil;

uint8_t back[BUF_LENGTH];

//short wave[42000];

void PWM_Start(void)
{
    // for(i=0;i<BUF_LENGTH;i++)
    //      back[i] = (sin(2.0 * 3.1415926535 * 400.0 * ((double)i / 42000.0) )+1) * 2000 / 2;

    // for(i=0;i<42000;i+= 1000){
    //     fio_printf(1, "%x\r\n", wave[i]);    
    // }
    if (f_open(&fil, "music.wav", FA_OPEN_ALWAYS | FA_READ) != FR_OK) {
        fio_printf(1, "open file failed");
        return;
    }

    f_lseek(&fil, 40);
    f_read(&fil, &chunck2_size, 4, &br);
    f_lseek(&fil, f_tell(&fil)+84000);
    f_read(&fil, buf, btr, &br);
    f_lseek(&fil, f_tell(&fil)+btr);
    f_read(&fil, buf2, btr, &br);
    f_lseek(&fil, f_tell(&fil)+btr);

    int max=40;
    int16_t data;
    for(i = 0; i < max; i+=2){
        data = buf2[i*2] | (buf2[i*2+1] << 8);
        fio_printf(1,"%X\r\n", data);
    }

//fio_printf(1, "%d",chunck2_size);
    PWM_RCC_Configuration();
    PWM_GPIO_Configuration();
    PWM_TIM2_Configuration();
    PWM_TIM1_Configuration();

    while(1){}
    f_close(&fil);
}

void  PWM_RCC_Configuration(void)
{
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE , ENABLE );//Enalbe AHB for GPIOE
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_TIM1, ENABLE );//Enable APB for TIM1
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );//Enable APB for TIM2
}


void TIM1_UP_TIM10_IRQHandler(void)
{
    TIM_ClearITPendingBit(TIM1, TIM_FLAG_Update);

    int32_t data;
    if(cur_buf == 0) data = buf[cur_point*2] | (buf[cur_point*2+1] << 8);
    else data = buf2[cur_point*2] | (buf2[cur_point*2+1] << 8);

    TIM1->CCR1 = (data+32768) * 4000 / 65536;

    ++cur_point;
    if(cur_point == BUF_LENGTH / 2)  {
        cur_buf = !cur_buf;
        update_buf = 1;
        cur_point = 0;
        play_time_sec++;
    }
    //if(play_time_sec > 10 ) TIM1->CCR1 = 0;

    // USART_SendData(USART1, wave[play_time_other]) ;
    //if(play_time_sec >= 10) TIM1->CCR1 = 0;
}

void TIM2_IRQHandler(void)
{
    
    if(update_buf == 1)
    {
        // USART_SendData(USART1, 48) ;
        if(cur_buf == 0)    f_read(&fil, buf2, btr, &br);
        else f_read(&fil, buf, btr, &br);

        f_lseek(&fil, f_tell(&fil)+btr);

        update_buf = 0;
        // USART_SendData(USART1, 49) ;
    }
    

    TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
}

void  PWM_TIM2_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

  /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 39999;
    TIM_TimeBaseStructure.TIM_Prescaler = 99;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;     
    TIM_OCInitStructure.TIM_Pulse = 99; 
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
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

  /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 3999;
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