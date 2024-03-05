#ifndef PROCESS_SERVER_H

#define PROCESS_SERVER_H
#pragma once

#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define FALSE  0
#define TRUE   !FALSE

#define INVALID_PORT -1

#define SOCK_DOMAIN AF_INET
#define SOCK_TYPE SOCK_STREAM
#define SOCK_PROTOCOL 0
#define BACKLOG 5

#define READ_BUF_SIZE 1024

// socket 
typedef struct s_socket_info {
    int domain;
    int type;
    int protocol;
    struct sockaddr_in address;
    int is_open;
    int fd;
} t_socket_info;

// functions
long get_port(const int argc, const char* argv[]);
void print_err(const char* msg);
void print_log(const char* msg);
void print_client_log(const char* msg, int client_id);
void print_process_log(const char* msg, int process_id);

void get_connection(t_socket_info server_socket);

void    oracle_call_server(int client_socket);

#endif