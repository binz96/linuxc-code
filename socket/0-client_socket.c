#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT (8080)

#define ERR_EXIT(m) \
            do { \
                perror(m); \
                exit(EXIT_FAILURE); \
            } while (0)

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        ERR_EXIT("socket");
    }
    
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ERR_EXIT("connect");
    }
    
    char sendbuf[4096];
    char recvbuf[4096];
    for ( ; ; ) {
        bzero(sendbuf, sizeof(sendbuf));
        read(STDIN_FILENO, sendbuf, sizeof(sendbuf));
        write(sockfd, sendbuf, strlen(sendbuf));
        bzero(recvbuf, sizeof(recvbuf));
        int nread = read(sockfd, recvbuf, sizeof(recvbuf));
        if (nread > 0) {
            printf("server: %s", recvbuf);
        } else if (nread == 0) {
            puts("服务器已断开连接");
            break;
        } else {
            ERR_EXIT("read");
        }
    }
    close(sockfd);
    return 0;
}