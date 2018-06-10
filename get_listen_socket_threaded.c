/**
 * Created by Alexandru Blinda.
 */
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

int get_listen_socket(const int port, const char* name) {

    /** Create a sockaddr_in6 that will hold the address details*/
    struct sockaddr_in6 address;
    int s;

    memset(&address, '\0', sizeof(struct sockaddr_in6));
    address.sin6_family = AF_INET6; /** Accept IPv6 */
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(port); /** Use our port number */

    /** Try and create socket. If it does not work, error */
    if((s=socket(PF_INET6, SOCK_STREAM, 0)) < 0) {

        fprintf(stderr, "%s: Could not create socket\n", name);
        return -1;
    }

    /** Try to bind the socket to a port */
    if((bind(s, (struct sockaddr *) &address, sizeof(address))) != 0) {

        fprintf(stderr, "%s: Could not bind to port\n", name);
        return -1;
    }

    /** Listen port */
    if(listen(s, 5) != 0) {

        fprintf(stderr, "%s: Could not listen socket\n", name);
        return -1;
    }

    return s;
}
