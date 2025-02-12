#include "audio_i2s.h"

// I2S PLL parameters for different I2S Sampling Frequency (assuming 1 MHz input clock)
const uint32_t I2SFreq[8] = {8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000};
const uint32_t I2SPLLN[8] = {256, 429, 213, 429, 426, 271, 258, 344};
const uint32_t I2SPLLR[8] = {5, 4, 4, 4, 4, 6, 3, 1};

static I2S_HandleTypeDef *i2s_audio_handle;

static void audio_i2s_pll_clock_config(uint32_t sampling_frequency)
{
    RCC_PeriphCLKInitTypeDef rccclkinit;
    uint8_t index = 0, frequency_index = 0xFF;

    for (index = 0; index < 8; index++)
    {
        if (I2SFreq[index] == sampling_frequency)
        {
            frequency_index = index;
        }
    }
    /* Enable PLLI2S clock */
    HAL_RCCEx_GetPeriphCLKConfig(&rccclkinit);
    /* PLLI2S_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
    if ((frequency_index & 0x7) == 0)
    {
        /* I2S clock config
        PLLI2S_VCO = f(VCO clock) = f(PLLI2S clock input) × (PLLI2SN/PLLM)
        I2SCLK = f(PLLI2S clock output) = f(VCO clock) / PLLI2SR */
        rccclkinit.PeriphClockSelection = RCC_PERIPHCLK_I2S;
        rccclkinit.PLLI2S.PLLI2SN = I2SPLLN[frequency_index];
        rccclkinit.PLLI2S.PLLI2SR = I2SPLLR[frequency_index];
        HAL_RCCEx_PeriphCLKConfig(&rccclkinit);
    }
    else
    {
        /* I2S clock config
        PLLI2S_VCO = f(VCO clock) = f(PLLI2S clock input) × (PLLI2SN/PLLM)
        I2SCLK = f(PLLI2S clock output) = f(VCO clock) / PLLI2SR */
        rccclkinit.PeriphClockSelection = RCC_PERIPHCLK_I2S;
        rccclkinit.PLLI2S.PLLI2SN = 258;
        rccclkinit.PLLI2S.PLLI2SR = 3;
        HAL_RCCEx_PeriphCLKConfig(&rccclkinit);
    }
}

static bool i2s3_frequency_update(uint32_t sampling_frequency)
{
    i2s_audio_handle->Instance = SPI3;

    __HAL_I2S_DISABLE(i2s_audio_handle);

    i2s_audio_handle->Init.AudioFreq = sampling_frequency;
    i2s_audio_handle->Init.ClockSource = I2S_CLOCK_PLL;
    i2s_audio_handle->Init.CPOL = I2S_CPOL_LOW;
    i2s_audio_handle->Init.DataFormat = I2S_DATAFORMAT_16B;
    i2s_audio_handle->Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    i2s_audio_handle->Init.Mode = I2S_MODE_MASTER_TX;
    i2s_audio_handle->Init.Standard = I2S_STANDARD_PHILIPS;
    if (HAL_I2S_Init(i2s_audio_handle) != HAL_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void audio_i2s_set_handle(I2S_HandleTypeDef *handle)
{
    i2s_audio_handle = handle;
}

bool audio_i2s_init(uint32_t audio_frequency)
{
    audio_i2s_pll_clock_config(audio_frequency);
    i2s3_frequency_update(audio_frequency);
    return true;
}

bool audio_i2s_play(uint16_t *audio_buffer, uint32_t size)
{
    CS43_start();
    HAL_I2S_Transmit_DMA(i2s_audio_handle, audio_buffer, DMA_MAX(size / AUDIODATA_SIZE));
    return true;
}

bool audio_i2s_change_buffer(uint16_t *audio_buffer, uint32_t size)
{
    HAL_I2S_Transmit_DMA(i2s_audio_handle, audio_buffer, DMA_MAX(size));
    return true;
}

void audio_i2s_pause(void)
{
    CS43_stop();
    HAL_I2S_DMAPause(i2s_audio_handle);
}

void audio_i2s_resume(void)
{
    CS43_start();
    HAL_I2S_DMAResume(i2s_audio_handle);
}

void audio_i2s_set_volume(uint8_t volume)
{
    CS43_set_volume(volume);
}

void audio_i2s_stop(void)
{
    CS43_stop();
    HAL_I2S_DMAStop(i2s_audio_handle);
}

__weak void audio_i2s_half_transfer_callback(void)
{
}

__weak void audio_i2s_full_transfer_callback(void)
{
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI3)
    {
        audio_i2s_full_transfer_callback();
    }
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI3)
    {
        audio_i2s_half_transfer_callback();
    }
}
