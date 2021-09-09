#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
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

typedef struct SockInfo {
    struct sockaddr_in addr;
    int connfd;
} SockInfo;

void* do_service(void* arg) {
    SockInfo* pinfo = (SockInfo*)arg;
    char client_ip[15] = {0};
    strcpy(client_ip, inet_ntoa(pinfo->addr.sin_addr));
    in_port_t client_port = ntohs(pinfo->addr.sin_port);
    int connfd = pinfo->connfd;
    char recvbuf[4096];
    for ( ; ; ) {
        bzero(recvbuf, sizeof(recvbuf));
        int nread = read(connfd, recvbuf, sizeof(recvbuf));
        if (nread > 0) {
            printf("client[%s:%d]: %s", client_ip, client_port, recvbuf);
            write(connfd, recvbuf, nread);
        } else if (nread == 0) {
            printf("client[%s:%d]已断开连接", client_ip, client_port);
            break;
        } else {
            ERR_EXIT("read");
        }
    }
    close(connfd);
}

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

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    for ( ; ; ) {
        bzero(&client_addr, sizeof(client_addr));
        int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addrlen);
        if (connfd < 0) {
            perror("accept");
            continue;
        }
        printf("client %s:%d connected! create a thread to handle\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        pthread_t tid;
        SockInfo info;
        info.addr = client_addr;
        info.connfd = connfd;
        pthread_create(&tid, NULL, do_service, &info);
        pthread_detach(tid);
    }
    close(listenfd);
}