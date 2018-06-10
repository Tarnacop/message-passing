/**
 * Created by Alexandru Blinda.
 */
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "file_handle_block_threaded.h"

#define BUFFER_SIZE 256

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /** LOCK */

/** Free liste of thread control */
void free_list(thread_control_list_head_t* head, pthread_mutex_t* list_lock) {

    pthread_mutex_lock(list_lock);
    /** If there is nothing to free, return */
    if(head->next == NULL) {

        pthread_mutex_unlock(list_lock);
        return;
    }
    thread_control_list_t* aux = head->next;
    thread_control_list_t* aux2;
    while(aux->next != NULL) {

        aux2 = aux->next;
        free(aux);
        aux = aux2;
    }
    head->next = NULL;
    free(aux);
    pthread_mutex_unlock(list_lock);
}

int add_thread(thread_control_list_head_t* head, pthread_t thread, pthread_mutex_t* list_lock) {

    pthread_mutex_lock(list_lock);
    thread_control_list_t* aux = head->next;
    /** When aux is NULL, stop */
    while(aux != NULL) {

        aux = aux->next;
    }
    /** Try and allocate space for a new list element */
    aux = malloc(sizeof(thread_control_list_t));
    /** If we did not succeed, return -1 */
    if(aux == NULL) {

        pthread_mutex_unlock(list_lock);
        return -1;
    }
    aux->thread = thread;
    aux->next = NULL;
    pthread_mutex_unlock(list_lock);
    return 0;
}

void remove_thread(thread_control_list_head_t* head, pthread_t thread, pthread_mutex_t* list_lock) {

    pthread_mutex_lock(list_lock);
    /** If there is nothing to check, return */
    if(head->next == NULL) {

        pthread_mutex_unlock(list_lock);
        return;
    }
    /** If the first element is the one we are looking for, free it and point the head to the next */
    thread_control_list_t* aux = head->next->next;
    if(head->next->thread == thread) {

        free(head->next);
        head->next = aux;
        pthread_mutex_unlock(list_lock);
        return;
    }
    thread_control_list_t* aux2 = head->next;
    while(aux->next != NULL) {

        if(aux->thread == thread) {

            aux2->next = aux->next;
            free(aux);
            pthread_mutex_unlock(list_lock);
            return;
        }
        aux=aux->next;
        aux2=aux2->next;
    }
    pthread_mutex_unlock(list_lock);
}

void cancel_threads(thread_control_list_head_t* head, pthread_mutex_t* list_lock) {

    pthread_mutex_lock(list_lock);
    if(head->next == NULL) {

        pthread_mutex_unlock(list_lock);
        return;
    }

    thread_control_list_t* aux = head->next;
    thread_control_list_t* aux2 = aux;
    while(aux != NULL) {

        if(pthread_cancel(aux->thread) != 0) {

            pthread_kill(aux->thread, SIGKILL);
        }
        aux = aux ->next;
        free(aux2);
        aux2 = aux;
    }
    head->next = NULL;
    pthread_mutex_lock(list_lock);
}

static void cleanup_handler(void *arg) {

    thread_control_block_t* tcb_pointer = (thread_control_block_t *) arg;

    close(tcb_pointer->client);
    free(tcb_pointer->name);
    free(tcb_pointer);
}

static void *client_thread(void *data) {

    thread_control_block_t *tcb_pointer = (thread_control_block_t *) data;

    pthread_cleanup_push(cleanup_handler, (void *) tcb_pointer);
    /** Client socket */
    char buffer[BUFFER_SIZE];
    memset(&buffer, '\0', sizeof(buffer));

    /** Flag to check if a message ends with newline; if it does not, we add it */
    short nl_flag = 0;
    int buffer_index = 0;
    short ln_flag = 1; /** Line Number flag */
    while (read(tcb_pointer->client, &buffer[buffer_index], 1) > 0) {

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

            pthread_mutex_lock(&lock); /** Lock because we access shared resources */
            if (tcb_pointer->log_file->file != NULL) {

                if(ln_flag == 1) {

                    if ((fprintf(tcb_pointer->log_file->file, "%d %s", tcb_pointer->log_file->line_number, buffer)) <
                        0) {

                        fprintf(stderr, "%s: ERROR! Could not write to file: %s\n", tcb_pointer->name,
                                strerror(ferror(tcb_pointer->log_file->file)));
                    } else {

                        ln_flag = 0;
                    }
                } else {

                    if ((fprintf(tcb_pointer->log_file->file, "%s", buffer)) < 0) {

                        fprintf(stderr, "%s: ERROR! Could not write to file: %s\n", tcb_pointer->name,
                                strerror(ferror(tcb_pointer->log_file->file)));
                    }
                }

                if (buffer[buffer_index] == '\n') {

                    tcb_pointer->log_file->line_number++;
                    ln_flag = 1;
                }

                if (fflush(tcb_pointer->log_file->file) < 0) {

                    fprintf(stderr, "%s: ERROR! Could not flush the file: %s\n", tcb_pointer->name,
                            strerror(ferror(tcb_pointer->log_file->file)));
                }
            }
            pthread_mutex_unlock(&lock); /** Unlock because we have finished with shared resources */
            memset(&buffer, '\0', sizeof(buffer));
            buffer_index = -1; /** When exiting the if, you get buffer_len ++ = 0 */
        }
        buffer_index++;
    }
    if (nl_flag != 0) {

        pthread_mutex_lock(&lock); /** Lock because we access shared resources */
        if (fprintf(tcb_pointer->log_file->file, "%s", "\n") < 0) {

            fprintf(stderr, "%s: ERROR! Could not write to file: %s\n", tcb_pointer->name,
                    strerror(ferror(tcb_pointer->log_file->file)));
        } else {

            tcb_pointer->log_file->line_number++;
        }
        if (fflush(tcb_pointer->log_file->file) < 0) {

            fprintf(stderr, "%s: ERROR! Could not flush the file: %s\n", tcb_pointer->name,
                    strerror(ferror(tcb_pointer->log_file->file)));
        }
        pthread_mutex_unlock(&lock); /** Unlock because we have finished with shared resources */
    }

    remove_thread(tcb_pointer->head, pthread_self(), tcb_pointer->list_lock);
    pthread_cleanup_pop(1);
    pthread_exit(EXIT_SUCCESS);
}

