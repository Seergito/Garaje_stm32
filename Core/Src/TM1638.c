#ifndef tm1638
#include "TM1638.h"

#include "stm32f4xx_hal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>


static void tm1638_SDOhigh(TM1638 *tm)
{
    HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_SET);
//   HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_SET);
}
static void tm1638_SDOlow(TM1638 *tm)
{
    HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_RESET);
//    HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_RESET);
//    HAL_GPIO_WritePin(tm->dio_port, tm->dio_pin, GPIO_PIN_RESET);
}
static void tm1638_STBhigh(TM1638 *tm)
{
    HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_SET);
//    HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_SET);
    //    HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_SET);
}
static void tm1638_STBlow(TM1638 *tm)
{
    HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_RESET);
    //     HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_RESET);
    //     HAL_GPIO_WritePin(tm->stb_port, tm->stb_pin, GPIO_PIN_RESET);
}
static void tm1638_CLKhigh(TM1638 *tm)
{
    HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_SET);
    //        HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_SET);
    //     HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_SET);
}
static void tm1638_CLKlow(TM1638 *tm)
{
    HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_RESET);
    //       HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_RESET);
    //      HAL_GPIO_WritePin(tm->clk_port, tm->clk_pin, GPIO_PIN_RESET);
}

/*
static void tm1638_InitPins(TM1638 *tm)
{
    tm1638_CLKhigh(tm);
    tm1638_STBhigh(tm);
   // HAL_GPIO_WritePin(tm->vcc_port, tm->vcc_pin, GPIO_PIN_SET);
}
*/


// send signal to start transmission of data
static void tm1638_StartPacket(TM1638 *tm)
{
    tm1638_CLKhigh(tm);
    tm1638_STBhigh(tm);
    tm1638_STBlow(tm);
    tm1638_CLKlow(tm);
}
// send signal to end transmission of data
static void tm1638_EndPacket(TM1638 *tm)
{
    tm1638_CLKlow(tm);
    tm1638_STBlow(tm);
    tm1638_CLKhigh(tm);
    tm1638_STBhigh(tm);
}
// send signal necessary to confirm transmission of data when using fix address mode
static void tm1638_Confirm(TM1638 *tm)
{
    tm1638_STBlow(tm);
    tm1638_SDOlow(tm);
    tm1638_STBhigh(tm);
    tm1638_SDOhigh(tm);
    tm1638_STBlow(tm);
    tm1638_SDOlow(tm);
}

static void tm1638_SendData(TM1638 *tm, uint8_t Data)
{
    uint8_t ByteData[8] = {0};

    // convert data to bit array
    for (uint8_t j = 0; j < 8; j++)
    {
        ByteData[j] = (Data & (0x01 << j)) && 1;
    }
    // send bit array
    for (int8_t j = 0; j < 8; j++)
    {
        tm1638_CLKlow(tm);
        if (ByteData[j] == GPIO_PIN_SET)
        {
            tm1638_SDOhigh(tm);
        }
        else
        {
            tm1638_SDOlow(tm);
        }
        tm1638_CLKhigh(tm);
    }
}


void tm1638_DisplayClear(TM1638 *tm)
{
   // tm1638_InitPins(tm);
    tm1638_StartPacket(tm);

    tm1638_SendData(tm, DATA_SET);     // 0x40 command Sets up sequential address mode.
    tm1638_Confirm(tm);

    tm1638_SendData(tm, 0xc0);   // Address 0
    for (uint8_t i = 0; i < 16; i++)
    {
        tm1638_SendData(tm, 0x00);
    }
    tm1638_Confirm(tm);


    tm1638_SendData(tm, DISPLAY_OFF); // 0x80  1000 <0><000>

    tm1638_EndPacket(tm);
}
/**
 * @param brightness must be an integer between 0 and 7
 */
void tm1638_TurnOn(TM1638 *tm, uint8_t brightness)
{
    if (brightness > 0 && brightness < 8)
    {
        brightness = brightness | DISPLAY_ON; // 0x88 1000 <1><000>
        tm1638_SendData(tm, brightness);
    }
    else
    {
        tm1638_DisplayTxt(tm, "Error 1");
        return;
    }
}

