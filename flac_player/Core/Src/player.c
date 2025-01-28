#include "player.h"

extern RNG_HandleTypeDef hrng;
extern uint32_t random_number;
extern bool random_number_ready;

extern FIL wav_file;
extern FILE_LIST wav_file_list;

static uint32_t file_length;
static uint8_t audio_buffer[AUDIO_BUFFER_SIZE];
static __IO uint32_t audio_size_remaining = 0;

static uint32_t sampling_frequency;
static UINT player_read_bytes = 0;
static bool is_finished = false;

typedef enum
{
    PLAYER_STATE_IDLE = 0,
    PLAYER_STATE_HALF_BUFFER,
    PLAYER_STATE_FULL_BUFFER,
    PLAYER_STATE_END_OF_FILE,
} PLAYER_STATE_ENUM;
volatile PLAYER_STATE_ENUM player_state = PLAYER_STATE_IDLE;

static void player_reset(void)
{
    audio_size_remaining = 0;
    player_read_bytes = 0;
}

bool player_file_select(const char *file_path)
{
    WAV_HEADER_STRUCT wav_header;
    UINT readBytes = 0;

    f_close(&wav_file);

    if (f_open(&wav_file, file_path, FA_READ) != FR_OK)
    {
        return false;
    }
    f_read(&wav_file, &wav_header, sizeof(wav_header), &readBytes);
    file_length = wav_header.file_size;
    sampling_frequency = wav_header.sample_rate;
    return true;
}

void player_play(void)
{
    is_finished = false;
    audio_i2s_init(sampling_frequency);

    f_read(&wav_file, &audio_buffer[0], AUDIO_BUFFER_SIZE, &player_read_bytes);
    audio_size_remaining = file_length - player_read_bytes;
    audio_i2s_play((uint16_t *)&audio_buffer[0], AUDIO_BUFFER_SIZE);
}

void player_process(void)
{
    switch (player_state)
    {
    case PLAYER_STATE_IDLE:
        break;

    case PLAYER_STATE_HALF_BUFFER:
        player_read_bytes = 0;
        player_state = PLAYER_STATE_IDLE;
        f_read(&wav_file, &audio_buffer[0], AUDIO_BUFFER_SIZE / 2, &player_read_bytes);
        if (audio_size_remaining > (AUDIO_BUFFER_SIZE / 2))
        {
            audio_size_remaining -= player_read_bytes;
        }
        else
        {
            audio_size_remaining = 0;
            player_state = PLAYER_STATE_END_OF_FILE;
        }
        break;

    case PLAYER_STATE_FULL_BUFFER:
        player_read_bytes = 0;
        player_state = PLAYER_STATE_IDLE;
        f_read(&wav_file, &audio_buffer[AUDIO_BUFFER_SIZE / 2], AUDIO_BUFFER_SIZE / 2, &player_read_bytes);
        if (audio_size_remaining > (AUDIO_BUFFER_SIZE / 2))
        {
            audio_size_remaining -= player_read_bytes;
        }
        else
        {
            audio_size_remaining = 0;
            player_state = PLAYER_STATE_END_OF_FILE;
        }
        break;

    case PLAYER_STATE_END_OF_FILE:
        f_close(&wav_file);
        player_reset();
        is_finished = true;
        player_state = PLAYER_STATE_IDLE;
        break;
    }
}

void player_stop(void)
{
    audio_i2s_stop();
    is_finished = true;
}

void player_pause(void)
{
    audio_i2s_pause();
}

void player_resume(void)
{
    audio_i2s_resume();
}

bool player_next_track(void)
{
    audio_i2s_stop();
    wav_file_list.current_index = (wav_file_list.current_index + 1) % wav_file_list.count;
    return player_file_select(wav_file_list.filenames[wav_file_list.current_index]);
}

bool player_previous_track(void)
{
    audio_i2s_stop();
    wav_file_list.current_index = (wav_file_list.current_index - 1 + wav_file_list.count) % wav_file_list.count;
    return player_file_select(wav_file_list.filenames[wav_file_list.current_index]);
}

bool player_random_track(void)
{
    audio_i2s_stop();

    if (wav_file_list.count == 0 || wav_file_list.count == 1)
    {
        return false;
    }

    int new_index;

    do
    {
        if (random_number_ready)
        {
            random_number = HAL_RNG_ReadLastRandomNumber(&hrng);
            random_number_ready = false;
            HAL_RNG_GenerateRandomNumber_IT(&hrng);
        }
        new_index = random_number % wav_file_list.count;
    } while (new_index == wav_file_list.current_index);

    wav_file_list.current_index = new_index;

    return player_file_select(wav_file_list.filenames[wav_file_list.current_index]);
}

bool player_is_finished(void)
{
    return is_finished;
}

void audio_i2s_half_transfer_callback(void)
{
    player_state = PLAYER_STATE_HALF_BUFFER;
}

void audio_i2s_full_transfer_callback(void)
{
    player_state = PLAYER_STATE_FULL_BUFFER;
}
