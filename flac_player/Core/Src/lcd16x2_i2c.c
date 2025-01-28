#include "lcd16x2_i2c.h"

#define LCD_CLEAR_DISPLAY 0x01
#define LCD_DISPLAY_CONTROL 0x08
#define LCD_FUNCTION_SET 0x20
#define LCD_SET_CGRAM_ADDR 0x40
#define LCD_SET_DDRAM_ADDR 0x80

#define LCD_ENTRY_ID 0x02
#define LCD_DISPLAY_B 0x01
#define LCD_DISPLAY_C 0x02
#define LCD_DISPLAY_D 0x04
#define LCD_FUNCTION_N 0x08

#define LCD_RS (1 << 0)
#define LCD_EN (1 << 2)
#define LCD_BACKLIGHT (1 << 3)

static I2C_HandleTypeDef *lcd16x2_i2c_handle;
static uint8_t LCD_I2C_SLAVE_ADDRESS = 0;
#define LCD_I2C_SLAVE_ADDRESS_0 0x4E
#define LCD_I2C_SLAVE_ADDRESS_1 0x7E

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

static void lcd16x2_i2c_send_command(uint8_t command)
{
    const uint8_t command_lower_nibble = (0xF0 & (command << 4));
    const uint8_t command_upper_nibble = (0xF0 & command);
    uint8_t i2c_data[4] =
        {
            command_upper_nibble | LCD_EN | LCD_BACKLIGHT,
            command_upper_nibble | LCD_BACKLIGHT,
            command_lower_nibble | LCD_EN | LCD_BACKLIGHT,
            command_lower_nibble | LCD_BACKLIGHT,
        };
    HAL_I2C_Master_Transmit(lcd16x2_i2c_handle, LCD_I2C_SLAVE_ADDRESS, i2c_data, 4, 200);
}

static void lcd16x2_i2c_send_data(uint8_t data)
{
    const uint8_t data_lower_nibble = (0xF0 & (data << 4));
    const uint8_t data_upper_nibble = (0xF0 & data);
    uint8_t i2c_data[4] =
        {
            data_upper_nibble | LCD_EN | LCD_BACKLIGHT | LCD_RS,
            data_upper_nibble | LCD_BACKLIGHT | LCD_RS,
            data_lower_nibble | LCD_EN | LCD_BACKLIGHT | LCD_RS,
            data_lower_nibble | LCD_BACKLIGHT | LCD_RS,
        };
    HAL_I2C_Master_Transmit(lcd16x2_i2c_handle, LCD_I2C_SLAVE_ADDRESS, i2c_data, 4, 200);
}

bool lcd16x2_i2c_init(I2C_HandleTypeDef *i2c_handle)
{
    lcd16x2_i2c_handle = i2c_handle;
    if (HAL_I2C_IsDeviceReady(lcd16x2_i2c_handle, LCD_I2C_SLAVE_ADDRESS_0, 5, 500) == HAL_OK)
    {
        LCD_I2C_SLAVE_ADDRESS = LCD_I2C_SLAVE_ADDRESS_0;
    }
    else
    {
        LCD_I2C_SLAVE_ADDRESS = LCD_I2C_SLAVE_ADDRESS_1;
    }

    HAL_Delay(20);

    lcd16x2_i2c_send_command(0x30);
    HAL_Delay(5);
    lcd16x2_i2c_send_command(0x30);
    HAL_Delay(1);
    lcd16x2_i2c_send_command(0x30);
    HAL_Delay(8);
    lcd16x2_i2c_send_command(0x20);
    HAL_Delay(8);

    lcd16x2_i2c_send_command(LCD_FUNCTION_SET | LCD_FUNCTION_N);
    HAL_Delay(1);
    lcd16x2_i2c_send_command(LCD_DISPLAY_CONTROL);
    HAL_Delay(1);
    lcd16x2_i2c_send_command(LCD_CLEAR_DISPLAY);
    HAL_Delay(3);
    lcd16x2_i2c_send_command(0x04 | LCD_ENTRY_ID);
    HAL_Delay(1);
    lcd16x2_i2c_send_command(LCD_DISPLAY_CONTROL | LCD_DISPLAY_D);
    HAL_Delay(3);

    return true;
}

void lcd16x2_i2c_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t command;
    command = (col) & 0x0F;
    if (row == 0)
    {
        command |= (0x80);
        lcd16x2_i2c_send_command(command);
    }
    else
    {
        command |= (0xc0);
        lcd16x2_i2c_send_command(command);
    }
}

void lcd16x2_i2c_clear(void)
{
    lcd16x2_i2c_send_command(LCD_CLEAR_DISPLAY);
}

void lcd16x2_i2c_load_audio_characters(void)
{
    uint8_t i;

    lcd16x2_i2c_send_command(LCD_SET_CGRAM_ADDR);

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

    lcd16x2_i2c_send_command(LCD_SET_DDRAM_ADDR);
}

void lcd16x2_i2c_print_audio_character(const char *str)
{
    char buf = *str;

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
}

void lcd16x2_i2c_printf(const char *str, ...)
{
    char string[20];
    va_list args;
    va_start(args, str);
    vsprintf(string, str, args);
    va_end(args);
    for (uint8_t i = 0; i < strlen(string) && i < 16; i++)
    {
        lcd16x2_i2c_send_data((uint8_t)string[i]);
    }
}
