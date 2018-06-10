/**
 * Created by Alexandru Blinda.
 */
#include <sys/errno.h>
#include <string.h>
#include "file_handle_block_threaded.h"

void open_log_file(file_handle_block_t* log_file, const char* filename, const char* name) {

    /** Try opening file */
    log_file->file = fopen(filename, "a+");
    log_file->line_number = 0;
    if(log_file->file == NULL) {

        fprintf(stderr, "%s: ERROR! Could not open file: %s\n", name, strerror(errno));
        /** DO NOT NEED TO EXIT_FAILURE */
    }

    if(log_file->file != NULL) {

        rewind(log_file->file);
        int ch = 0;
        /** Take the number of lines */
        do {
            ch = fgetc(log_file->file);
            if(ch == '\n') {

                log_file->line_number++;
            }
        }while(ch != EOF);
    }
}