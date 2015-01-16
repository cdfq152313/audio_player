#include "pwm.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "misc.h"
#include <math.h>
#include "ff.h"
#include "stm32f4xx_dac.h"
#include "mp3dec.h"
#include "mp3common.h"
#include "gui.h"

uint32_t play_time_other = 0;
uint32_t play_time_sec = 0;

#define BUF_LENGTH 2304
#define READBUF_SIZE 2000

int i;

uint16_t *buf;
uint16_t *buf2;
static uint8_t readBuf[READBUF_SIZE];
int chunck2_size = 0;
uint8_t cur_buf = 0;
uint8_t update_buf = 0;
uint32_t cur_point = 0;

FIL fil;
static HMP3Decoder hMP3Decoder;
MP3FrameInfo mp3FrameInfo; 
volatile u32 bytesLeft = 0; 
volatile u32 outOfData = 0;
uint8_t *readPtr = readBuf;
uint32_t offset;  
UINT cnt;
int count = 0;
int ok = 1;
int ismp3 = 0;


void play_test(void *p){
    play("C128.mp3");
}

int play(char * name)
{
    if (f_open(&fil, name, FA_OPEN_ALWAYS | FA_READ) != FR_OK) {
        return -1;
    }

    if (strstr(name, "MP3")) ismp3 = 1;

    TIM_Cmd(TIM1, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    //DAC_Cmd(DAC_Channel_1, ENABLE);
    //DAC_Cmd(DAC_Channel_2, ENABLE);

    for(int i=0;i<BUF_LENGTH;i++)
    {
        buf[i] = 32768;
        buf2[i] = 32768;
    }

    int cnt;
    f_read(&fil,readBuf,sizeof(readBuf),&cnt);
    bytesLeft += cnt;
    readPtr = readBuf;

    return 0;
}

void stop()
{
    TIM_Cmd(TIM1, DISABLE);
    TIM_Cmd(TIM2, DISABLE);
    //DAC_Cmd(DAC_Channel_1, DISABLE);
    //DAC_Cmd(DAC_Channel_2, DISABLE);

    play_time_other = 0;
    play_time_sec = 0;
    bytesLeft = 0;
    outOfData = 0;

    f_close(&fil);
}

void player_init(void)
{
    PWM_RCC_Configuration();
    //DAC_Configuration();
    hMP3Decoder = MP3InitDecoder();
    PWM_GPIO_Configuration();
    PWM_TIM2_Configuration();
    PWM_TIM1_Configuration();

    buf = pvPortMalloc(sizeof(uint16_t)*BUF_LENGTH);
    buf2 = pvPortMalloc(sizeof(uint16_t)*BUF_LENGTH);
}

void DAC_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

  /* DMA1 clock and GPIOA clock enable (to be used with DAC) */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  /* DAC Periph clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

  /* DAC channel 1 & 2 (DAC_OUT1 = PA.4)(DAC_OUT2 = PA.5) configuration */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    // GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN ;

    GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* TIM6 Configuration ------------------------------------------------------*/ 

    //DAC_DeInit(); 

     /* DAC channel2 Configuration */
    DAC_InitTypeDef  DAC_InitStructure;
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Disable;
    // DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);
    //DAC_DualSoftwareTriggerCmd(ENABLE);
    // DAC_SoftwareTriggerCmd(DAC_Channel_1,ENABLE);
    DAC_SoftwareTriggerCmd(DAC_Channel_2,ENABLE);
}

void  PWM_RCC_Configuration(void)
{
    RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );//Enalbe AHB for GPIOB
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_TIM1, ENABLE );//Enable APB for TIM1
    RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM2, ENABLE );//Enable APB for TIM2
}


