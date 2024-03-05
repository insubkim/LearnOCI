#include "connect.h"
#include "server.h"
#include "list.h"

#define OCI_EPOLL_SIZE 2

#include <string.h>
#include "oci.h"

#define OCI_CHKSUCC(c)  (c == OCI_SUCCESS || c == OCI_SUCCESS_WITH_INFO)


#define USER_ID     "insub"
#define USER_PW     "123"

#define ERR_SIZE        1024
#define BUF_SIZE        1024

OCIEnv*     env_hp;
OCIError*   error_hp;
OCISvcCtx*  service_hp;
OCIServer*  server_hp;
OCIStmt*    stmt_hp = 0;
OCIDefine*  define = 0;

char err[ERR_SIZE];

void    clean_up(int exit_code) {
    int ret = OCISessionEnd(service_hp, error_hp, 0, OCI_DEFAULT);
    ret = OCIHandleFree(service_hp, OCI_HTYPE_SVCCTX);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHandleFree: failed\n");
    }
    ret = OCIHandleFree(error_hp, OCI_HTYPE_ERROR);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHandleFree: failed\n");
    }
    ret = OCIHandleFree(env_hp, OCI_HTYPE_ENV);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHandleFree: failed\n");
    }
    printf("clean up\n");
    exit(exit_code);
}

void set_connection(void) {
    int ret = OCI_ERROR;
    //oci 환경 초기화
    ret = OCIEnvCreate(&env_hp, OCI_DEFAULT, 
                    NULL, NULL, NULL, NULL, 0, NULL);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIEnvCreate: failed\n");
        clean_up(1);
    }

    //에러 핸들 할당
    ret = OCIHandleAlloc(env_hp, (void **)&error_hp, 
                        OCI_HTYPE_ERROR, 0, NULL);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHangleAlloc: failed\n");
        clean_up(1);
    }

    //서비스 핸들 할당
    ret = OCIHandleAlloc(env_hp, (void **)&service_hp, 
                        OCI_HTYPE_SVCCTX, 0, NULL);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHangleAlloc: failed\n");
        clean_up(1);
    }

    //서비스 핸들, 서버, 유저, 트랜잭션 초기화
    ret = OCILogon(env_hp, error_hp, &service_hp,
            (unsigned char *)USER_ID, strlen(USER_ID),
            (unsigned char *)USER_PW, strlen(USER_PW),
            (unsigned char *)NULL, 0);
    if (!OCI_CHKSUCC(ret)) {
        int errcode = -1;
        err[0] = '\0';
        OCIErrorGet(error_hp, 1, NULL, 
                &errcode, (unsigned char *)err,
                ERR_SIZE, OCI_HTYPE_ERROR);
        fprintf(stderr, "OCILogon: failed: %d:%s\n",
                errcode, err);
        clean_up(1);
    }
    printf("CONNECTED!\n");
}

int have_data(int status) {
    return status == OCI_SUCCESS;
}

t_list* send_query(t_list* list) {
    int ret = OCI_ERROR;

    //statement 할당
    ret = OCIHandleAlloc(env_hp, (dvoid **)&stmt_hp, 
                    OCI_HTYPE_STMT, 0, 0);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHandleAlloc: failed\n");
        clean_up(1);
    }

    text      sql_statement[BUF_SIZE] = {0, };
    t_list *node = list;
    int offset = 0;
    while (node) {
        memcpy(sql_statement + offset, node->data, node->size);
        offset += node->size;
        node = node->next;
    }
    
    clear_list(list);
    list = NULL;
    //statement 준비
    ret = OCIStmtPrepare(stmt_hp, error_hp, sql_statement, 
                strlen((const char*)sql_statement), 
                OCI_NTV_SYNTAX, OCI_DEFAULT); 
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIStmtPrepare: failed\n");
        clean_up(1);
    }
    //set buffer pos                      
    char*       name_buffer     = malloc(sizeof(char) * BUF_SIZE);    
    ret = OCIDefineByPos(stmt_hp, &define, error_hp, 
                (ub4)1, name_buffer, BUF_SIZE, 
                (ub2)SQLT_STR, 0, 0, 0, OCI_DEFAULT);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIDefineByPos: failed\n");
        clean_up(1);
    }

    int status;
    int rows = 1;
    //statement 실행
    status = OCIStmtExecute(service_hp, stmt_hp, error_hp,
                            rows, (ub4)0,
                            (CONST OCISnapshot*)NULL,
                            (OCISnapshot*)NULL, OCI_COMMIT_ON_SUCCESS);
    if (!OCI_CHKSUCC(status)) {
        fprintf(stderr, "OCIStmtExecute: failed\n");
        int errcode = -1;
        OCIErrorGet(error_hp, 1, NULL, 
                &errcode, (unsigned char *)err,
                ERR_SIZE, OCI_HTYPE_ERROR);
        fprintf(stderr, "OCILogon: failed: %d:%s\n",
                errcode, err);
        clean_up(1);
    }
    if (status == OCI_NO_DATA){
        printf("fetched none\n");
        return 0;
    } else {
        printf("fetched data\n");
    }

    while (have_data(status)) {//check status
        push_back(&list, name_buffer);
        status = OCIStmtFetch(stmt_hp, error_hp, 
                            rows, OCI_FETCH_NEXT, OCI_DEFAULT);
    }
    OCIHandleFree(stmt_hp, OCI_HTYPE_STMT);
    return list;
}

int sql_done(t_list* last_node) {
    char*   buf;

    buf = last_node->data;

    while (*buf && *buf != '\n')
        buf++;

    return *buf == '\n';
}

void    oracle_call_server(int client_socket) {
    
    set_connection();

    int ret;
    int epoll_fd;
    struct epoll_event event;

    epoll_fd = epoll_create(OCI_EPOLL_SIZE);
    assert(epoll_fd != -1);

    event.events = EPOLLIN;
    event.data.fd = client_socket;
    ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
    assert(ret == 0);

    t_list* list = NULL;
    
    while (1) {
        struct epoll_event  epoll_events[OCI_EPOLL_SIZE];
        int                 epoll_event_count;
        
        epoll_event_count = epoll_wait(epoll_fd, epoll_events, EPOLL_EVENT_SIZE, -1);
        assert(epoll_event_count != -1);    

        for (int i = 0; i < epoll_event_count; i++) {
            if (epoll_events[i].events && EPOLLIN) {
                char buf[BUF_SIZE] = {0, };
                int read_bytes; 
                
                read_bytes = read(epoll_events[i].data.fd, buf, BUF_SIZE - 1);
                buf[read_bytes] = 0;
                if (read_bytes == 0) {
                    print_log("client disconnect\n");
                    clean_up(0);
                }
                t_list* node = push_back(&list, buf);
                if (node == NULL) {
                    print_err("malloc failed on push_back()");
                    return 1;
                }

                if (sql_done(node)) {
                    list = send_query(list);
                    
                    event.events = EPOLLOUT;
                    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_socket, &event);
                    assert(ret == 0);
                }
            }
            if (epoll_events[i].events && EPOLLOUT) {
                t_list* node = list;
                while (node) {
                    write(epoll_events[i].data.fd, node->data, node->size);
                    write(epoll_events[i].data.fd, "\n", 1);
                    node = node->next;
                }

                clear_list(list);
                list = NULL;
                
                event.events = EPOLLIN;
                int ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_socket, &event);
                assert(ret == 0);
            }
        }
    }
}
