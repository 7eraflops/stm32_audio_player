#ifndef FILES_H_
#define FILES_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <fatfs.h>
#include <stdbool.h>

#define MAX_FILES 32
#define MAX_FILENAME_LENGTH 256

    typedef struct
    {
        char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
        uint16_t count;
        uint16_t current_index;
    } FILE_LIST;

    bool is_wav_file(const char *file_name);

    bool is_flac_file(const char *file_name);

    bool scan_usb(const char *file_type);

#ifdef __cplusplus
}
#endif

#endif /* FILES_H_ */
