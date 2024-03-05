#include "connect.h"
#include "server.h"


static int get_client_socket(int server_socket_fd) {
    struct sockaddr_in  client_address;
    socklen_t           client_len;
    int                 client_socket;

    client_len = sizeof(client_address);
    client_socket = accept(server_socket_fd, &client_address, &client_len);
    if (client_socket == -1) print_err("accept failed\n");
    print_log("new client connected\n");
    return client_socket;
}

void get_connection(t_socket_info server_socket) {
    int                 ret;
    int                 epoll_fd;
    struct epoll_event  event;
    
    epoll_fd = epoll_create(EPOLL_EVENT_SIZE);
    assert(epoll_fd != -1);

    event.events = EPOLLIN;
    event.data.fd = server_socket.fd;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket.fd, &event);
    assert(ret == 0);
    
    while (1) {
        struct epoll_event  epoll_events[EPOLL_EVENT_SIZE];
        int                 epoll_event_count;
        
        epoll_event_count = epoll_wait(epoll_fd, epoll_events, EPOLL_EVENT_SIZE, -1);
        if (epoll_event_count == -1) print_err("epoll_wait failed()\n");

        for (int i = 0; i < epoll_event_count; i++) {
            int         *client_socket;
            pthread_t   pthread;
            int         ret;

            client_socket = malloc(sizeof *client_socket);
            if (client_socket == NULL) {
                print_err("malloc failed\n");
                continue ;
            }
            *client_socket = get_client_socket(server_socket.fd);
            if (*client_socket == -1) continue ; 
            
            ret = pthread_create(&pthread, 0, oracle_call_server, client_socket);
            if (ret != 0) {
                print_err("pthread_create failed\n");
                close(*client_socket);
                free(client_socket);
            }
            if (ret != 0) {
                print_err("pthread_detach failed\n");
                close(*client_socket);
                free(client_socket);
            }
        }
    }
    exit(1);
}
