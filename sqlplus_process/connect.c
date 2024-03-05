#include "connect.h"
#include "server.h"


int get_client_socket(int server_socket_fd) {
    struct sockaddr_in  client_address;
    socklen_t           client_len;
    int                 client_socket;

    client_len = sizeof(client_address);
    client_socket = accept(server_socket_fd, &client_address, &client_len);
    if (client_socket == -1) print_err("accept failed\n");
    print_log("new client connected\n");
    return client_socket;
}

void proc_exit(void) {
    int status;
    int ret;
    
    do {
        ret = waitpid(-1, &status,WNOHANG);
        if (ret > 0) print_process_log("terminated\n", ret);
    } while (ret > 0) ;
}

void get_connection(t_socket_info server_socket) {
    int ret;
    int epoll_fd;
    struct epoll_event event;
    
    //set epoll
    epoll_fd = epoll_create(EPOLL_EVENT_SIZE);
    assert(epoll_fd != -1);

    event.events = EPOLLIN;
    event.data.fd = server_socket.fd;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket.fd, &event);
    assert(ret == 0);
    
    signal (SIGCHLD, proc_exit);
    
    while (1) {
        struct epoll_event  epoll_events[EPOLL_EVENT_SIZE];
        int                 epoll_event_count;
        
        epoll_event_count = epoll_wait(epoll_fd, epoll_events, EPOLL_EVENT_SIZE, -1);

        for (int i = 0; i < epoll_event_count; i++) {
            int client_socket;
            pid_t pid;
            
            client_socket = get_client_socket(server_socket.fd);
            
            pid = fork();
            if (pid == 0) {
                oracle_call_server(client_socket);
                exit(0);
            }

            close(client_socket);
            break;
        }
    }
}
