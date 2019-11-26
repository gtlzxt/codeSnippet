#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define SERVER_PORT (7778)
#define EPOLL_MAX_NUM (2048)
#define BUFFER_MAX_LEN (4096)

char buffer[BUFFER_MAX_LEN];

void str_toupper(char* str)
{
    for(int i = 0; i < strlen(str); ++i)
    {
        str[i] = toupper(str[i]);
    }
}
void server()
{
    int listen_fd = 0;
    int client_fd = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    socklen_t client_len;

    int epfd = 0;

    struct epoll_event event, *my_events;

    //listen socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    //bind
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
    bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    //listen
    listen(listen_fd, 10);

    //epoll create
    epfd = epoll_create(EPOLL_MAX_NUM);
    if(epfd < 0)
    {
        perror("epoll create");
        goto END;
    }

    //register listen_fd to epfd, listen_fd only care about EPOLLIN event
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event) < 0)
    {
        perror("epoll ctl add listen_fd");
        goto END;
    }

    //there will be at most EPOLL_MAX_NUM event can be returned after wait return
    my_events = (epoll_event*)malloc(sizeof(struct epoll_event)* EPOLL_MAX_NUM);
    while(1)
    {
        //wait until event happens, active_fds_cnt sockets have event
        int active_fds_cnt = epoll_wait(epfd, my_events, EPOLL_MAX_NUM, -1);
        for(int i = 0; i < active_fds_cnt; ++i)
        {
            //if listen_fd has event, it means new socket is connected
            if(my_events[i].data.fd == listen_fd)
            {
                //accept
                client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
                if(client_fd < 0)
                {
                    perror("accept");
                    continue;
                }

                char ip[20];
                printf("new connection[%s:%d]\n", inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip)), ntohs(client_addr.sin_port));
                //register new accepted socket to epfd, both EPOLLIN and EPOLLOUT
                event.events = EPOLLIN | EPOLLOUT;
                event.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event);
            }
            //other socket have event EPOLLIN means we can read data from this socket
            else if(my_events[i].events & EPOLLIN)
            {
                printf("EPOLLIN\n");
                client_fd = my_events[i].data.fd;

                buffer[0] = '\0';
                //read data, this will return immediately
                int n = read(client_fd, buffer, 5);
                //something happens when read, just igonre it
                if(n < 0)
                {
                    perror("read");
                    continue;
                }
                //this means socket is closed by peer side, we close it on this side too.
                else if(n == 0)
                {
                    printf("[read]:%s\n", buffer);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, &event);
                    close(client_fd);
                }
                //here we only upper the input case and echo it back to client
                else
                {
                    printf("[read]:%s\n", buffer);
                    buffer[n] = '\0';

                    str_toupper(buffer);
                    write(client_fd, buffer, strlen(buffer));
                    printf("[write]:%s\n", buffer);
                    memset(buffer, 0, BUFFER_MAX_LEN);
                }
            }
        }
    }

    END:
    close(epfd);
    return;
}