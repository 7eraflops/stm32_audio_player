/*
 * lcd16x2_i2c.c
 *
 *  Created on: Mar 28, 2020
 *      Author: Mohamed Yaqoob
 */

#include "lcd16x2_i2c.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* LCD Commands */
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

/* Commands bitfields */
// 1) Entry mode Bitfields
#define LCD_ENTRY_SH 0x01
#define LCD_ENTRY_ID 0x02
// 2) Display control
#define LCD_DISPLAY_B 0x01
#define LCD_DISPLAY_C 0x02
#define LCD_DISPLAY_D 0x04
// 3) Shift control
#define LCD_SHIFT_RL 0x04
#define LCD_SHIFT_SC 0x08
// 4) Function set control
#define LCD_FUNCTION_F 0x04
#define LCD_FUNCTION_N 0x08
#define LCD_FUNCTION_DL 0x10

/* I2C Control bits */
#define LCD_RS (1 << 0)
#define LCD_RW (1 << 1)
#define LCD_EN (1 << 2)
#define LCD_BK_LIGHT (1 << 3)

/* Library variables */
static I2C_HandleTypeDef *lcd16x2_i2cHandle;
static uint8_t LCD_I2C_SLAVE_ADDRESS = 0;
#define LCD_I2C_SLAVE_ADDRESS_0 0x4E
#define LCD_I2C_SLAVE_ADDRESS_1 0x7E

/* Custom characters */
static const char CH_play[] = {
    0b10000,
    0b11000,
    0b11100,
    0b11110,
    0b11110,
    0b11100,
    0b11000,
    0b10000};
static const char CH_play_inv[] = {
    0b01111,
    0b00111,
    0b00011,
    0b00001,
    0b00001,
    0b00011,
    0b00111,
    0b01111};
static const char CH_pause[] = {
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11011,
    0b11011};
static const char CH_pause_inv[] = {
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100,
    0b00100};
static const char CH_stop[] = {
    0b00000,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b00000,
    0b00000};
static const char CH_next[] = {
    0b10001,
    0b11001,
    0b11101,
    0b11111,
    0b11111,
    0b11101,
    0b11001,
    0b10001};
static const char CH_previous[] = {
    0b10001,
    0b10011,
    0b10111,
    0b11111,
    0b11111,
    0b10111,
    0b10011,
    0b10001};
static const char CH_vol_block[] = {
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b11111};

/* Private functions */
static void lcd16x2_i2c_send_command(uint8_t command)
{
    const uint8_t command_0_3 = (0xF0 & (command << 4));
    const uint8_t command_4_7 = (0xF0 & command);
    uint8_t i2cData[4] =
        {
            command_4_7 | LCD_EN | LCD_BK_LIGHT,
            command_4_7 | LCD_BK_LIGHT,
            command_0_3 | LCD_EN | LCD_BK_LIGHT,
            command_0_3 | LCD_BK_LIGHT,
        };
    HAL_I2C_Master_Transmit(lcd16x2_i2cHandle, LCD_I2C_SLAVE_ADDRESS, i2cData, 4, 200);
}

static void lcd16x2_i2c_send_data(uint8_t data)
{
    const uint8_t data_0_3 = (0xF0 & (data << 4));
    const uint8_t data_4_7 = (0xF0 & data);
    uint8_t i2cData[4] =
        {
            data_4_7 | LCD_EN | LCD_BK_LIGHT | LCD_RS,
            data_4_7 | LCD_BK_LIGHT | LCD_RS,
            data_0_3 | LCD_EN | LCD_BK_LIGHT | LCD_RS,
            data_0_3 | LCD_BK_LIGHT | LCD_RS,
        };
    HAL_I2C_Master_Transmit(lcd16x2_i2cHandle, LCD_I2C_SLAVE_ADDRESS, i2cData, 4, 200);
}

bool lcd16x2_i2c_init(I2C_HandleTypeDef *pI2cHandle)
{
    HAL_Delay(50);
    lcd16x2_i2cHandle = pI2cHandle;
    if (HAL_I2C_IsDeviceReady(lcd16x2_i2cHandle, LCD_I2C_SLAVE_ADDRESS_0, 5, 500) != HAL_OK)
    {
        if (HAL_I2C_IsDeviceReady(lcd16x2_i2cHandle, LCD_I2C_SLAVE_ADDRESS_1, 5, 500) != HAL_OK)
        {
            return false;
        }
        else
        {
            LCD_I2C_SLAVE_ADDRESS = LCD_I2C_SLAVE_ADDRESS_1;
        }
    }
    else
    {
        LCD_I2C_SLAVE_ADDRESS = LCD_I2C_SLAVE_ADDRESS_0;
    }
    // Initialise LCD for 4-bit operation
    // 1. Wait at least 15ms
    HAL_Delay(45);
    // 2. Attentions sequence
    lcd16x2_i2c_send_command(0x30);
    HAL_Delay(5);
    lcd16x2_i2c_send_command(0x30);
    HAL_Delay(1);
    lcd16x2_i2c_send_command(0x30);
    HAL_Delay(8);
    lcd16x2_i2c_send_command(0x20);
    HAL_Delay(8);

    lcd16x2_i2c_send_command(LCD_FUNCTIONSET | LCD_FUNCTION_N);
    HAL_Delay(1);
    lcd16x2_i2c_send_command(LCD_DISPLAYCONTROL);
    HAL_Delay(1);
    lcd16x2_i2c_send_command(LCD_CLEARDISPLAY);
    HAL_Delay(3);
    lcd16x2_i2c_send_command(0x04 | LCD_ENTRY_ID);
    HAL_Delay(1);
    lcd16x2_i2c_send_command(LCD_DISPLAYCONTROL | LCD_DISPLAY_D);
    HAL_Delay(3);

    return true;
}

