#include "server.h"
#include "error_msg.h"


static void    set_reuseaddr(int socket_fd) {
    int optval = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

/* @brief returns server_socket, if fails is_open is false
 */

static t_socket_info   make_socket(int port) 
{
    t_socket_info       server_socket;
    int                 socket_fd;
    struct sockaddr_in  addr;
    int                 ret;

    memset(&server_socket, 0, sizeof server_socket);
    
    socket_fd = socket(SOCK_DOMAIN , \
                SOCK_TYPE, SOCK_PROTOCOL);
    if (socket_fd == -1) return server_socket;
    server_socket.fd = socket_fd;

    set_reuseaddr(server_socket.fd);

    addr.sin_family = SOCK_DOMAIN;
    addr.sin_addr.s_addr= INADDR_ANY;
    addr.sin_port = htons(port);
    ret = bind(server_socket.fd, &addr, sizeof(addr));
    if (ret == -1) return server_socket;

    server_socket.domain = SOCK_DOMAIN; 
    server_socket.type = SOCK_TYPE;
    server_socket.protocol = SOCK_PROTOCOL;
    server_socket.address = addr;
    server_socket.is_open = TRUE;



    return server_socket;
}

int main(int argc, char* argv[]) {
    int             ret;
    long            port;
    t_socket_info   server_socket;

    if (argc != 2) 
    {
        print_err(err_input);
        return 1;
    }

    port = get_port(argc, argv);
    if (port == INVALID_PORT) 
    {
        print_err(err_input);
        return 1;
    }
    
    server_socket = make_socket(port);
    if (server_socket.is_open == FALSE) 
    {
        print_err(err_make_socket);
        return 2;
    }

    ret = listen(server_socket.fd, BACKLOG);
    if (ret == -1)
    {
        print_err(err_listen);
        return 2; 
    }

    assert(server_socket.fd >= 0);
    
    get_connection(server_socket);

    close(server_socket.fd);
    return 0;
}