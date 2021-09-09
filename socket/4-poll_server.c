#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/time.h>

#define PORT (8080)
#define OPEN_MAX (1024)

#define ERR_EXIT(m) \
            do { \
                perror(m); \
                exit(EXIT_FAILURE); \
            } while (0)

int main() {
    
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) 
        ERR_EXIT("socket");
    
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int on = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        ERR_EXIT("setsockopt");
    }
    
    if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ERR_EXIT("bind");
    }

    if (listen(listenfd, 1024) < 0) {
        ERR_EXIT("listen");
    }

//     /* Data structure describing a polling request.  */
// struct pollfd
//   {
//     int fd;			/* File descriptor to poll.  */
//     short int events;		/* Types of events poller cares about.  */
//     short int revents;		/* Types of events that actually occurred.  */
//   };

    struct pollfd pollfds[OPEN_MAX];

    pollfds[0].fd = listenfd;
    pollfds[0].events = POLLIN;

    for (int i = 1; i < OPEN_MAX; ++i) {
        pollfds[i].fd = -1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    for ( ; ; ) {
        int nready = poll(pollfds, OPEN_MAX, -1);

        if (nready == -1) {
            ERR_EXIT("poll");
        }

        if(pollfds[0].revents & POLLIN) {//可能是listenfd可读，表示有新客户端发起连接
            pollfds[0].revents = 0;
            int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addrlen);
            if (connfd == -1) {
                ERR_EXIT("accept"); 
            }
            int i;
            for (i = 0; i < OPEN_MAX; ++i) {
                if (pollfds[i].fd == -1) {
                    pollfds[i].fd = connfd;
                    pollfds[i].events = POLLIN;
                    break;
                }
            }
            if (i == OPEN_MAX) {
                puts("too many clients");
                exit(EXIT_FAILURE);
            }
            char ipv4_addr[16];
            if (inet_ntop(AF_INET, &client_addr.sin_addr, ipv4_addr, sizeof(ipv4_addr)) == NULL)
                ERR_EXIT("inet_ntop");
            printf("client(%s:%d) is connected!\n", ipv4_addr, ntohs(client_addr.sin_port));
        } else {//也可能是有已连接客户端发来数据
            for (int i = 1; i < OPEN_MAX; ++i) {
                if (pollfds[i].fd != -1 && pollfds[i].revents & POLLIN) {
                    char recvbuf[4096] = {0};
                    int nread = read(pollfds[i].fd, recvbuf, sizeof(recvbuf));
                    if (nread == -1)
                        ERR_EXIT("read");
                    if (nread == 0) {
                        close(pollfds[i].fd);
                        printf("connfd=%d已断开\n", pollfds[i].fd);
                        pollfds[i].fd = -1;
                    } else {
                        printf("connfd=%d: %s\n", pollfds[i].fd, recvbuf);
                        write(pollfds[i].fd, recvbuf, strlen(recvbuf));
                    }
                }
            }
        }
    }
    close(listenfd);
}