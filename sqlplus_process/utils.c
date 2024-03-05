#include "server.h"

long get_port(const int argc, const char* argv[]) 
{
    if (argc != 2) return INVALID_PORT;
    
    for (size_t i = 0; argv[1][i]; i++) {
        if (isdigit(argv[1][i]) == FALSE) return INVALID_PORT;
    }

    size_t len = strlen(argv[1]);//port 0 ~ 65535, so len 1 ~ 5
    if (len < 1 || len > 5) return INVALID_PORT;

    long port;
    port = strtol(argv[1], NULL, 10);
    return (port < 0 || port > 65535) ?\
             INVALID_PORT : port;
}

static void print_msg(int fd, const char* msg) 
{
    write(fd, msg, strlen(msg));
}

void print_err(const char* msg) 
{
    print_msg(STDERR, msg);
}

void print_log(const char* msg)
{
    print_msg(STDOUT, msg);
}

void print_id_format(const char* log_format_front, const char* log_format_back, 
                    const char* msg, int id) {
    print_log(log_format_front);
    char id_str[32] = {0, };
    int str_bytes = sprintf(id_str, "%d", id);
    write(STDOUT, id_str, str_bytes);
    
    print_log(log_format_back);
    print_log(msg);
}

void print_client_log(const char* msg, int client_id)
{
    print_id_format("client [", "] : ", msg, client_id);
}

void print_process_log(const char* msg, int process_id)
{
    print_id_format("process [", "] : ", msg, process_id);
}