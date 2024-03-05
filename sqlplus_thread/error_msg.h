// error msg
#ifndef ERR_MSG
#define ERR_MSG

const char* err_input = "need socket port input\n";
const char* err_make_socket = "make_socket() failed\n";
const char* err_listen = "listen() failed\n";
const char* err_accept = "accept() failed\n";
const char* err_fork = "fork() failed\n";
const char* err_waitpid = "waitpid() failed\n";
const char* log_read_eof = "client sent eof\n";

#endif