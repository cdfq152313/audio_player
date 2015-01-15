#include "gui.h"
#include "tm_stm32f4_fatfs.h"
#include "stdio.h"
#include "string.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "pwm.h"

#define MAX_LINE 12

static uint8_t state = 1;
static uint8_t press_count = 0;
static uint8_t press_count_max = 5;
static uint8_t curfile = 0;
static uint8_t maxfile = 0;
static char curdir[256] = {0};
static FILINFO file[MAX_LINE];
extern volatile xQueueHandle play_queue;

typedef void btnfunc();
typedef struct {
    const char *name;
    btnfunc *fptr;
    uint16_t Xpos;
    uint16_t Ypos;
    uint16_t Height;
    uint16_t Width;
} btnlist;

void prev_touch();
void next_touch();
void choose_touch();

#define MKCL(n, x, y, h, w) {.name=#n, .fptr=n ## _touch, .Xpos=x, .Ypos=y, .Height=h, .Width=w}
btnlist bl[]={
    MKCL(prev, 5,288,30,30),
    MKCL(choose, 75,288,30,30),
    MKCL(next, 110,288,30,30)
};

void gui_init(){
    // LCD initialization 
    LCD_Init();
    LCD_LayerInit();

    // LTDC reload configuration   
    LTDC_ReloadConfig(LTDC_IMReload);

    // Enable the LTDC 
    LTDC_Cmd(ENABLE);

    // Set LCD foreground layer 
    LCD_SetLayer(LCD_FOREGROUND_LAYER);

    // Configure the IO Expander
    IOE_Config();

    // Initialize User Button mounted on STM32F429I-DISCO 
    STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);

    // Display test name on LCD 
    LCD_Clear(LCD_COLOR_WHITE);
    LCD_SetBackColor(LCD_COLOR_WHITE);
    LCD_SetTextColor(LCD_COLOR_BLACK);

}

void display_choosen_line(uint16_t line, char * str){
    // static int i = 0;
    // (i >= file[curfile].lfsize - 3)? i = 0 : i++;
    /* Display test name on LCD */
    LCD_SetBackColor(LCD_COLOR_BLUE);
    LCD_SetTextColor(LCD_COLOR_GREY);
    LCD_ClearLine( LINE(line) );
    LCD_DisplayStringLine(LINE(line),  (uint8_t*)str);
}

void display_normal_line(uint16_t line, char * str){
    LCD_SetBackColor(LCD_COLOR_WHITE);
    LCD_SetTextColor(LCD_COLOR_BLACK);
    LCD_ClearLine( LINE(line) );
    LCD_DisplayStringLine(LINE(line), (uint8_t*)str);
}


FRESULT scan_files (char* nextdir)
{
    static FRESULT res;
    static DIR dir = {0};

    // initalize
    if(!curdir[0]){
        curdir[0] = '/';
        curdir[1] = 0;
        res = f_opendir(&dir, "/");
    }

    // open directory, if has path
    // if not, use previous dir
    if (nextdir) {
        f_closedir(&dir);
        if (nextdir[0] == '.' && nextdir[1] == '.' && nextdir[2] == 0) {
            char * pch;
            pch=strrchr(curdir,'/');
            if(pch != curdir)
                *pch = 0;
            else
                curdir[1] = 0;
            res = f_opendir(&dir, curdir);
        } else {
            //f_closedir(&dir);
            if(curdir[1])
                strcat (curdir,"/");
            strcat (curdir,nextdir);
            res = f_opendir(&dir, curdir);
        } 
    }
    

    uint16_t index = 0;
    curfile = 0;
    LCD_Clear(LCD_COLOR_WHITE);
    // scan file
    if (res == FR_OK) {
        
        while (index < MAX_LINE) {
            if (index == 0) {
                if (curdir[1] != 0) {
                    file[index].fname[0] = '.';
                    file[index].fname[1] = '.';
                    file[index].fname[2] = 0;
                    file[index].fattrib = AM_DIR;
                    display_choosen_line(index, file[index].fname);
                    index++;
                    continue;
                }   
            }
            res = f_readdir(&dir, &file[index]);
            // open error
            if (res != FR_OK) break;
            // current entry
            //if (file[index].fname[0] == '.' && file[index].fname[1] == 0) continue;
            // end directory
            if (file[index].fname[0] == 0){
                f_closedir(&dir);
                res = f_opendir(&dir, curdir);
                if(res != FR_OK)
                    break;
                if(index == 0)
                    continue;
                else
                    break;
            }

            // print file
            if(index == 0)
                display_choosen_line(index, file[index].fname);
            else
                display_normal_line(index, file[index].fname);
            index ++;
        }
        maxfile = index;
    }
    else LCD_DisplayStringLine(LCD_LINE_7,(uint8_t*)"open error");
    return res;
}

