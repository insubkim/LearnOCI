#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h> 

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define FALSE  0
#define TRUE   !FALSE

#define SOCK_DOMAIN AF_INET
#define SOCK_TYPE SOCK_STREAM
#define SOCK_PROTOCOL 0

typedef struct s_socket_info {
    int domain;
    int type;
    int protocol;
    struct sockaddr_in address;
    int is_open;
    int fd;
} t_socket_info;

t_socket_info   make_socket(int port) 
{
    t_socket_info server_socket;
    memset(&server_socket, 0, sizeof server_socket);
    
    int socket_fd = socket(SOCK_DOMAIN , \
                SOCK_TYPE, SOCK_PROTOCOL);
    if (socket_fd == -1) return server_socket;
    server_socket.fd = socket_fd;

    struct sockaddr_in addr;
    addr.sin_family = SOCK_DOMAIN;
    addr.sin_addr.s_addr= INADDR_ANY;
    addr.sin_port = htons(port);

    server_socket.domain = SOCK_DOMAIN; 
    server_socket.type = SOCK_TYPE;
    server_socket.protocol = SOCK_PROTOCOL;
    server_socket.address = addr;
    server_socket.is_open = TRUE;
    return server_socket;
}


#define PORT 8080
#define SERVER_ADDR "127.0.0.1"

void*   f(void* val) {
    t_socket_info sock = make_socket(PORT);
    
    assert(connect(sock.fd, &sock.address, sizeof sock.address) != -1);
    
    char*   msg = "select info from student\n";
    

    int wbytes = write(sock.fd, msg, strlen(msg));
    char buf[1024] = {0, };
    int rbytes = read(sock.fd, buf, 1023);
    if (wbytes == -1|| rbytes == -1)// || wbytes != rbytes)
    	printf("error!!! on write(%d) read(%d)\n", wbytes,  rbytes);
    close(sock.fd);
}

int main(int argc, char** argv) {
    if (argc == 1) return -1;

    int thread_num = atoi(argv[1]);
    if (thread_num > 1023) return -1;
    time_t start = clock();
    pthread_t ptids[1024] = {0, };
    printf("clients start\n");
    for (int i = 0; i < thread_num; i++) 
    {
        pthread_t ptid;
        assert(pthread_create(&ptid, 0, f, 0) == 0);
        ptids[i] = ptid;
    }
    for (int i = 0; i < thread_num; i++) 
    {
        assert(pthread_join(ptids[i], 0) == 0);
    }
    time_t end = clock();
    printf("client done time : %lf\n", (double)(end - start)/CLOCKS_PER_SEC);
    printf("clients end\n");
    return 0;
}
