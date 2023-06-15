#include "syslog.h"
#include <stdio.h>

int main (int argc, char *argv[]) {
    // argc = ?
    // argv[0] = ?
    // argv[1] = ?

    // printf("Arguments are %s, %s, %s\n", argv[0], argv[1], argv[2]);
    // Exit if arguments aren't supplied
    // if (argc != 3) {
    //     printf("Incorrect arguments given: argc=%d\n", argc);
    //     return 1;
    // }

    // Open logger
    openlog(NULL,0,LOG_USER);
    syslog(LOG_DEBUG, "Start aesdsocket.c");

// TODO: b. Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.


// TODO: c. Listens for and accepts a connection


// TODO: e. Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
/*
Your implementation should use a newline to separate data packets received.  In other words a packet is considered complete when a newline character is found in the input receive stream, and each newline should result in an append to the /var/tmp/aesdsocketdata file.
You may assume the data stream does not include null characters (therefore can be processed using string handling functions).
You may assume the length of the packet will be shorter than the available heap size.  In other words, as long as you handle malloc() associated failures with error messages you may discard associated over-length packets.
*/


// TODO: f. Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
/*
You may assume the total size of all packets sent (and therefore size of /var/tmp/aesdsocketdata) will be less than the size of the root filesystem, however you may not assume this total size of all packets sent will be less than the size of the available RAM for the process heap.
*/


// TODO: g. Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.


// TODO: h. Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received (see below).


// TODO: i. Gracefully exits when SIGINT or SIGTERM is received, completing any open connection operations, closing any open sockets, and deleting the file /var/tmp/aesdsocketdata.
/*
Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received.
*/


}