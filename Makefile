# Examine me if you wish, but we will learn about Makefiles in a later
# lecture.  This file allows us to automate tedious repetitive build
# commands
CFLAGS = -D_POSIX_SOURCE -Wall -Werror -pedantic -std=c99 -D_GNU_SOURCE -pthread
GCC = gcc $(CFLAGS)


APPS = client serverSingle serverThreaded

all: $(APPS)

client: client.c
	$(GCC) -o client client.c

serverSingle: serverSingle.c
	$(GCC) -o serverSingle serverSingle.c file_handle_block.h get_listen_socket.h get_listen_socket.c open_log_file.c resolve_listen_socket.c

serverThreaded: serverThreaded.c
	$(GCC) -o serverThreaded serverThreaded.c file_handle_block_threaded.h get_listen_socket_threaded.h get_listen_socket_threaded.c open_log_file_threaded.c accept_listen_socket_threaded.c

clean:
	rm -f $(APPS) 