void tm1638_DisplayChar(TM1638 *tm, int position, char c, bool dot)
{
	        int hex = chr_to_hex(c);
	        if (hex >= 0)
	        {
	            if (dot)
	            {
	            	hex += 0x80; // activa punto decimal
	            }

	            tm1638_Seg(tm, position, hex);

	        }
	        else
	        {
	            tm1638_DisplayTxt(tm, "Error 7");
	            return;
	        }

}

/**
 * @param c must have a maximum length of 8(+ 8 maximum dots) and must contain only valid characters
 */
void tm1638_DisplayTxt(TM1638 *tm, char *c)
{
    char padded[8] = "        ";
    uint8_t dot[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    uint8_t p = 0;
    for (int8_t i = strlen(c) - 1; i >= 0; i--)
    {
        if (p == 8)
        {
            tm1638_DisplayTxt(tm, "Error 2");
            return;
        }

        if (c[i] == '.')
            dot[p] = 1;
        else
            padded[p++] = c[i];
    }


    for (int8_t i = 7; i >= 0; i--)
    {
    	tm1638_DisplayChar(tm, 7 - i + 1,padded[i],dot[i]);
    }

  //  tm1638_TurnOn(tm, 1);
}

void tm1638_Led(TM1638 *tm, int position, int on)
{
    if (!(position >= 1 && position <= 8))
    {
        tm1638_DisplayTxt(tm, "Error 3");
        return;
    }
    if (!(on == 0 || on == 1))
    {
        tm1638_DisplayTxt(tm, "Error 4");
        return;
    }
   // tm1638_InitPins(tm);
    tm1638_StartPacket(tm);

    tm1638_SendData(tm, DATA_SET);  // 0x40 command Sets up sequential address mode.
    tm1638_Confirm(tm);

    //tm1638_SendData(tm, 0xc0 + 0x01 * 2 * position - 0x01); // Address
    tm1638_SendData(tm, 0xc0 +  2 * position - 0x01); // Address impar LED 2n-1

    tm1638_CLKhigh(tm);
    tm1638_SDOhigh(tm);
    tm1638_SDOlow(tm);

    tm1638_SendData(tm, on);
    tm1638_Confirm(tm);

    tm1638_TurnOn(tm, 7);

    tm1638_EndPacket(tm);
}


void tm1638_Seg(TM1638 *tm, int position, int data)
{
    if (!(position >= 1 && position <= 8))
    {
        tm1638_DisplayTxt(tm, "Error 5");
        return;
    }
    if (!(data >= 0x00 && data <= 0xff))
    {
        tm1638_DisplayTxt(tm, "Error 6");
        return;
    }
   // tm1638_InitPins(tm);
    tm1638_StartPacket(tm);

    tm1638_SendData(tm, DATA_SET); // 0x40 command Sets up sequential address mode.
    tm1638_Confirm(tm);

   // tm1638_SendData(tm, 0xc0 + 0x01 * 2 * (position - 1)); // Address par DISPLAY
    tm1638_SendData(tm, 0xc0 +  2 * (position - 1)); // Address par DISPLAY 2 (n-1)

    tm1638_CLKhigh(tm);
    tm1638_SDOhigh(tm);
    tm1638_SDOlow(tm);
    tm1638_SendData(tm, data);
    tm1638_Confirm(tm);

    tm1638_TurnOn(tm, 7);

    tm1638_EndPacket(tm);
}

bool tm1638_KeyState(TM1638 *tm,uint8_t buttons,int position)
{
	if (!(position >= 1 && position <= 8))
	{
		tm1638_DisplayTxt(tm, "Error K");
		return false;
	}
	//uint8_t buttons=tm1638_ScanButtons(tm);
    bool ok= (buttons & 1<<(position-1)) != 0;

	return (ok);
}


// S1... 1 S2... 9 S3... 17 S4... 25 S5... 5 S6... 13 S7... 21 S8... 29
static int a[32] = {0, 0, 0, 0, 0, 0, 0, 0,
		     0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0, 0, 0, 0};
uint8_t tm1638_ScanButtons(TM1638 *tm)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
  //  HAL_GPIO_WritePin(VCC_GPIO_Port, VCC_Pin, 1);

    int ok = 1;
    int keys=0;

            tm1638_StartPacket(tm);
            tm1638_SendData(tm, DATA_READ); //0x42 Read the buttons.

            GPIO_InitStruct.Pin = DIO_Pin;
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            HAL_GPIO_Init(DIO_GPIO_Port, &GPIO_InitStruct);

            int indice=-1;
            for (int i = 0; i < 4; i++)  // 4 bytes
            {
                tm1638_CLKhigh(tm);
                for (int j = 0; j < 8; j++)  // 8 bits
                {
                    tm1638_CLKhigh(tm);

                   // HAL_Delay(1);  por que???????
                    indice++;
                    a[indice] = HAL_GPIO_ReadPin(DIO_GPIO_Port, DIO_Pin);
                    if (a[indice] && j != 0)
                        ok = 0;  // button pressed

                    tm1638_CLKlow(tm);
                }
            }
            tm1638_CLKhigh(tm);
            tm1638_STBhigh(tm);


        GPIO_InitStruct.Pin = DIO_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        HAL_GPIO_Init(DIO_GPIO_Port, &GPIO_InitStruct);

    if (ok==0) {
        if (a[1]==1) keys |= 0x1;
       if (a[9]==1) keys |= 0x2;
       if (a[17]==1) keys |= 0x4;
       if (a[25]==1) keys |= 0x8;
       if (a[5]==1) keys |= 0x10;
       if (a[13]==1) keys |= 0x20;
       if (a[21]==1) keys |= 0x40;
       if (a[29]==1) keys |= 0x80;
    }

    return keys;
}