void accept_listen_socket(const int s, file_handle_block_t* log_file,
                          const char* name,
                          thread_control_list_head_t* head,
                          pthread_mutex_t* list_lock) {

    /** Wait for connections */
    printf("%s: Waiting for connections\n", name);

    while (1) {

        /**
        * Allocate space for a thread control block
        * Because it is malloc'd, it is thread safe
        */
        thread_control_block_t *tcb_pointer = malloc(sizeof(thread_control_block_t));

        if (tcb_pointer == NULL) {

            fprintf(stderr, "%s: ERROR! Could not allocate memory\n", name);
            /** If there is no more memory to allocate, just jump over the loop until some resources are available*/
            sleep(5); /** sleep a little so we do not overkill the CPU */
            continue;
        }

        tcb_pointer->client_address_size = sizeof(tcb_pointer->client_address);
        tcb_pointer->log_file = log_file;
        tcb_pointer->head = head;
        tcb_pointer->list_lock = list_lock;
        tcb_pointer->name = strdup(name);
        if (tcb_pointer->name == NULL) {

            fprintf(stderr, "%s: ERROR! Could not allocate memory\n", name);
            free(tcb_pointer);
            /** If there is no more memory to allocate, just jump over the loop until some resources are available*/
            sleep(5); /** sleep a little so we do not overkill the CPU */
            continue;
        }

        tcb_pointer->client = accept(s,
                                     (struct sockaddr *) &(tcb_pointer->client_address),
                                     &(tcb_pointer->client_address_size));

        if (tcb_pointer->client >= 0) {

            pthread_t thread;
            pthread_attr_t pthread_attr; /** attributes for newly create thread */

            if (pthread_attr_init(&pthread_attr) != 0) {

                fprintf(stderr, "%s: ERROR! Could not create initial thread attributes\n", name);
                close(tcb_pointer->client);
                free(tcb_pointer->name);
                free(tcb_pointer);
                sleep(5); /** sleep a little so we do not overkill the CPU */
                continue; /** We do not want to kill the server. Disconnect the client, free the resources and retry */
            }

            if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {

                fprintf(stderr, "%s: ERROR! Setting thread attributes failed\n", name);
                close(tcb_pointer->client);
                free(tcb_pointer->name);
                free(tcb_pointer);
                sleep(5); /** sleep a little so we do not overkill the CPU */
                continue; /** We do not want to kill the server. Disconnect the client, free the resources and retry */
            }

            if (pthread_create(&thread, &pthread_attr, &client_thread, (void *) tcb_pointer) != 0) {

                fprintf(stderr, "%s: ERROR! Could not create thread\n", name);
                close(tcb_pointer->client);
                free(tcb_pointer->name);
                free(tcb_pointer);
                sleep(5); /** sleep a little so we do not overkill the CPU */
                continue; /** We do not want to kill the server. Disconnect the client, free the resources and retry */
            }

            if (add_thread(head, thread, list_lock) < 0) {

                fprintf(stderr, "%s: ERROR! Could not create thread control\n", name);
                free_list(head, list_lock);
                close(tcb_pointer->client);
                free(tcb_pointer->name);
                free(tcb_pointer);
                sleep(5); /** sleep a little so we do not overkill the CPU */
                continue; /** We do not want to kill the server. Disconnect the client, free the resources and retry */
            }

        } else {

            close(tcb_pointer->client);
            free(tcb_pointer->name);
            free(tcb_pointer);
        }
    }
}

