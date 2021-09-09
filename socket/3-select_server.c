#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#define PORT (8080)

#define ERR_EXIT(m) \
            do { \
                perror(m); \
                exit(EXIT_FAILURE); \
            } while (0)

int main() {
    
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        ERR_EXIT("socket");
    }
    
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

    int maxfd = listenfd;
    int connfd_array[1024];
    memset(connfd_array, -1, sizeof(connfd_array));
    fd_set readfds, tmpfds;
    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    for ( ; ; ) {
        memcpy(&tmpfds, &readfds, sizeof(readfds));
        if (select(maxfd+1, &tmpfds, NULL, NULL, NULL) == -1) {
            ERR_EXIT("select");
        }
            
        if (FD_ISSET(listenfd, &tmpfds)) {//可能是listenfd可读，表示有新客户端发起连接
            int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addrlen);
            if (connfd == -1) {
                ERR_EXIT("accept");
            }
            int i;
            for (i = 0; i < 1024; ++i) {
                if (connfd_array[i] == -1) {
                    connfd_array[i] = connfd;
                    break;
                }
            }
            if (i == 1024) {
                puts("too many clients");
                exit(EXIT_FAILURE);
            }
            FD_SET(connfd, &readfds);
            if (connfd > maxfd) {
                maxfd = connfd;
            }
            char ipv4_addr[16];
            if (inet_ntop(AF_INET, &client_addr.sin_addr, ipv4_addr, sizeof(ipv4_addr)) == NULL)
                ERR_EXIT("inet_ntop");
            printf("client(%s:%d) is connected!\n", ipv4_addr, ntohs(client_addr.sin_port));
        } else {//也可能是有已连接客户端发来数据
            for (int i = 0; i < 1024; ++i) {
                if (connfd_array[i] != -1 && FD_ISSET(connfd_array[i], &tmpfds)) {
                    char recvbuf[4096] = {0};
                    int nread = read(connfd_array[i], recvbuf, sizeof(recvbuf));
                    if (nread == -1)
                        ERR_EXIT("read");
                    if (nread == 0) {
                        close(connfd_array[i]);
                        FD_CLR(connfd_array[i], &readfds);
                        printf("connfd=%d已断开连接\n", connfd_array[i]);
                        connfd_array[i] = -1;
                    } else {
                        printf("connfd=%d: %s\n", connfd_array[i], recvbuf);
                        write(connfd_array[i], recvbuf, strlen(recvbuf));
                    }
                }
            }
        }
    }
    close(listenfd);
}