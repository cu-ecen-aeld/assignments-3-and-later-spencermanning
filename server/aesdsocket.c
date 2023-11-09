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
#include <arpa/inet.h>  // for inet_ntop()
// extras
#include <netinet/in.h>
#include <fcntl.h>

// Assignment 6
#include <sys/queue.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

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

volatile int received_exit_signal = 0;
struct addrinfo *servinfo; // res
int sockfd;

void close_all_things() {
    remove("/var/tmp/aesdsocketdata");
    shutdown(sockfd,SHUT_RDWR);
    close(sockfd);
    freeaddrinfo(servinfo); // Free the malloc'd space from getaddrinfo()
}

static void signal_handler(int signal_num) {
    /* Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received. */
    syslog(LOG_ERR, "Caught signal, exiting\n");
    
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

// the recursive "entries" member is necessary to use the linked lists from sys/queue.h
struct Node {
    pthread_t thread_id;
    bool is_complete;
    SLIST_ENTRY(Node) entries;
};

struct threadArgs {
    char ipaddr[INET_ADDRSTRLEN];
    int acceptfd;
    FILE *file;
    struct Node *thread_node;
};

pthread_mutex_t mutex;

void closeThread(struct threadArgs *args) {
    close(args->acceptfd);

    // 5g. Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
    syslog(LOG_NOTICE, "Closed connection from %s\n", args->ipaddr);

    // Set a global boolean to signal the end of the thread
    args->thread_node->is_complete = true;

    pthread_exit(NULL);
}

void* timer_thread(void * arg) {
    // Same aesdsocketdata file always
    FILE *argfile = (FILE *)arg;

    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[31];

    while (1) {
        // Sleep first for 10 seconds to avoid interrupting initial connections
        sleep(10);

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestamp, sizeof(timestamp), "timestamp:%Y-%m-%d %H:%M:%S\n", timeinfo);
        
        pthread_mutex_lock(&mutex);
        fwrite(&timestamp, sizeof(char), 31, argfile);
        fflush(argfile); // push the data to the file
        pthread_mutex_unlock(&mutex);
    }
    // should never get here
    pthread_exit(NULL);
}

void* connection_thread(void * arg) {
    
    // Unpack the conn_args
    struct threadArgs *conn_args = (struct threadArgs *)arg;

    // 5e. Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
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
    ssize_t recvbyte;

    pthread_mutex_lock(&mutex);
    do
    {
        recvbyte = recv(conn_args->acceptfd, &sockbuf1byte, 1, 0); // if doesn't work, try read()
        if (recvbyte == 0 || recvbyte == -1) {
            syslog(LOG_ERR, "Socket recv() received an error: %i\n", (int)recvbyte);
            closeThread(conn_args);
        }
        if (fwrite(&sockbuf1byte, sizeof(char), 1, conn_args->file) == 0) {
            syslog(LOG_ERR, "Write to file failed.");
            closeThread(conn_args);
        }

    } while (memcmp(&sockbuf1byte, &newlinechar, 1) != 0);

    // Write to /var/tmp/aesdsocketdata
    fflush(conn_args->file); // fwrite needs to be flushed after it's called
    
    pthread_mutex_unlock(&mutex); // Don't put mutex functions in do-while loops

    // 5f. Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
    /* You may assume the total size of all packets sent 
    (and therefore size of /var/tmp/aesdsocketdata) will be less than the size 
    of the root filesystem, however you may not assume this total size of all 
    packets sent will be less than the size of the available RAM for the process heap.
    */

    char readbuf1byte[30] = {0}; // the sockettest.sh for asy6.1 works better with this as 30 chars
    ssize_t num_read;
    int fseekerr = fseek(conn_args->file, 0, SEEK_SET);
    if (fseekerr) {
        exit(1);
    }
    do {
        num_read = fread(&readbuf1byte, sizeof(char), 1, conn_args->file);
        if (feof(conn_args->file)) {
            break;
        }
        if (send(conn_args->acceptfd, &readbuf1byte, 1, 0) == -1) {
            syslog(LOG_ERR, "Send failed");
            closeThread(conn_args);
        }
    } while (num_read > 0);

    closeThread(conn_args);

    return NULL; // will not get here.
}

