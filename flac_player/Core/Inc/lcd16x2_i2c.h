#ifndef LCD16X2_I2C_H_
#define LCD16X2_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdbool.h>

bool lcd16x2_i2c_init(I2C_HandleTypeDef *pI2cHandle);

void lcd16x2_i2c_set_cursor(uint8_t row, uint8_t col);

void lcd16x2_i2c_1st_line(void);

void lcd16x2_i2c_2nd_line(void);

void lcd16x2_i2c_two_lines(void);

void lcd16x2_i2c_one_line(void);

void lcd16x2_i2c_cursor_show(bool state);

void lcd16x2_i2c_clear(void);

void lcd16x2_i2c_display(bool state);

void lcd16x2_i2c_shift_right(uint8_t offset);

void lcd16x2_i2c_shift_left(uint8_t offset);

void lcd16x2_i2c_audio_ch(void);

void lcd16x2_i2c_print_audio(const char *str);

void lcd16x2_i2c_printf(const char *str, ...);

#ifdef __cplusplus
}
#endif

#endif /* LCD16X2_I2C_H_ */
