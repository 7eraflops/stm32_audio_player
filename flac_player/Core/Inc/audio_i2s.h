#ifndef AUDIOI2S_H_
#define AUDIOI2S_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdbool.h"
#include "stm32f4xx_hal.h"

// Audio library defines
#define DMA_MAX_SZE 0xFFFF
#define DMA_MAX(_X_) (((_X_) <= DMA_MAX_SZE) ? (_X_) : DMA_MAX_SZE)
#define AUDIODATA_SIZE 2 /* 16-bits audio data size */

/* I2S Audio library function prototypes */

void audio_i2s_set_handle(I2S_HandleTypeDef *handle);

bool audio_i2s_init(uint32_t sampling_frequency);

bool audio_i2s_play(uint16_t *audio_buffer, uint32_t size);

bool audio_i2s_change_buffer(uint16_t *audio_buffer, uint32_t size);

void audio_i2s_pause(void);

void audio_i2s_resume(void);

void audio_i2s_set_volume(uint8_t volume);

void audio_i2s_stop(void);

void audio_i2s_half_transfer_callback(void);

void audio_i2s_full_transfer_callback(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIOI2S_H_ */
