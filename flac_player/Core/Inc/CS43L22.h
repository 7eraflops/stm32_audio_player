#ifndef CS43L22_H_
#define CS43L22_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stm32f4xx_hal.h>

#include "main.h"

#define POWER_CONTROL1 0x02
#define POWER_CONTROL2 0x04
#define CLOCKING_CONTROL 0x05
#define INTERFACE_CONTROL1 0x06
#define INTERFACE_CONTROL2 0x07
#define PASSTHROUGH_A 0x08
#define PASSTHROUGH_B 0x09
#define MISCELLANEOUS_CONTRLS 0x0E
#define PLAYBACK_CONTROL 0x0F
#define PASSTHROUGH_VOLUME_A 0x14
#define PASSTHROUGH_VOLUME_B 0x15
#define PCM_VOLUME_A 0x1A
#define PCM_VOLUME_B 0x1B
#define CONFIG_00 0x00
#define CONFIG_47 0x47
#define CONFIG_32 0x32

#define CS43L22_REG_MASTER_A_VOL 0x20
#define CS43L22_REG_MASTER_B_VOL 0x21
#define CS43L22_REG_HEADPHONE_A_VOL 0x22
#define CS43L22_REG_HEADPHONE_B_VOL 0x23

#define DAC_I2C_ADDR 0x94

#define CS43_MUTE 0x00

#define CS43_RIGHT 0x01
#define CS43_LEFT 0x02
#define CS43_RIGHT_LEFT 0x03

#define VOLUME_MASTER(volume) (((volume) >= 231 && (volume) <= 255) ? (volume - 231) : (volume + 25))

    typedef enum
    {
        CS43_MODE_I2S = 0,
        CS43_MODE_ANALOG,
    } CS43_MODE;

    void CS43_Init(I2C_HandleTypeDef i2c_handle, CS43_MODE output_mode);
    void CS43_enable_right_left(uint8_t side);
    void CS43_set_volume(uint8_t volume);
    void CS43_set_mute(bool mute);
    void CS43_start(void);
    void CS43_stop(void);

#ifdef __cplusplus
}
#endif

#endif // CS43L22_H_
