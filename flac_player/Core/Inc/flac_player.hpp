#ifndef FLAC_PLAYER_H_
#define FLAC_PLAYER_H_

#include <stdbool.h>
#include <stdint.h>
#define MAX_FILES 16
#define MAX_FILENAME_LENGTH 64

// FLAC file list structure
typedef struct {
    char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
    uint16_t count;
    uint16_t current_index;
} FLAC_file_list;

// Function declarations for FLAC player
bool flac_player_scan_USB(void);
bool flac_player_file_select(const char* file_path);
void flac_player_play(void);
void flac_player_stop(void);
void flac_player_pause(void);
void flac_player_resume(void);
void flac_player_process(void);
bool flac_player_next_track(void);
bool flac_player_previous_track(void);
bool flac_player_is_finished(void);

#endif // FLAC_PLAYER_H_
