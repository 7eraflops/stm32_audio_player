#include "files.h"

FILE_LIST wav_file_list __attribute__((section(".ccmram"))) = {.count = 0, .current_index = 0};
FILE_LIST flac_file_list __attribute__((section(".ccmram"))) = {.count = 0, .current_index = 0};

FIL wav_file;
FIL flac_file;

bool is_wav_file(const char *file_name)
{
    size_t len = strlen(file_name);
    if (len < 4)
        return false;
    return (strcasecmp(file_name + len - 4, ".wav") == 0);
}

bool is_flac_file(const char *file_name)
{
    size_t len = strlen(file_name);
    if (len < 5)
        return false;
    return (strcasecmp(file_name + len - 5, ".flac") == 0);
}

bool scan_usb(const char *file_type)
{
    DIR dir;
    FILINFO fno;
    FRESULT fr;

    FILE_LIST *current_list;
    bool (*is_target_file)(const char *);

    if (strcasecmp(file_type, "wav") == 0)
    {
        current_list = &wav_file_list;
        is_target_file = is_wav_file;
    }
    else if (strcasecmp(file_type, "flac") == 0)
    {
        current_list = &flac_file_list;
        is_target_file = is_flac_file;
    }
    else
    {
        return false;
    }

    current_list->count = 0;
    current_list->current_index = 0;

    fr = f_opendir(&dir, "/");
    if (fr != FR_OK)
    {
        return false;
    }

    while (1)
    {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
        {
            break;
        }

        if (fno.fattrib & AM_DIR)
        {
            continue;
        }

        if (is_target_file(fno.fname))
        {
            if (current_list->count < MAX_FILES)
            {
                strncpy(current_list->filenames[current_list->count],
                        fno.fname,
                        MAX_FILENAME_LENGTH - 1);
                current_list->filenames[current_list->count][MAX_FILENAME_LENGTH - 1] = '\0';
                current_list->count++;
            }
        }
    }

    f_closedir(&dir);
    return (current_list->count > 0);
}