uint8_t tm1638_ReadKey(TM1638 *tm,uint8_t buttons)
{
int key=0;

for (int i=0; i<8; i++){
  if ((buttons & (1<<i)) != 0)
	  key += i+1;
}
  return key;
}


uint8_t tm1638_ReadKey_Blocking(TM1638 *tm)
{
	uint8_t buttons=0;
	do {
	  buttons=tm1638_ScanButtons(tm);
	} while (buttons==0);

	uint8_t key=tm1638_ReadKey(tm,buttons);
    return key;
}

void tm1638_Draw(TM1638 *tm)
{
    int pos = 0;
    int current = 0x00;
    int key = 0;

    tm1638_DisplayClear(tm);
   // tm1638_DisplayClear(tm);
   // tm1638_DisplayClear(tm);
   // tm1638_DisplayClear(tm);
  //  HAL_GPIO_WritePin(tm->vcc_port, tm->vcc_pin, 1);

    while (1)
    {
    	uint8_t buttons=tm1638_ScanButtons(tm);

	 // uint8_t k=tm1638_ReadKey_Blocking(&TM);
	  key=tm1638_ReadKey(tm,buttons);
        if (key == 1)
        {
            if (pos)
            {
                tm1638_Led(tm, pos, 0);
            }
            pos = pos % 8 + 1;
            tm1638_Led(tm, pos, 1);
            current = 0x00;
            HAL_Delay(150);
        }
        else
        {
            current = current ^ ((int)(pow(0x02, key - 2)));
            tm1638_Seg(tm, pos, current);
            HAL_Delay(150);
        }
    }
}

int8_t chr_to_hex(char c)
{
    switch (c)
    {
    case '0':
        return 0x3f;
    case '1':
        return 0x06;
    case '2':
        return 0x5b;
    case '3':
        return 0x4f;
    case '4':
        return 0x66;
    case '5':
        return 0x6d;
    case '6':
        return 0x7d;
    case '7':
        return 0x07;
    case '8':
        return 0x7f;
    case '9':
        return 0x6f;
    case ' ':
        return 0x00;
    case 'A':
        return 0x77;
    case 'B':
        return 0x7f;
    case 'C':
        return 0x39;
    case 'E':
        return 0x79;
    case 'F':
        return 0x71;
    case 'H':
        return 0x76;
    case 'I':
        return 0x06;
    case 'J':
        return 0x0e;
    case 'L':
        return 0x38;
    case 'O':
        return 0x3f;
    case 'P':
        return 0x73;
    case 'S':
        return 0x6d;
    case 'U':
        return 0x3e;
    case '_':
        return 0x08;
    case '-':
        return 0x40;
    case 'a':
        return 0x5f;
    case 'b':
        return 0x7c;
    case 'c':
        return 0x58;
    case 'd':
        return 0x5e;
    case 'h':
        return 0x74;
    case 'i':
        return 0x04;
    case 'n':
        return 0x54;
    case 'o':
        return 0x5c;
    case 'r':
        return 0x50;
    case 't':
        return 0x78;
    case 'u':
        return 0x1c;
    case 'y':
        return 0x6e;

    }
    return -0x01;
}
#endif
