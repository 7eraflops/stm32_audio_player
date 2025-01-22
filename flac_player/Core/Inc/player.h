#ifndef WAV_PLAYER_H_
#define WAV_PLAYER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_BUFFER_SIZE 32768

// Audio buffer state
typedef struct
{
    uint32_t chunk_id;        /* 0 */
    uint32_t file_size;       /* 4 */
    uint32_t file_format;     /* 8 */
    uint32_t subchunk_1_id;   /* 12 */
    uint32_t subchunk_1_size; /* 16*/
    uint16_t audio_format;    /* 20 */
    uint16_t channel_count;   /* 22 */
    uint32_t sample_rate;     /* 24 */

    uint32_t byte_rate;       /* 28 */
    uint16_t block_align;     /* 32 */
    uint16_t bits_per_sample; /* 34 */
    uint32_t sunchunk_2_id;   /* 36 */
    uint32_t subchunk_2_size; /* 40 */

} WAV_HEADER_STRUCT;

bool is_wav_file(const char *file_name);

bool player_scan_usb(void);

bool player_file_select(const char *file_path);

void player_play(void);

void player_stop(void);

void player_process(void);

bool player_is_finished(void);

void player_pause(void);

void player_resume(void);

bool player_next_track(void);

bool player_previous_track(void);

bool player_random_track(void);

#ifdef __cplusplus
}
#endif

#endif /* WAV_PLAYER_H_ */
