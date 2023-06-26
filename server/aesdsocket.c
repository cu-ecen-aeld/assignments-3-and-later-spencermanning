#include "syslog.h"
#include <stdio.h>
#include <stdlib.h>     // for exit()
#include <sys/types.h>  // for socket
#include <sys/socket.h> // for socket/shutdown
#include <netdb.h>      // for getaddrinfo/freeaddrinfo()/addrinfo struct
#include <string.h>     // for memset()
#include <signal.h>     // for sigaction/SIGINT/SIGTERM
#include <unistd.h>     // for read()
#include <errno.h>      // for errno

// const struct {
//     sa_family_t sa_family;
//     char        sa_data[14];
// } sockaddr;

// const struct {
//     int             ai_flags;
//     int             ai_family;
//     int             ai_socktype;
//     int             ai_protocol;
//     size_t          ai_addrlen;
//     struct sockaddr *ai_addr;
//     char            *ai_canonname;
//     struct addrinfo *ai_next;
// } addrinfo; // servinfo or res

#define MAX_BUF_LEN 20 // TODO: Could it be larger?

int received_exit_signal = 0;
struct addrinfo *servinfo; // res
int sockfd;

void close_all_things() {
    remove("/var/tmp/aesdsocketdata");
    shutdown(sockfd,SHUT_RDWR);
    close(sockfd);
    freeaddrinfo(servinfo); // Free the malloc'd space from getaddrinfo()
}

static void signal_handler(int signal_num) {
    if (signal_num == SIGINT) { // ctrl+c
        printf("***We got a SIGINT***\n");

        close_all_things();

        received_exit_signal = 1;
        
    }
    else if (signal_num == SIGTERM) {
        printf("***We got a SIGTERM***\n");

        close_all_things();

        received_exit_signal = 1;
    }
}

