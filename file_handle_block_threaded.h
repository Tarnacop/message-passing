#include <stdio.h>
#include <netinet/in.h>
#include <pthread.h>

/**
 * Create by Alexandru Blinda.
 */
typedef struct file_handle_block_threaded {

    FILE *file;
    int line_number;
} file_handle_block_t;

typedef struct thread_control_list {

    pthread_t thread;
    struct thread_control_list* next;
} thread_control_list_t;

typedef struct thread_control_list_head {

    thread_control_list_t* next;
} thread_control_list_head_t;

/**
 * Thread_control_block struct will hold the data that is thread related
 */
typedef struct thread_control_block {

    /**
    * Declaring variables
    * client holds the file descriptor of the accepted socket or -1 in case of error
    * client_address holds the client address
    * client_address_size holds the size of the client address
    */
    int client;
    struct sockaddr_in6 client_address;
    socklen_t client_address_size;
    /** Required to write to file */
    file_handle_block_t* log_file;
    /** The name of the file used for printing */
    char* name;
    thread_control_list_head_t* head;
    pthread_mutex_t* list_lock;
} thread_control_block_t;

void open_log_file(file_handle_block_t*, const char*, const char*);
void accept_listen_socket(const int, file_handle_block_t*, const char*, thread_control_list_head_t* head,
                          pthread_mutex_t* list_lock);
void free_list(thread_control_list_head_t* head, pthread_mutex_t* list_lock);
int add_thread(thread_control_list_head_t* head, pthread_t thread, pthread_mutex_t* list_lock);
void remove_thread(thread_control_list_head_t* head, pthread_t thread, pthread_mutex_t* list_lock);
void cancel_threads(thread_control_list_head_t* head, pthread_mutex_t* list_lock);