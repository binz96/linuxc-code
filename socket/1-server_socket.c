#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // 绑定本地网卡的IP地址
    //server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //inet_aton("127.0.0.1", &server_addr.sin_addr);
    // if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
    //     perror("inet_pton");
    //     exit(EXIT_FAILURE);
    // }

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

    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    socklen_t client_addrlen = sizeof(client_addr);

    int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addrlen);
    if (connfd < 0) {
        ERR_EXIT("accept");
    }
    printf("client %s:%d connected!\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    char recvbuf[4096];
    for ( ; ; ) {
        bzero(recvbuf, sizeof(recvbuf));
        int nread = read(connfd, recvbuf, sizeof(recvbuf));
        if (nread > 0) {
            printf("client[%s:%d]: %s", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), recvbuf);
            write(connfd, recvbuf, nread);
        } else if (nread == 0) {
            puts("客户端已断开连接");
            break;
        } else {
            ERR_EXIT("read");
        }
        // read返回大于0：实际读取的字节数
        // read返回0：对方断开连接
        // read返回-1：读取失败
        // TCP接受缓冲区空，read会阻塞
        
        // write返回大于0：实际发送的字节数
        // write返回-1：发送数据失败
        // TCP发送缓冲区满，write会阻塞
    }
    close(connfd);
    close(listenfd);
    // 关闭成功返回0，失败返回-1
}