void choose_touch(){

}

void prev_touch(){
    display_normal_line(curfile, file[curfile].fname);
    (curfile==0) ? curfile = maxfile-1: curfile--;
    display_choosen_line(curfile, file[curfile].fname);
}

void next_touch(){
    display_normal_line(curfile, file[curfile].fname);
    curfile ++;
    if(curfile == maxfile)
        scan_files(0);
    else
        display_choosen_line(curfile, file[curfile].fname);
}

void set_touch_function(uint16_t Xpos, uint16_t Ypos, uint16_t Height, uint16_t Width, int function)
{
    LCD_SetTextColor(LCD_COLOR_RED); 
    LCD_DrawFullRect(5, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_GREEN); 
    LCD_DrawFullRect(75, 288, 30, 30);
    LCD_SetTextColor(LCD_COLOR_BLACK); 
    LCD_DrawFullRect(110, 288, 30, 30);
}

void release_button(){
    press_count = 0;
    state = 1;
}

void open_file(){
    static uint8_t playing = 0;

    if(!playing){
        playing = 1;

        uint16_t pathlen = strlen(curdir);
        display_normal_line(MAX_LINE-1, curdir);
        if(curdir[1])
            strcat(curdir, "/");
        strcat(curdir, file[curfile].fname);
        display_normal_line(MAX_LINE, curdir);
        if (play(curdir) == -1){
            display_normal_line(MAX_LINE, "open file error");
        }
        else{
            display_normal_line(MAX_LINE, "play");
        }
        curdir[pathlen] = 0;
    }
    else{
        playing = 0;
        stop();
        display_normal_line(MAX_LINE, "stop");
    }
}

void open_dir(){
    scan_files(file[curfile].fname);
}

void open_file_or_dir(){
    // It is a directory
    if (file[curfile].fattrib & AM_DIR) {
        open_dir();
    }
    // It is a file.
    else {
        open_file();
    }
}


void gui_start(void *pvParameters){
    scan_files(0);
    while(1){
        switch(state){
        case 1:
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_RESET){
                press_count ++;
                state = 2;
            }
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET){
                release_button();
                // display_choosen_line(curfile, file[curfile].fname);
            }
        break;
        case 2:
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_RESET){
                press_count ++;
                if(press_count >= press_count_max)
                    state = 3;
            }
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET){
                next_touch();
                release_button();
            }
        break;
        case 3:
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_RESET){
                open_file_or_dir();
                state = 4;
            }
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET)
                release_button();
        break;
        case 4:
            if (STM_EVAL_PBGetState(BUTTON_USER) != Bit_SET)
                release_button();
        break;
        }
        vTaskDelay(50);
    }
}


// void gui_play(void *pvParameters){
//     portBASE_TYPE xStatus;
//     FILINFO file;
//     uint8_t playing = 0;
//     while(1){
//         xStatus = xQueueReceive( play_queue, &file, portMAX_DELAY );
//         if( xStatus == pdPASS )
//         {
//             if(!playing){
//                 playing = 1;
//                 if (play(file.fname) == -1)
//                     display_normal_line(MAX_LINE, "open file error");
//                 else
//                     display_normal_line(MAX_LINE, "play");
//             }
//             else{
//                 playing = 0;
//                 stop();
//                 display_normal_line(MAX_LINE, "stop");
//             }
//         }
//         else{
//             LCD_ClearLine(LINE(5));
//             LCD_DisplayStringLine(LINE(5),(uint8_t*)"could not receive data");
//         }
//     }
// }