int main (int argc, char *argv[]) {
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
                exit(1);
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

    // Start with a clean file
    remove("/var/tmp/aesdsocketdata");

    FILE *file;
    file = fopen("/var/tmp/aesdsocketdata", "a+");
    if (!file) {
        syslog(LOG_ERR, "File didn't open\n");
        printf("File didn't open\n");
        exit(1);
    }

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

    // Get the socket address (sockaddr)
    struct addrinfo hints;
    // struct addrinfo *servinfo; // res
    memset(&hints, 0, sizeof(hints)); // start struct as zeros
    hints.ai_family = AF_UNSPEC; // don't care if either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    int getaddrinfoout = getaddrinfo(NULL, "9000", &hints, &servinfo);
    if (getaddrinfoout == -1) {
        exit(1);
    }

    // 5b. Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sockfd == -1) {
        syslog(LOG_ERR, "Socket connection failed\n");
        exit(1); 
    }

    // Make the address able to be used again with SO_REUSEADDR
    int myoptval = 1;
    int sockoptout = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &myoptval, sizeof(myoptval));
    if (sockoptout == -1) {
        exit(1);
    }

    int bindout  = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (bindout == -1) {
        exit(1);
    }

    // 5c. Listens for and accepts a connection
    int listenout = listen(sockfd, 20); // 20 from TA
    if (listenout == -1) {
        exit(1);
    }

    struct sockaddr_storage clientinfo;
    // struct sockaddr_in clientinfo;
    socklen_t client_addr_size = sizeof(clientinfo);

    pthread_mutex_init(&mutex, NULL);

    pthread_t timer_thread_id;
    if (pthread_create(&timer_thread_id, NULL, timer_thread, (void *)file) != 0) {
        syslog(LOG_ERR, "Timer thread creation failed");
        return 1;
    }

    // Initialize the head of the linked list
    struct Node *myNode = NULL;
    SLIST_HEAD(ListHead, Node) head = SLIST_HEAD_INITIALIZER(head);
    SLIST_INIT(&head);

    // 5h. Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received (see below).
    while (received_exit_signal == 0) {

        char ipaddr[INET_ADDRSTRLEN]; // IPv4 or IPv6??

        int acceptfd = accept(sockfd, (struct sockaddr*)&clientinfo, &client_addr_size);
        if (acceptfd == -1) {
            continue;
        }

        // Set up client ip address info
        if (clientinfo.ss_family == AF_INET) {
            struct sockaddr_in *addr =  (struct sockaddr_in *)&clientinfo;
            inet_ntop(AF_INET, addr, ipaddr, sizeof(ipaddr));
        }
        else if (clientinfo.ss_family == AF_INET6) {
            struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&clientinfo;
            inet_ntop(AF_INET, addr, ipaddr, sizeof(ipaddr));
        }

        // 5d. Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client.
        syslog(LOG_NOTICE, "Accepted connection from %s\n", ipaddr);

        // malloc memory for each node created. Will be freed with SLIST_REMOVE()
        myNode = (struct Node *)malloc(sizeof(myNode));
        myNode->is_complete = false;

        // Set up the args to pass in
        struct threadArgs args;
        strncpy(args.ipaddr, ipaddr, INET_ADDRSTRLEN);
        args.acceptfd = acceptfd;
        args.file = file;
        args.thread_node = myNode;

        if (pthread_create(&myNode->thread_id, NULL, connection_thread, (void *)&args) != 0) {
            syslog(LOG_ERR, "Thread creation failed");
            return 1;
        }

        SLIST_INSERT_HEAD(&head, myNode, entries);

        SLIST_FOREACH(myNode, &head, entries) {
            if (myNode->is_complete) {
                pthread_join(myNode->thread_id, NULL);
                SLIST_REMOVE(&head, myNode, Node, entries);
                free(myNode);
            }
        }
    }

    /* 5i. Gracefully exits when SIGINT or SIGTERM is received, 
    completing any open connection operations, 
    closing any open sockets, 
    and deleting the file /var/tmp/aesdsocketdata.
    */
    printf("Caught signal, exiting\n");
    fclose(file);
    close_all_things();

    return 0; // no errors
}