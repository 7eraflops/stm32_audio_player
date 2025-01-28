#include "CS43L22.h"

static I2C_HandleTypeDef i2cx;
extern I2S_HandleTypeDef hi2s3;

static void write_register(uint8_t reg, uint8_t *data)
{
    uint8_t transmit_buffer[2] = {0};
    transmit_buffer[0] = reg;
    transmit_buffer[1] = data[0];
    HAL_I2C_Master_Transmit(&i2cx, DAC_I2C_ADDR, transmit_buffer, 2, 100);
}

static void read_register(uint8_t reg, uint8_t *data)
{
    uint8_t transmit_buffer[2] = {0};
    transmit_buffer[0] = reg;
    HAL_I2C_Master_Transmit(&i2cx, DAC_I2C_ADDR, transmit_buffer, 1, 100);
    HAL_I2C_Master_Receive(&i2cx, DAC_I2C_ADDR, data, 1, 100);
}

void CS43_Init(I2C_HandleTypeDef i2c_handle, CS43_MODE output_mode)
{
    uint8_t data[2] = {0};
    __HAL_UNLOCK(&hi2s3);
    __HAL_I2S_ENABLE(&hi2s3);

    HAL_GPIO_WritePin(AUDIO_RST_GPIO_Port, AUDIO_RST_Pin, GPIO_PIN_SET); // Enable CS43L22

    i2cx = i2c_handle;

    data[1] = 0x01;
    write_register(POWER_CONTROL1, data);

    data[1] = (2 << 6);
    data[1] |= (2 << 4);
    data[1] |= (3 << 2);
    data[1] |= (3 << 0);
    write_register(POWER_CONTROL2, &data[1]);

    data[1] = (1 << 7);
    write_register(CLOCKING_CONTROL, &data[1]);

    read_register(INTERFACE_CONTROL1, data);
    data[1] &= (1 << 5);
    data[1] &= ~(1 << 7);
    data[1] &= ~(1 << 6);
    data[1] &= ~(1 << 4);
    data[1] &= ~(1 << 2);
    data[1] |= (1 << 2);

    data[1] |= (3 << 0);
    write_register(INTERFACE_CONTROL1, &data[1]);

    read_register(PASSTHROUGH_A, &data[1]);
    data[1] &= 0xF0;
    data[1] |= (1 << 0);
    write_register(PASSTHROUGH_A, &data[1]);

    read_register(PASSTHROUGH_B, &data[1]);
    data[1] &= 0xF0;
    data[1] |= (1 << 0);
    write_register(PASSTHROUGH_B, &data[1]);

    read_register(MISCELLANEOUS_CONTRLS, &data[1]);
    if (output_mode == CS43_MODE_ANALOG)
    {
        data[1] |= (1 << 7);
        data[1] |= (1 << 6);
        data[1] &= ~(1 << 5);
        data[1] &= ~(1 << 4);
        data[1] &= ~(1 << 3);
    }
    else if (output_mode == CS43_MODE_I2S)
    {
        data[1] = 0x02;
    }
    write_register(MISCELLANEOUS_CONTRLS, &data[1]);

    read_register(PLAYBACK_CONTROL, &data[1]);
    data[1] = 0x00;
    write_register(PLAYBACK_CONTROL, &data[1]);

    data[1] = 0;
    write_register(PASSTHROUGH_VOLUME_A, &data[1]);
    write_register(PASSTHROUGH_VOLUME_B, &data[1]);
    write_register(PCM_VOLUME_A, &data[1]);
    write_register(PCM_VOLUME_B, &data[1]);
}

void CS43_enable_right_left(uint8_t side)
{
    uint8_t data[2] = {0};
    switch (side)
    {
    case 0: // Both headphone channels off
        data[1] = (3 << 6);
        data[1] |= (3 << 4);
        break;
    case 1: // Right headphone channel on
        data[1] = (2 << 6);
        data[1] |= (3 << 4);
        break;
    case 2: // Left headphone channel on
        data[1] = (3 << 6);
        data[1] |= (2 << 4);
        break;
    case 3: // Both headphone channels on
        data[1] = (2 << 6);
        data[1] |= (2 << 4);
        break;
    default:
        break;
    }
    // Both speaker channels always off
    data[1] |= (3 << 2);
    data[1] |= (3 << 0);
    write_register(POWER_CONTROL2, &data[1]);
}

void CS43_set_volume(uint8_t volume)
{
    uint8_t data[2] = {0};
    data[1] = VOLUME_MASTER(volume);
    write_register(CS43L22_REG_MASTER_A_VOL, &data[1]);
    write_register(CS43L22_REG_MASTER_B_VOL, &data[1]);
}

void CS43_set_mute(bool mute)
{
    uint8_t data[2] = {0};
    if (mute)
    {
        data[1] = 0xFF;
        write_register(POWER_CONTROL2, &data[1]);
        data[1] = 0x01;
        write_register(CS43L22_REG_HEADPHONE_A_VOL, &data[1]);
        write_register(CS43L22_REG_HEADPHONE_B_VOL, &data[1]);
    }
    else
    {
        data[1] = 0x00;
        write_register(CS43L22_REG_HEADPHONE_A_VOL, &data[1]);
        write_register(CS43L22_REG_HEADPHONE_B_VOL, &data[1]);
        data[1] = 0xAF;
        write_register(POWER_CONTROL2, &data[1]);
    }
}

void CS43_start(void)
{
    uint8_t data[2] = {0};
    CS43_set_mute(0);
    // Write 0x99 to register 0x00.
    data[1] = 0x99;
    write_register(CONFIG_00, &data[1]);
    // Write 0x80 to register 0x47.
    data[1] = 0x80;
    write_register(CONFIG_47, &data[1]);
    // Write '1'b to bit 7 in register 0x32.
    read_register(CONFIG_32, &data[1]);
    data[1] |= 0x80;
    write_register(CONFIG_32, &data[1]);
    // Write '0'b to bit 7 in register 0x32.
    read_register(CONFIG_32, &data[1]);
    data[1] &= ~(0x80);
    write_register(CONFIG_32, &data[1]);
    // Write 0x00 to register 0x00.
    data[1] = 0x00;
    write_register(CONFIG_00, &data[1]);
    // Set the "Power Ctl 1" register (0x02) to 0x9E
    data[1] = 0x9E;
    write_register(POWER_CONTROL1, &data[1]);
}

void CS43_stop(void)
{
    uint8_t data[2] = {0};
    CS43_set_mute(1);
    data[1] = 0x04;
    write_register(MISCELLANEOUS_CONTRLS, &data[1]);
    data[1] = 0x9F;
    write_register(POWER_CONTROL1, &data[1]);
}