void TIM1_UP_TIM10_IRQHandler(void)
{
    TIM_ClearITPendingBit(TIM1, TIM_FLAG_Update);

    // if(cur_point == 0)
    // {
    //     if(cur_buf == 1 && ok == 0) {
    //         char ch[12];
    //         sprintf(ch,"%d",++count);
    //         LCD_ClearLine( LINE(10) );
    //         LCD_DisplayStringLine(LINE(10), ch);
    //     }
    // }
    if(cur_buf == 0) 
    {

        TIM1->CCR1 = buf[cur_point*2];
        TIM1->CCR2 = buf[cur_point*2+1];

        // if(cur_point%16 == 0)
        // {
        //     // DAC_SetChannel1Data(DAC_Align_12b_R,data1);
        //     DAC_SetChannel2Data(DAC_Align_12b_R,data2);
        // } 
        // else 
        // {
            
        //     if(cur_point%16 == 15 - buf[(cur_point/16)*2+1] % 16)
        //     {
        //         if(data2 < 4094)
        //         {
        //             ++data2;
        //             DAC_SetChannel1Data(DAC_Align_12b_R,data2);
        //         }
                
        //     }
        // }
    }
    else
    {
        TIM1->CCR1 = buf2[cur_point*2];
        TIM1->CCR2 = buf2[cur_point*2+1];


        // if(cur_point%16 == 0)
        // {
            
        //     // DAC_SetChannel1Data(DAC_Align_12b_R,data1);
        //     DAC_SetChannel2Data(DAC_Align_12b_R,data2);
        // } 
        // else 
        // {
        //     if(cur_point%16 == 15 - buf2[(cur_point/16)*2+1] % 16)
        //     {
        //         if(data2 < 4094)
        //         {
        //             ++data2;
        //             DAC_SetChannel2Data(DAC_Align_12b_R,data2);
        //         }
        //     }
        // }
        
    }

    ++cur_point;

    if(cur_point == BUF_LENGTH / 2)  {
        if(cur_buf == 1)ok = 0;
        cur_buf = !cur_buf;
        update_buf = 1;
        cur_point = 0;
        play_time_sec++;
    }
}

void decode(uint16_t *outbuf)
{
    offset = MP3FindSyncWord(readPtr, READBUF_SIZE);
    
    if(offset < 0)
    {
        stop();
    }
    else
    {
        readPtr += offset;                         //data start point
        bytesLeft -= offset;                 //in buffer
        MP3Decode(hMP3Decoder, &readPtr, &bytesLeft, outbuf, 0);

        if (bytesLeft < READBUF_SIZE)
        {
            memmove(readBuf,readPtr,bytesLeft);
            f_read(&fil, readBuf + bytesLeft, READBUF_SIZE - bytesLeft, &cnt);
            if (cnt < READBUF_SIZE - bytesLeft);
                memset(readBuf + bytesLeft + cnt, 0, READBUF_SIZE - bytesLeft - cnt);
            bytesLeft=READBUF_SIZE;
            readPtr=readBuf;               
        }

        MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
        //if(mp3FrameInfo.outputSamps != 2304)   decode(outbuf);
    }
}

void TIM2_IRQHandler(void)
{
    if(update_buf == 1)
    {
        if(cur_buf == 0)   
        {
ok = 1;
            if(ismp3)decode(buf2);
            else    f_read(&fil, buf2, BUF_LENGTH*sizeof(uint16_t), &cnt);
            int i;
            for(i=0;i<BUF_LENGTH;i++)
            {
                buf2[i] = ((int)((short)buf2[i])+32768)* 4000 / 65536;
            }


        }
        else
        {
            if(ismp3)decode(buf);
            else    f_read(&fil, buf, BUF_LENGTH*sizeof(uint16_t), &cnt);
            
            int i;
            for(i=0;i<BUF_LENGTH;i++)
            {
                buf[i] = ((int)((short)buf[i])+32768)* 4000 / 65536;
            }          
        }
        update_buf = 0;
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
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 500-1;
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
    TIM_Cmd(TIM2, DISABLE);
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
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;

    TIM_OCInitStructure.TIM_Pulse = 3999;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset; 

    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    TIM_ClearITPendingBit(TIM1,TIM_IT_Update);
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

 /* TIM4 enable counter */
    TIM_Cmd(TIM1, DISABLE);
}

void PWM_GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_TIM1);
}