/*
 * flac_player.cpp
 *
 *  Updated to support FLAC playback using the provided FLAC decoding class.
 */

#include "flac_player.hpp"
#include "Flac.hpp"
#include "audioI2S.h"
#include "fatfs.h"
#include <vector>

// FLAC File System variables
static FIL flac_file;
FLAC_file_list flac_files = {.count = 0, .current_index = 0};

// FLAC Audio Buffer
static std::vector<buffer_sample_type> audio_buffer;
static __IO uint32_t samples_remaining = 0;
static size_t max_frame_size = 0;
static size_t current_frame_size = 0;

// FLAC Player state variables
static bool is_finished = false;
static Flac flac_decoder(flac_file);
static bool audio_moved = false;

// FLAC Player process states
typedef enum
{
    PLAYER_CONTROL_Idle = 0,
    PLAYER_CONTROL_HalfBuffer,
    PLAYER_CONTROL_FullBuffer,
    PLAYER_CONTROL_EndOfFile,
} PLAYER_CONTROL_e;
volatile PLAYER_CONTROL_e player_control_sm = PLAYER_CONTROL_Idle;

void flac_player_reset(void)
{
    samples_remaining = 0;
    current_frame_size = 0;
    max_frame_size = 0;
}

static bool is_flac_file(const char *filename)
{
    size_t len = strlen(filename);
    if (len < 5)
        return false;
    return (strcasecmp(filename + len - 5, ".flac") == 0);
}

bool flac_player_scan_USB(void)
{
    DIR dir;
    FILINFO fno;
    FRESULT fr;

    // Reset the file count
    flac_files.count = 0;
    flac_files.current_index = 0;

    // Open root directory
    fr = f_opendir(&dir, "/");
    if (fr != FR_OK)
    {
        return false;
    }

    // Read directory contents
    while (true)
    {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
        {
            break; // End of directory or error
        }

        // Skip directories
        if (fno.fattrib & AM_DIR)
        {
            continue;
        }

        // Check if it's a FLAC file
        if (is_flac_file(fno.fname))
        {
            if (flac_files.count < MAX_FILES)
            {
                strncpy(flac_files.filenames[flac_files.count], fno.fname, MAX_FILENAME_LENGTH - 1);
                flac_files.filenames[flac_files.count][MAX_FILENAME_LENGTH - 1] = '\0';
                flac_files.count++;
            }
        }
    }

    f_closedir(&dir);
    return (flac_files.count > 0);
}

bool flac_player_file_select(const char *file_path)
{
    f_close(&flac_file);

    // Open FLAC file
    if (f_open(&flac_file, file_path, FA_READ) != FR_OK)
    {
        return false;
    }

    flac_decoder.set_file(flac_file);
    flac_decoder.initialize();

    return true;
}

static size_t calculate_buffer_size() {
    // Get the maximum possible frame size for this file
    size_t samples_per_frame = flac_decoder.get_stream_info().max_block_size;
    size_t channels = flac_decoder.get_stream_info().channels;

    // Calculate total samples needed for double buffering
    // samples_per_frame * channels * 2 (for double buffering)
    return samples_per_frame * channels * 2;
}

void flac_player_play(void)
{
    is_finished = false;

    // Initialize I2S Audio with the correct sample rate
    audioI2S_init(flac_decoder.get_stream_info().sample_rate);

    // Calculate and resize the buffer based on the FLAC file properties
    size_t required_buffer_size = calculate_buffer_size();
    audio_buffer.resize(required_buffer_size);

    // Fill the first half of the buffer
    if (flac_decoder.decode_frame()) {
        is_finished = true;
        return;
    }

    const auto &first_frame = flac_decoder.get_audio_buffer();
    current_frame_size = first_frame.size();
    memcpy(audio_buffer.data(), first_frame.data(), current_frame_size * sizeof(buffer_sample_type));

    // Fill the second half of the buffer
    if (flac_decoder.decode_frame()) {
        is_finished = true;
        return;
    }

    const auto &second_frame = flac_decoder.get_audio_buffer();
    current_frame_size = second_frame.size();
    memcpy(audio_buffer.data() + current_frame_size, second_frame.data(), current_frame_size * sizeof(buffer_sample_type));

    // Calculate remaining samples (in terms of individual audio samples)
    samples_remaining = flac_decoder.get_stream_info().total_samples - first_frame.size() - second_frame.size();

    audio_moved = true;
    // Start playback with the maximum possible buffer size
    audioI2S_play((uint16_t *)audio_buffer.data(), required_buffer_size);
}