int main (int argc, char *argv[]) {
    // socklen_t socklen;

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
    syslog(LOG_DEBUG, "Start aesdsocket.c\n");
    printf("We have started the aesdsocket\n");

    syslog(LOG_NOTICE, "Anotherlog");

    // Set up new_action that points to the signal_handler function (vid3.10)
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler=signal_handler;
    sigaction(SIGTERM, &new_action, NULL);
    sigaction(SIGINT, &new_action, NULL);

    // b. Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        syslog(LOG_ERR, "Socket connection failed\n");
        return -1; 
    }

    // Get the socket address (sockaddr)
    struct addrinfo hints;
    // struct addrinfo *servinfo; // res
    memset(&hints, 0, sizeof(hints)); // start struct as zeros
    hints.ai_family = AF_UNSPEC; // don't care if either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int getaddrinfoout = getaddrinfo(NULL, "9000", &hints, &servinfo);
    if (getaddrinfoout == -1) {
        return -1;
    }

    // socklen_t addrlen = sizeof(servinfo->ai_addrlen);

    // Make the address able to e used again with SO_REUSEADDR
    int myoptval = 1;
    int sockoptout = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &myoptval, sizeof(myoptval));
    if (sockoptout == -1) {
        return -1;
    }

    int bindout  = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (bindout == -1) {
        return -1;
    }
    
    // FIXME: Does the daemon creation need to be placed here?


    // c. Listens for and accepts a connection
    // socklen_t addrlen = sizeof(addrinfo.ai_addr); // Should it be this way?
    int listenout = listen(sockfd, 20); // 20 from TA
    if (listenout == -1) {
        return -1;
    }

    struct sockaddr_in clientinfo;
    socklen_t client_addr_size = sizeof(clientinfo);

    // Start with a clean file
    remove("/var/tmp/aesdsocketdata");

    FILE *file;
    file = fopen("/var/tmp/aesdsocketdata", "w+");
    if (!file) {
        syslog(LOG_ERR, "File didn't open\n");
        printf("File didn't open\n");
        return -1;
    }

    char *line = NULL; // for getline()

    // h. Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received (see below).
    while (received_exit_signal == 0) {

        char ipaddr[INET_ADDRSTRLEN]; // IPv4 or IPv6??

        int acceptfd = accept(sockfd, (struct sockaddr*)&clientinfo, &client_addr_size); // TODO: Should servinfo->ai_addr be socklen_t socklen instead?
        if (acceptfd == -1) {
            return -1;
        }

        // 5. Modify your program to support a -d argument which runs the aesdsocket application as a daemon.
        // When in daemon mode the program should fork after ensuring it can bind to port 9000.
        if (argc == 2) {
            syslog(LOG_ERR, "Start daemon\n");
            // TODO: Fill in the daemon here

            if (strcmp(argv[1], "-d") == 0) {
                syslog(LOG_NOTICE, "About to fork the process");
                int pid = fork();
                if (pid == -1) {
                    syslog(LOG_ERR, "Fork failed");
                    return -1;
                }
                else if (pid > 0) { // parent
                    syslog(LOG_NOTICE, "In parent, exiting now");
                    closelog();
                    exit(0);
                }
                // Create the new session for the child process.
                // ie put it in the background as a daemon!
                setsid();

                // Change working directory to "/"
                chdir("/"); // Do we need this?
            }
        }
        
        // d. Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client.
        syslog(LOG_NOTICE, "Accepted connection from %s\n", servinfo->ai_addr->sa_data);

        // e. Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
        /*
        Your implementation should use a newline to separate data packets received.  
        In other words a packet is considered complete when a newline character is found in the input receive stream, 
        and each newline should result in an append to the /var/tmp/aesdsocketdata file.
        You may assume the data stream does not include null characters (therefore can be processed using string handling functions).
        You may assume the length of the packet will be shorter than the available heap size.  
        In other words, as long as you handle malloc() associated failures with error messages you may discard associated over-length packets.
        */
        // char sockbuf[MAX_BUF_LEN];
        char sockbuf1byte;
        char newlinechar;
        memcpy(&newlinechar, "\n", 1);
        // memset(sockbuf, 0, sizeof(sockbuf));
        // int cnt = 0;
        ssize_t recvbyte;
        do
        {
            recvbyte = recv(acceptfd, &sockbuf1byte, 1, 0); // if doesn't work, try read()
            if (recvbyte == 0 || recvbyte == -1) {
                syslog(LOG_ERR, "Socket recv() received a %i\n", (int)recvbyte);
                // syslog(LOG_ERR, "Errno: %d\n", errno);
                // return -1; // use break; instead??
            }
            // memcpy(&sockbuf[cnt], &sockbuf1byte, 1);
            // cnt++;
            fwrite(&sockbuf1byte, sizeof(char), 1, file);

        } while (memcmp(&sockbuf1byte, &newlinechar, 1) != 0);

        // Write to /var/tmp/aesdsocketdata
        // size_t sockbuflen = sizeof(sockbuf)/sizeof(char);
        // fwrite(sockbuf, sizeof(char), sockbuflen, file);
        // fputs(sockbuf, file);
        fflush(file); // fwrite appears to need fflush() called after it.

        // f. Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
        /*
        You may assume the total size of all packets sent (and therefore size of /var/tmp/aesdsocketdata) will be less than the size of the root filesystem, however you may not assume this total size of all packets sent will be less than the size of the available RAM for the process heap.
        */
        // char *line = NULL;
        // size_t len_write = 0;
        // rewind(file); // necessary to use with getline
        // getline(&line, &len_write, file);
        // send(acceptfd, line, strlen(line), 0);
        
        // while (getline(&line, &len_write, file) != -1) {
        //     send(acceptfd, line, strlen(line), 0);
        // }

        // char readbuf[MAX_BUF_LEN] = {0};
        char readbuf1byte = 0;
        // memset(readbuf, 0, sizeof(readbuf));
        ssize_t num_read;
        // cnt = 0;
        int fseekerr = fseek(file, 0, SEEK_SET);
        if (fseekerr) {
            return -1;
        }
        // size_t size1 = 1;
        // size_t nmemb = 1;
        do {
            num_read = fread(&readbuf1byte, sizeof(char), 1, file);
            if (feof(file)) {
                break;
            }
            // readbuf[cnt] = readbuf1byte;
            // cnt++;
            send(acceptfd, &readbuf1byte, 1, 0);
        } while (num_read > 0);

        // int ch;
        // int ch_all[MAX_BUF_LEN];
        // cnt = 0;
        // rewind(file);
        // long pos = 0;
        // while ((ch = fgetc(file)) != EOF) {
        //     putchar(ch);  // Display the character (you can perform other operations here)
        //     pos = ftell(file);
        // }
        // (void)pos;

        // do {
        //     ch = fgetc(file);
        //     int ferrorval = ferror(file);
        //     if (ferrorval) {
        //         syslog(LOG_ERR, "Fgetc failed %d", ferrorval);
        //     }
        //     // fseek(file, cnt, SEEK_SET);
        //     memcpy(&ch_all[cnt], &ch, 1);
        //     cnt++;
        // } while (ch != EOF);
        // send(acceptfd, ch_all, MAX_BUF_LEN, 0);



        free(line);
        close(acceptfd);
        // TODO: g. Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
        syslog(LOG_NOTICE, "Closed connection from %s\n", ipaddr);
    }

    // TODO: i. Gracefully exits when SIGINT or SIGTERM is received, completing any open connection operations, closing any open sockets, and deleting the file /var/tmp/aesdsocketdata.
    /*
    Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received.
    */
    syslog(LOG_ERR, "Caught signal, exiting\n");
    fclose(file);
    printf("Caught signal, exiting\n");
    remove("/var/tmp/aesdsocketdata");
    shutdown(sockfd,SHUT_RDWR);
    close(sockfd);
    freeaddrinfo(servinfo); // Free the malloc'd space from getaddrinfo()

    return 0; // no errors
}