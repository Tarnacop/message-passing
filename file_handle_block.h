#include <stdio.h>

/**
 * Create by Alexandru Blinda.
 */
typedef struct file_handle_block {

    FILE *file;
    int line_number;
} file_handle_block_t;

void open_log_file(file_handle_block_t* log_file, const char* filename, const char *name);
void resolve_listen_socket(const int s, file_handle_block_t* log_file, const char* name);