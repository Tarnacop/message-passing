/**
 * Created by Alexandru Blinda.
 */
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "file_handle_block.h"

#define BUFFER_SIZE 256

void resolve_listen_socket(const int s, file_handle_block_t* log_file, const char* name) {

    /** Client socket */
    int client;
    struct sockaddr_in6 client_address;
    memset(&client_address, '\0', sizeof(struct sockaddr_in6));
    socklen_t address_length = sizeof(client_address);
    char buffer[BUFFER_SIZE];
    memset(&buffer, '\0', sizeof(buffer));

    /** Wait for connections */
    printf("%s: Waiting for connections\n", name);
    short nl_flag; /** flag for new line */
    short ln_flag; /** flag for line number */
    int buffer_index;

    while(1) {

        client = accept(s, (struct sockaddr *) &client_address, &address_length);
        /** Flag to check if a message ends with newline; if it does not, we add it */
        nl_flag = 0;
        ln_flag = 1;
        buffer_index = 0;
        if (client >= 0) {

            /** Read the stream byte by byte so we can have better control over it */
            while (read(client, &buffer[buffer_index], 1) > 0) {

                /** If we read a non-printable character (exception being NL) or a
                 * non ascii character (> 127) then just jump over it
                 */
                if(buffer[buffer_index] < 10 ||
                        ((buffer[buffer_index] > 10 ) && (buffer[buffer_index] < 32)) ||
                        (buffer[buffer_index] > 127)){
                    continue;
                }

                nl_flag = buffer[buffer_index] != '\n';
                /** Check if we have reached the maximum capacity of the buffer or we have a newline */
                if (buffer_index == BUFFER_SIZE - 2 || buffer[buffer_index] == '\n') {

                    if (log_file->file != NULL) {

                        if(ln_flag == 1) {

                            if ((fprintf(log_file->file, "%d %s", log_file->line_number, buffer)) < 0) {

                                fprintf(stderr, "%s: ERROR! Could not write to file: %s\n", name,
                                        strerror(ferror(log_file->file)));
                            } else {

                                ln_flag = 0;
                            }
                        } else {

                            if ((fprintf(log_file->file, "%s", buffer)) < 0) {

                                fprintf(stderr, "%s: ERROR! Could not write to file: %s\n", name,
                                        strerror(ferror(log_file->file)));
                            }
                        }

                        if (buffer[buffer_index] == '\n') {

                            log_file->line_number++;
                            ln_flag = 1;
                        }

                        if (fflush(log_file->file) < 0) {

                            fprintf(stderr, "%s: ERROR! Could not flush the file: %s\n", name,
                                    strerror(ferror(log_file->file)));
                        }
                    }
                    memset(&buffer, '\0', sizeof(buffer));
                    buffer_index = -1; /** When exiting the if, you get buffer_len ++ = 0 */
                }
                buffer_index++;
            }
            if (nl_flag != 0) {

                if (fprintf(log_file->file, "%s", "\n") < 0) {

                    fprintf(stderr, "%s: ERROR! Could not write to file: %s\n", name,
                            strerror(ferror(log_file->file)));
                } else {

                    log_file->line_number++;
                }

                if (fflush(log_file->file) < 0) {

                    fprintf(stderr, "%s: ERROR! Could not flush the file: %s\n", name,
                            strerror(ferror(log_file->file)));
                }
            }
        }
        if (client < 0) {

            close(client);
        }
    }
}

