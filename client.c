/**
 * Created by Alexandru Blinda.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <signal.h>

#define BUFFER_SIZE 256

/** FUNCTION DEFINITION */

void sig_handler(int);

/** GLOBAL VARIABLES - VARIABLES THAT MUST BE CLOSED / FREED ETC */

int s = -1; // SOCKET -1 MEANING NOT OPENED
struct addrinfo *result = NULL; // HOLDS addrinfo RESULTS

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

    if(signal(SIGPIPE, sig_handler) == SIG_ERR) {

        fprintf(stderr, "ERROR HANDLING SIGTERM\n");
        exit(EXIT_FAILURE);
    }

    /** Check if there are enough arguments */
    if(argc != 3) {

        fprintf(stderr, "%s: Usage %s hostname port\n", argv[0], argv[0]);
        exit(EXIT_FAILURE);
    }

    char *end_ptr;

    int port_number = strtol(argv[2], &end_ptr, 10);
    /** Check if the port number is valid */
    if(port_number < 0 || port_number > 65535 || *end_ptr != '\0') {

        fprintf(stderr, "%s: ERROR! Invalid port number %s: %s\n", argv[0], argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    /** -------------------------- CREATING SOCKET THAT WORKS WITH IPv4 AND IPv6 ------------------------- */

    struct addrinfo hints;
    struct addrinfo *rp = NULL;

    memset(&hints, '\0', sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /** IPv4 OR IPv6 */
    hints.ai_socktype = SOCK_STREAM; /** TCP */

    int res = getaddrinfo(argv[1], argv[2], &hints, &result);

    if(res != 0) {

        fprintf(stderr, "%s: ERROR! Cannot resolve %s: %s (%s)\n",
                argv[0], argv[1], argv[2], gai_strerror(res));
        exit(EXIT_FAILURE);
    }

    /**
     * Try each address until we successfuly connect
     * If we do not connect, close the socket and try the next addresss
     */
    for(rp = result; rp != NULL; rp=rp->ai_next) {

        if((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) >= 0) {

            if((connect(s, rp->ai_addr, rp->ai_addrlen)) == 0) {

                break; /** Success, break the loop */
            }
        }
        close(s); /** Did not succeed close the socket and try the next one */
    }

    if(rp == NULL) {

        fprintf(stderr, "%s: ERROR! Could not connect\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result); /** No longer needed */
    result = NULL; /** For testing purposes */

    /** ------------------------------- READING USER INPUT -------------------------------- */

    printf("%s: Input message to the server\n", argv[0]);

    char buffer[BUFFER_SIZE];     /** Buffer to hold data read from stdin */
    memset(buffer, '\0', BUFFER_SIZE); /** Make sure it has null bytes */
    int character_index = 0; /** Index for character read */
    size_t bytes_written; /** Number of bytes written to server for error check */
    int character_int; /** getchar() returns the int char code and we store it here */
    short nl_flag = 0; /** Int to flag if we need a nl or not to mark end of message */
    /** Get the first char and check if it is an EOF */
    while((character_int=getchar()) != EOF){

        /**
         * Input sanitisation: If there is a null byte or an invalid char int code
         * Do not take it into account and redo the loop with the next character
         */
        if(character_int < 1 || character_int > 127) {

            continue;
        }
        buffer[character_index] = (char) character_int; /** If it is not EOF, convert it to char */

        nl_flag = buffer[character_index] != '\n';
        /** If we have filled the buffer or we have read a new line */
        if(character_index == (BUFFER_SIZE - 2) || !nl_flag ) {

            /** Send the buffer to the server and a null byte at the end */
            bytes_written = write(s, buffer, character_index + 2);
            /** If we cannot send the data, print an error and exit failure */
            if(bytes_written < 0) {

                fprintf(stderr, "%s: ERROR! Could not send message to server: %s\n", argv[0], strerror(errno));
                exit(EXIT_FAILURE);
            }
            /** Set the count to -1 because it will be incremented and reset to 0 */
            character_index = -1;
            /** Reset the buffer to null byte */
            memset(buffer, '\0', BUFFER_SIZE);
        }

        /** Increment the count since we are going to read a new character */
        character_index = character_index + 1;
    }

    /**
     * If EOF occurred, we need to make sure the last char is a newline that marks the end of the last message
     * if we have not already sent a newline
     */
    if(nl_flag != 0) {

        if(sizeof(buffer) == 0) {

            buffer[0] = '\n';
        } else {

            if (buffer[character_index - 1] != '\n') {

                buffer[character_index] = '\n';
            }
        }
    }

    /** Send the rest of the buffer to the server, only if the buffer contains any data */
    if(strlen(buffer) > 0) {

        bytes_written = write(s, buffer, character_index + 1);

        /** If we cannot send the data, print an error and exit failure */
        if(bytes_written < 0) {

            fprintf(stderr, "%s: ERROR! Could not send message to server: %s\n", argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    /** Close socket */
    close(s);
    s = -1;

    exit(EXIT_SUCCESS);
} // END OF client FUNCTION

/** FUNCTION DECLARATION */

void sig_handler(int signo) {

    if(signo == SIGINT) {

        if(s >= 0) {

            close(s);
        }

        if(result != NULL) {

            freeaddrinfo(result);
        }

        _exit(EXIT_SUCCESS);
    }

    if(signo == SIGTERM) {

        if(s >= 0) {

            close(s);
        }

        if(result != NULL) {

            freeaddrinfo(result);
        }

        _exit(EXIT_SUCCESS);
    }

    if(signo == SIGPIPE) {

        if(s >= 0) {

            close(s);
        }

        if(result != NULL) {

            freeaddrinfo(result);
        }

        fprintf(stderr, "Server shut down: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }

} // END OF sig_handler FUNCTION