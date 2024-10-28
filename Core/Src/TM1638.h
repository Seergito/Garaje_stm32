#ifndef tm1638
#define tm1638

#include <stdbool.h>
#include "main.h"


#define DATA_SET 0x40
#define DATA_READ 0x42
#define DISPLAY_ON 0x88 //1000 1000
#define DISPLAY_OFF 0x80 //0000 0000

typedef struct
{
    GPIO_TypeDef *clk_port;
    GPIO_TypeDef *dio_port;
    GPIO_TypeDef *stb_port;
    GPIO_TypeDef *vcc_port;
    uint16_t clk_pin, dio_pin, stb_pin; //, vcc_pin;
    int key;

} TM1638;


void tm1638_Draw(TM1638 *tm);
void tm1638_Seg(TM1638 *tm, int position, int data);
int8_t chr_to_hex(char c);

void tm1638_DisplayClear(TM1638 *tm);
void tm1638_TurnOn(TM1638 *tm, uint8_t brightness);

void tm1638_Led(TM1638 *tm, int position, int on);
void tm1638_DisplayTxt(TM1638 *tm, char *c);
void tm1638_DisplayChar(TM1638 *tm, int position, char c, bool dot);

uint8_t tm1638_ReadKey(TM1638 *tm,uint8_t buttons);
uint8_t tm1638_ReadKey_Blocking(TM1638 *tm);
bool tm1638_KeyState(TM1638 *tm,uint8_t buttons,int position);
uint8_t tm1638_ScanButtons(TM1638 *tm);

#endif