void flac_player_process(void)
{
    switch (player_control_sm)
    {
    case PLAYER_CONTROL_Idle:
    	/*
    	if (audio_moved)
    	{
    		if (flac_decoder.decode_frame())
    		{
    		                player_control_sm = PLAYER_CONTROL_EndOfFile;
    		                break;
    		            }
    		audio_moved = false;
    	}
    	*/
        break;

    case PLAYER_CONTROL_HalfBuffer:
        {
            player_control_sm = PLAYER_CONTROL_Idle;

            if (flac_decoder.decode_frame()) {
                player_control_sm = PLAYER_CONTROL_EndOfFile;
                break;
            }

            const auto &new_frame = flac_decoder.get_audio_buffer();
            current_frame_size = new_frame.size();

            // Always copy to the start of the buffer
            memcpy(audio_buffer.data(), new_frame.data(), current_frame_size * sizeof(buffer_sample_type));
            audio_moved = true;

            if (samples_remaining > flac_decoder.get_stream_info().max_block_size) {
                samples_remaining -= flac_decoder.get_stream_info().max_block_size;
            } else {
                player_control_sm = PLAYER_CONTROL_EndOfFile;
            }
        }
        break;

    case PLAYER_CONTROL_FullBuffer:
        {
            player_control_sm = PLAYER_CONTROL_Idle;

            if (flac_decoder.decode_frame()) {
                player_control_sm = PLAYER_CONTROL_EndOfFile;
                break;
            }

            const auto &new_frame = flac_decoder.get_audio_buffer();
            current_frame_size = new_frame.size();

            // Copy to the second half of the buffer
            memcpy(audio_buffer.data() + current_frame_size, new_frame.data(), current_frame_size * sizeof(buffer_sample_type));
            audio_moved = true;

            if (samples_remaining > flac_decoder.get_stream_info().max_block_size) {
                samples_remaining -= flac_decoder.get_stream_info().max_block_size;
            } else {
                player_control_sm = PLAYER_CONTROL_EndOfFile;
            }
        }
        break;

    case PLAYER_CONTROL_EndOfFile:
        f_close(&flac_file);
        flac_player_reset();
        is_finished = true;
        player_control_sm = PLAYER_CONTROL_Idle;
        break;
    }
}

void flac_player_stop(void)
{
    audioI2S_stop();
    is_finished = true;
}

void flac_player_pause(void)
{
    audioI2S_pause();
}

void flac_player_resume(void)
{
    audioI2S_resume();
}

bool flac_player_next_track(void)
{
    // Stop current playback
    audioI2S_stop();

    // Move to next track with wrapping
    flac_files.current_index = (flac_files.current_index + 1) % flac_files.count;

    // Select and play the new track
    return flac_player_file_select(flac_files.filenames[flac_files.current_index]);
}

bool flac_player_previous_track(void)
{
    // Stop current playback
    audioI2S_stop();

    // Move to previous track with wrapping
    flac_files.current_index = (flac_files.current_index - 1 + flac_files.count) % flac_files.count;

    // Select and play the new track
    return flac_player_file_select(flac_files.filenames[flac_files.current_index]);
}

bool flac_player_is_finished(void)
{
    return is_finished;
}

void audioI2S_halfTransfer_Callback(void)
{
    player_control_sm = PLAYER_CONTROL_HalfBuffer;
}

void audioI2S_fullTransfer_Callback(void)
{
    player_control_sm = PLAYER_CONTROL_FullBuffer;
}
