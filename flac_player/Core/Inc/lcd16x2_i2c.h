#ifndef LCD16X2_I2C_H_
#define LCD16X2_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "main.h"

bool lcd16x2_i2c_init(I2C_HandleTypeDef *i2c_handle);

void lcd16x2_i2c_set_cursor(uint8_t row, uint8_t col);

void lcd16x2_i2c_clear(void);

void lcd16x2_i2c_load_audio_characters(void);

void lcd16x2_i2c_print_audio_character(const char *str);

void lcd16x2_i2c_printf(const char *str, ...);

#ifdef __cplusplus
}
#endif

#endif /* LCD16X2_I2C_H_ */
