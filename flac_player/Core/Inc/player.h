#ifndef PLAYER_H_
#define PLAYER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <fatfs.h>
#include <stdbool.h>
#include <stdint.h>

#include "audio_i2s.h"
#include "files.h"

#define AUDIO_BUFFER_SIZE 4096

    typedef struct
    {
        uint32_t chunk_id;
        uint32_t file_size;
        uint32_t file_format;
        uint32_t subchunk_1_id;
        uint32_t subchunk_1_size;
        uint16_t audio_format;
        uint16_t channel_count;
        uint32_t sample_rate;

        uint32_t byte_rate;
        uint16_t block_align;
        uint16_t bits_per_sample;
        uint32_t sunchunk_2_id;
        uint32_t subchunk_2_size;

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

#endif /* PLAYER_H_ */
