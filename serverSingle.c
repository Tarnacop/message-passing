/**
 * Created by Alexandru Blinda.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "file_handle_block.h"
#include "get_listen_socket.h"

#define BUFFER_SIZE 256
/**
 * Created by Alexandru Blinda.
 */

/** FUNCTION DEFINITION */

void sig_handler(int);

/** GLOBAL VARIABLES - VARIABLES THAT MUST BE CLOSED / FREED ETC */

file_handle_block_t log_file;
int s = -1; /** Will hold the socket details */

/** MAIN */

int main(int argc, char** argv) {

    /** --------------------- PREP STAGE --------------------------- */

    /** Add the handler to the main method */
    if (signal(SIGINT, sig_handler) == SIG_ERR) {

        fprintf(stderr, "ERROR HANDLING SIGINT\n");
        exit(EXIT_FAILURE);
    }

    if(signal(SIGTERM, sig_handler) == SIG_ERR) {

        fprintf(stderr, "ERROR HANDLING SIGTERM\n");
        exit(EXIT_FAILURE);
    }

    /** Check if there are enough arguments */
    if(argc != 3) {

        fprintf(stderr, "%s: Usage %s port filename", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

    char *end_ptr;

    int port_number = strtol(argv[1], &end_ptr, 10);
    /** Check if the port number is valid */
    if(port_number < 0 || port_number > 65535 || *end_ptr != '\0') {

        fprintf(stderr, "%s: ERROR! Invalid port number %s\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }

   open_log_file(&log_file, argv[2], argv[0]);

    /** -------------------------- CREATING SOCKET THAT WORKS WITH IPv4 AND IPv6 ------------------------- */

    s = get_listen_socket(port_number, argv[0]);
    if(s < 0) {

        exit(EXIT_FAILURE);
    }

    /** Client socket */

    resolve_listen_socket(s, &log_file, argv[0]);
} // END OF main FUNCTION

/** FUNCTION DECLARATION */

void sig_handler(int signo) {

    if (signo == SIGINT) {

        if(s >= 0) {

            close(s);
        }
        if(log_file.file != NULL) {

            fclose(log_file.file);
        }
        _exit(EXIT_SUCCESS);
    }

    if(signo == SIGTERM) {

        if(s >= 0) {

            close(s);
        }
        if(log_file.file != NULL) {

            fclose(log_file.file);
        }
        _exit(EXIT_SUCCESS);
    }
}