/*
 * wav_player.c
 *
 *  Created on: 17 Apr 2020
 *      Author: Mohamed Yaqoob
 */

#include "wav_player.h"
#include "audioI2S.h"
#include "fatfs.h"

//WAV File System variables
static FIL wavFile;
WAV_FileList wavFiles = {.count = 0, .currentIndex = 0};

//WAV Audio Buffer
static uint32_t fileLength;
#define AUDIO_BUFFER_SIZE  4096*1
static uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
static __IO uint32_t audioRemainSize = 0;

//WAV Player
static uint32_t samplingFreq;
static UINT playerReadBytes = 0;
static bool isFinished=0;
//WAV Player process states
typedef enum
{
  PLAYER_CONTROL_Idle=0,
  PLAYER_CONTROL_HalfBuffer,
  PLAYER_CONTROL_FullBuffer,
  PLAYER_CONTROL_EndOfFile,
}PLAYER_CONTROL_e;
volatile PLAYER_CONTROL_e playerControlSM = PLAYER_CONTROL_Idle;

static void wavPlayer_reset(void)
{
  audioRemainSize = 0;
  playerReadBytes = 0;
}

static bool isWavFile(const char* filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    return (strcasecmp(filename + len - 4, ".wav") == 0);
}

bool wavPlayer_scanUSB(void) {
    DIR dir;
    FILINFO fno;
    FRESULT fr;

    // Reset the file count
    wavFiles.count = 0;
    wavFiles.currentIndex = 0;

    // Open root directory
    fr = f_opendir(&dir, "/");
    if (fr != FR_OK) {
        return false;
    }

    // Read directory contents
    while (1) {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0) {
            break;  // End of directory or error
        }

        // Skip directories
        if (fno.fattrib & AM_DIR) {
            continue;
        }

        // Check if it's a WAV file
        if (isWavFile(fno.fname)) {
            if (wavFiles.count < MAX_WAV_FILES) {
                strncpy(wavFiles.filenames[wavFiles.count], fno.fname, MAX_FILENAME_LENGTH - 1);
                wavFiles.filenames[wavFiles.count][MAX_FILENAME_LENGTH - 1] = '\0';
                wavFiles.count++;
            }
        }
    }

    f_closedir(&dir);
    return (wavFiles.count > 0);
}

/**
 * @brief Select WAV file to play
 * @retval returns true when file is found in USB Drive
 */
bool wavPlayer_fileSelect(const char* filePath)
{
  WAV_HeaderTypeDef wavHeader;
  UINT readBytes = 0;

  f_close(&wavFile);

  //Open WAV file
  if(f_open(&wavFile, filePath, FA_READ) != FR_OK)
  {
    return false;
  }
  //Read WAV file Header
  f_read(&wavFile, &wavHeader, sizeof(wavHeader), &readBytes);
  //Get audio data size
  fileLength = wavHeader.FileSize;
  //Play the WAV file with frequency specified in header
  samplingFreq = wavHeader.SampleRate;
  return true;
}

/**
 * @brief WAV File Play
 */
void wavPlayer_play(void)
{
  isFinished = false;
  //Initialise I2S Audio Sampling settings
  audioI2S_init(samplingFreq);
  //Read Audio data from USB Disk
  f_lseek(&wavFile, 0);
  f_read (&wavFile, &audioBuffer[0], AUDIO_BUFFER_SIZE, &playerReadBytes);
  audioRemainSize = fileLength - playerReadBytes;
  //Start playing the WAV
  audioI2S_play((uint16_t *)&audioBuffer[0], AUDIO_BUFFER_SIZE);
}

/**
 * @brief Process WAV
 */
void wavPlayer_process(void)
{
  switch(playerControlSM)
  {
  case PLAYER_CONTROL_Idle:
    break;

  case PLAYER_CONTROL_HalfBuffer:
    playerReadBytes = 0;
    playerControlSM = PLAYER_CONTROL_Idle;
    f_read (&wavFile, &audioBuffer[0], AUDIO_BUFFER_SIZE/2, &playerReadBytes);
    if(audioRemainSize > (AUDIO_BUFFER_SIZE / 2))
    {
      audioRemainSize -= playerReadBytes;
    }
    else
    {
      audioRemainSize = 0;
      playerControlSM = PLAYER_CONTROL_EndOfFile;
    }
    break;

  case PLAYER_CONTROL_FullBuffer:
    playerReadBytes = 0;
    playerControlSM = PLAYER_CONTROL_Idle;
    f_read (&wavFile, &audioBuffer[AUDIO_BUFFER_SIZE/2], AUDIO_BUFFER_SIZE/2, &playerReadBytes);
    if(audioRemainSize > (AUDIO_BUFFER_SIZE / 2))
    {
      audioRemainSize -= playerReadBytes;
    }
    else
    {
      audioRemainSize = 0;
      playerControlSM = PLAYER_CONTROL_EndOfFile;
    }
    break;

  case PLAYER_CONTROL_EndOfFile:
    f_close(&wavFile);
    wavPlayer_reset();
    isFinished = true;
    playerControlSM = PLAYER_CONTROL_Idle;
    break;
  }
}

/**
 * @brief WAV stop
 */
void wavPlayer_stop(void)
{
  audioI2S_stop();
  isFinished = true;
}

/**
 * @brief WAV pause/resume
 */
void wavPlayer_pause(void)
{
  audioI2S_pause();
}
void wavPlayer_resume(void)
{
  audioI2S_resume();
}

bool wavPlayer_nextTrack(void) {
    if (wavFiles.count == 0) {
        if (!wavPlayer_scanUSB()) {
            return false;
        }
    }

    // Stop current playback
    audioI2S_stop();

    // Move to next track with wrapping
    wavFiles.currentIndex = (wavFiles.currentIndex + 1) % wavFiles.count;

    // Select and play the new track
    return wavPlayer_fileSelect(wavFiles.filenames[wavFiles.currentIndex]);
}

bool wavPlayer_previousTrack(void) {
    if (wavFiles.count == 0) {
        if (!wavPlayer_scanUSB()) {
            return false;
        }
    }

    // Stop current playback
    audioI2S_stop();

    // Move to previous track with wrapping
    wavFiles.currentIndex = (wavFiles.currentIndex - 1 + wavFiles.count) % wavFiles.count;


    // Select and play the new track
    return wavPlayer_fileSelect(wavFiles.filenames[wavFiles.currentIndex]);
}

/**
 * @brief isEndofFile reached
 */
bool wavPlayer_isFinished(void)
{
  return isFinished;
}

/**
 * @brief Half/Full transfer Audio callback for buffer management
 */
void audioI2S_halfTransfer_Callback(void)
{
  playerControlSM = PLAYER_CONTROL_HalfBuffer;
}
void audioI2S_fullTransfer_Callback(void)
{
  playerControlSM = PLAYER_CONTROL_FullBuffer;
//  audioI2S_changeBuffer((uint16_t*)&audioBuffer[0], AUDIO_BUFFER_SIZE / 2);
}

