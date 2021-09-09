#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/time.h>

#define PORT (8080)

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
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        ERR_EXIT("setsockopt");
    
    if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        ERR_EXIT("bind");

    if (listen(listenfd, 1024) < 0)
        ERR_EXIT("listen");


    int epfd = epoll_create(1); // 参数无意义，填个正整数就行
    if (epfd == -1) {
        ERR_EXIT("epoll_creat");
    }

// typedef union epoll_data
// {
//   void *ptr;
//   int fd;
//   uint32_t u32;
//   uint64_t u64;
// } epoll_data_t;

// struct epoll_event
// {
//   uint32_t events;	/* Epoll events */
//   epoll_data_t data;	/* User data variable */
// } __EPOLL_PACKED;

    struct epoll_event ev;
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;

    // 添加监听套接字
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        ERR_EXIT("epoll_ctl");
    }
    
    struct epoll_event events[1024] = {0};
    
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);

    char buf[4096];

    for ( ; ; ) {

        int nfds = epoll_wait(epfd, events, 1024, -1);
        if (nfds == -1) {
            ERR_EXIT("epoll_wait");
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == listenfd) {
                int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addrlen);
                if (connfd == -1) {
                    ERR_EXIT("accept");
                }
                ev.data.fd = connfd;
                ev.events = EPOLLIN;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
                    ERR_EXIT("epoll_ctl");
                }
            } else {
                bzero(buf, sizeof(buf));
                int nread = read(events[i].data.fd, buf, sizeof(buf));
                if (nread > 0) {
                    printf("Recvive from %d: %s\n", events[i].data.fd, buf);
                    write(events[i].data.fd, buf, nread);
                } else if (nread == 0) {
                    printf("client %d quit!\n", events[i].data.fd);
                    if (epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1) {
                        ERR_EXIT("epoll_ctl");
                    }
                    close(events[i].data.fd);
                } else {
                    ERR_EXIT("read");
                }
            }
        }
    }
    close(listenfd);
}
