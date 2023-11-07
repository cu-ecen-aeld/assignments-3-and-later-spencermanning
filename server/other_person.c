#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

static int exit_signal = 0;

void sig_handler(int sig)
{
    if (sig == SIGINT)
    {
        exit_signal = 1;
    }
    else if (sig == SIGTERM)
    {
        exit_signal = 1;
    }
}

int main(int argc, char** argv)
{

    if (argc == 2)
    {
//        if (strlen(argv[2]) && argv[2][1] == 'd')
        {
            //make damon
            int pid = fork();
            if (pid != 0)
            {
                exit(0);
            }
            setsid();
            chdir("/");
        }
    }

    struct sigaction new_action;
    memset(&new_action, 0, sizeof(struct sigaction));
    new_action.sa_handler=sig_handler;
    sigaction(SIGTERM, &new_action, NULL);
    sigaction(SIGINT, &new_action, NULL);

    struct addrinfo hints;
    struct addrinfo* servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, "9000", &hints, &servinfo);
    if (status != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    int socketfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socketfd == -1)
    {
        fprintf(stderr, "socket error: %d, %s\n", errno, strerror(errno));
        exit(1);
    }

    int option = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    status = bind(socketfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (status == -1)
    {
        fprintf(stderr, "bind error: %d, %s\n", errno, strerror(errno));
        exit(1);
    }

    status = listen(socketfd, 10);
    if (status == -1)
    {
        fprintf(stderr, "listen error: %d, %s\n", errno, strerror(errno));
        exit(1);
    }

    struct sockaddr_storage their_addr; 
    socklen_t addr_size = sizeof(their_addr);
    while(exit_signal == 0)
    {
        char ipstr[INET6_ADDRSTRLEN];
        int acc_fd = accept(socketfd, (struct sockaddr*)&their_addr, &addr_size);
        if (acc_fd == -1)
        {
            fprintf(stderr, "accept error: %d, %s\n", errno, strerror(errno));
        }
        else
        {
            if (their_addr.ss_family == AF_INET)
            {
                struct sockaddr_in *addr = (struct sockaddr_in *)&their_addr;
                inet_ntop(AF_INET, addr, ipstr, sizeof(ipstr));
            }
            else if (their_addr.ss_family == AF_INET6)
            {
                struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&their_addr;
                inet_ntop(AF_INET, addr, ipstr, sizeof(ipstr));
            }
            else
            {
                fprintf(stderr, "undetermined incoming address type");
            }
            printf("Accepted connection from %s\n", ipstr);

            FILE* file = fopen("/var/tmp/aesdsocketdata", "a+");
            if (file == NULL)
            {
                fprintf(stderr, "fopen error: %d, %s\n", errno, strerror(errno));
                exit(1);
            }
            char recvbuf[512];
            while (1)
            {
                ssize_t recv_bytes = recv(acc_fd, recvbuf, 512, 0);
                if (recv_bytes == 0)
                {
                    break;
                }
                else if (recv_bytes == -1)
                {
                    fprintf(stderr, "recv error: %d, %s\n", errno, strerror(errno));
                }
                else
                {
                    fwrite(recvbuf, 1, recv_bytes, file);        
                    if (recvbuf[recv_bytes-1] == '\n')
                    {
                        break;
                    }
                }
            }

            // Close file for appending and open for reading
            rewind(file);
            char* line = NULL;
            size_t len = 0;
            ssize_t read = 0;
            while((read = getline(&line, &len, file)) != -1) 
            {
                send(acc_fd, line, read, 0);
            }
            free(line);
            fclose(file);
            close(acc_fd);
            printf("Closed connection from %s\n", ipstr);

        }
    } 

    remove("/var/tmp/aesdsocketdata");
    shutdown(socketfd, SHUT_RDWR);
    close(socketfd);
    fprintf(stdout, "Goodbye\n");
    freeaddrinfo(servinfo);
    return 0;
}