void lcd16x2_i2c_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t maskData;
    maskData = (col) & 0x0F;
    if (row == 0)
    {
        maskData |= (0x80);
        lcd16x2_i2c_send_command(maskData);
    }
    else
    {
        maskData |= (0xc0);
        lcd16x2_i2c_send_command(maskData);
    }
}

void lcd16x2_i2c_1st_line(void)
{
    lcd16x2_i2c_set_cursor(0, 0);
}

void lcd16x2_i2c_2nd_line(void)
{
    lcd16x2_i2c_set_cursor(1, 0);
}

void lcd16x2_i2c_two_lines(void)
{
    lcd16x2_i2c_send_command(LCD_FUNCTIONSET | LCD_FUNCTION_N);
}
void lcd16x2_i2c_one_line(void)
{
    lcd16x2_i2c_send_command(LCD_FUNCTIONSET);
}

void lcd16x2_i2c_cursor_show(bool state)
{
    if (state)
    {
        lcd16x2_i2c_send_command(LCD_DISPLAYCONTROL | LCD_DISPLAY_B | LCD_DISPLAY_C | LCD_DISPLAY_D);
    }
    else
    {
        lcd16x2_i2c_send_command(LCD_DISPLAYCONTROL | LCD_DISPLAY_D);
    }
}

void lcd16x2_i2c_clear(void)
{
    lcd16x2_i2c_send_command(LCD_CLEARDISPLAY);
    HAL_Delay(3);
}

void lcd16x2_i2c_display(bool state)
{
    if (state)
    {
        lcd16x2_i2c_send_command(LCD_DISPLAYCONTROL | LCD_DISPLAY_B | LCD_DISPLAY_C | LCD_DISPLAY_D);
    }
    else
    {
        lcd16x2_i2c_send_command(LCD_DISPLAYCONTROL | LCD_DISPLAY_B | LCD_DISPLAY_C);
    }
}

/**
 * @brief Shift content to right
 */
void lcd16x2_i2c_shift_right(uint8_t offset)
{
    for (uint8_t i = 0; i < offset; i++)
    {
        lcd16x2_i2c_send_command(0x1c);
    }
}

/**
 * @brief Shift content to left
 */
void lcd16x2_i2c_shift_left(uint8_t offset)
{
    for (uint8_t i = 0; i < offset; i++)
    {
        lcd16x2_i2c_send_command(0x18);
    }
}

void lcd16x2_i2c_audio_ch(void)
{
    uint8_t i;

    lcd16x2_i2c_send_command(LCD_SETCGRAMADDR); // Set CGRAM address = 0

    // Load all Polish characters to CGRAM
    for (i = 0; i < 8; i++)
        lcd16x2_i2c_send_data(CH_play[i]);
    for (i = 0; i < 8; i++)
        lcd16x2_i2c_send_data(CH_play_inv[i]);
    for (i = 0; i < 8; i++)
        lcd16x2_i2c_send_data(CH_pause[i]);
    for (i = 0; i < 8; i++)
        lcd16x2_i2c_send_data(CH_pause_inv[i]);
    for (i = 0; i < 8; i++)
        lcd16x2_i2c_send_data(CH_stop[i]);
    for (i = 0; i < 8; i++)
        lcd16x2_i2c_send_data(CH_next[i]);
    for (i = 0; i < 8; i++)
        lcd16x2_i2c_send_data(CH_previous[i]);
    for (i = 0; i < 8; i++)
        lcd16x2_i2c_send_data(CH_vol_block[i]);

    lcd16x2_i2c_send_command(LCD_SETDDRAMADDR); // Set DDRAM address = 0
}

void lcd16x2_i2c_print_audio(const char *str)
{
    char buf;

    while (*str)
    {
        buf = *str;
        if (buf == '/' && *(str + 1) == '/')
        {
            str += 2;
            switch (*str)
            {
            case 'p':
                buf = 0x0;
                break;
            case 'i':
                buf = 0x01;
                break;
            case 'P':
                buf = 0x02;
                break;
            case 'I':
                buf = 0x03;
                break;
            case 's':
                buf = 0x04;
                break;
            case 'n':
                buf = 0x05;
                break;
            case 'b':
                buf = 0x06;
                break;
            case 'v':
                buf = 0x07;
                break;
            default:
                str -= 2;
                break;
            }
        }
        lcd16x2_i2c_send_data(buf);
        str++;
    }
}

void lcd16x2_i2c_printf(const char *str, ...)
{
    char stringArray[20];
    va_list args;
    va_start(args, str);
    vsprintf(stringArray, str, args);
    va_end(args);
    for (uint8_t i = 0; i < strlen(stringArray) && i < 16; i++)
    {
        lcd16x2_i2c_send_data((uint8_t)stringArray[i]);
    }
}
