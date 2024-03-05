#include "connect.h"
#include "server.h"
#include "list.h"

#define OCI_EPOLL_SIZE 2

#include <string.h>
#include "oci.h"

#define OCI_CHKSUCC(c)  (c == OCI_SUCCESS || c == OCI_SUCCESS_WITH_INFO)


#define USER_ID     "insub"
#define USER_PW     "123"

#define BUF_SIZE        1024
#define ERR_SIZE        1024

OCIEnv*     env_hp = NULL;

typedef struct s_oci {
    OCIEnv*     env_hp;
    OCIError*   error_hp;
    OCISvcCtx*  service_hp;
    OCIServer*  server_hp;
    OCIStmt*    stmt_hp;
    OCIDefine*  define;

    int                 client_socket;
    int                 epoll_fd;
    struct epoll_event  event;
} t_oci;



static void    clean_up(int exit_code, t_oci oci) {
    int ret;    

    if (oci.service_hp && oci.error_hp) {
        ret = OCILogoff(oci.service_hp, oci.error_hp);
            if (!OCI_CHKSUCC(ret)) {
                int errcode;
                char err[ERR_SIZE];
                err[0] = '\0';
                OCIErrorGet(oci.error_hp, 1, NULL, &errcode, (unsigned char *)err,
                    ERR_SIZE, OCI_HTYPE_ERROR);
                fprintf(stderr, "OCILogoff: failed: %d:%s\n",
                    errcode, err);
        }
    }
    if (oci.error_hp) {
        ret = OCIHandleFree(oci.error_hp, OCI_HTYPE_ERROR);
        if (!OCI_CHKSUCC(ret)) {
            fprintf(stderr, "error_hp OCIHandleFree: failed\n");
        }
    }
    if (oci.epoll_fd) {
        ret = epoll_ctl(oci.epoll_fd, EPOLL_CTL_DEL, oci.client_socket, &(oci.event));
        if (ret != 0) printf("EPOLL CTL DEL FAIL\n");
    }
    if (oci.client_socket) {
        ret = close(oci.client_socket);
        if (ret == -1) printf("oci.client_socket close FAIL\n");
    }
    if (oci.epoll_fd) {
        ret = close(oci.epoll_fd);
        if (ret == -1) printf("oci.epoll_fd close FAIL\n");
    }
    printf("clean up\n");
}

static int set_connection(t_oci* ocip) {
    int ret = OCI_ERROR;
    //oci 환경 초기화
    if (env_hp == NULL) {
        ret = OCIEnvCreate(&env_hp, OCI_THREADED, 
                        NULL, NULL, NULL, NULL, 0, NULL);
        if (!OCI_CHKSUCC(ret)) {
            fprintf(stderr, "OCIEnvCreate: failed\n");
            clean_up(1, *ocip);
            return 0;
        }
    }
    ocip->env_hp = env_hp;
    
    //에러 핸들 할당
    ret = OCIHandleAlloc(ocip->env_hp, (void **)&(ocip->error_hp), 
                        OCI_HTYPE_ERROR, 0, NULL);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHangleAlloc: failed\n");
        clean_up(1, *ocip);
        return 0;
    }

    //서비스 핸들 할당
    ret = OCIHandleAlloc(ocip->env_hp, (void **)&(ocip->service_hp), 
                        OCI_HTYPE_SVCCTX, 0, NULL);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHangleAlloc: failed\n");
        clean_up(1, *ocip);
        return 0;
    }

    //서비스 핸들, 서버, 유저, 트랜잭션 초기화
    ret = OCILogon(ocip->env_hp, ocip->error_hp, &(ocip->service_hp),
            (unsigned char *)USER_ID, strlen(USER_ID),
            (unsigned char *)USER_PW, strlen(USER_PW),
            (unsigned char *)NULL, 0);
    if (!OCI_CHKSUCC(ret)) {
        int errcode = -1;
        char err[ERR_SIZE];
        err[0] = '\0';
        OCIErrorGet(ocip->error_hp, 1, NULL, 
                &errcode, (unsigned char *)err,
                ERR_SIZE, OCI_HTYPE_ERROR);
        fprintf(stderr, "OCILogon: failed: %d:%s\n",
                errcode, err);
        clean_up(1, *ocip);
        return 0;
    }
    printf("CONNECTED!\n");
    return 1;
}

static int have_data(int status) {
    return status == OCI_SUCCESS;
}

static t_list* send_query(t_list* list, t_oci* ocip) {
    int ret = OCI_ERROR;

    //statement 할당
    ret = OCIHandleAlloc(ocip->env_hp, (dvoid **)&(ocip->stmt_hp), 
                    OCI_HTYPE_STMT, 0, 0);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHandleAlloc: failed\n");
        clean_up(1, *ocip);
        return NULL;
    }

    text      sql_statement[BUF_SIZE] = {0, };
    t_list *node = list;
    int offset = 0;
    while (node) {
        memcpy(sql_statement + offset, node->data, node->size);
        offset += node->size;
        node = node->next;
    }
    
    //statement 준비
    ret = OCIStmtPrepare(ocip->stmt_hp, ocip->error_hp, sql_statement, 
                strlen((const char*)sql_statement), 
                OCI_NTV_SYNTAX, OCI_DEFAULT); 
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIStmtPrepare: failed\n");
        clean_up(1, *ocip);
        return NULL;
    }
    //set buffer pos                      
    char*       name_buffer     = malloc(sizeof(char) * BUF_SIZE);    
    ret = OCIDefineByPos(ocip->stmt_hp, &(ocip->define), ocip->error_hp, 
                (ub4)1, name_buffer, BUF_SIZE, 
                (ub2)SQLT_STR, 0, 0, 0, OCI_DEFAULT);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIDefineByPos: failed\n");
        clean_up(1, *ocip);
        return NULL;
    }

    int status;
    int rows = 1;
    //statement 실행
    status = OCIStmtExecute(ocip->service_hp, ocip->stmt_hp, ocip->error_hp,
                            rows, (ub4)0,
                            (CONST OCISnapshot*)NULL,
                            (OCISnapshot*)NULL, OCI_COMMIT_ON_SUCCESS);
    if (!OCI_CHKSUCC(status)) {
        fprintf(stderr, "OCIStmtExecute: failed\n");
        int errcode = -1;
        char err[ERR_SIZE];
        OCIErrorGet(ocip->error_hp, 1, NULL, 
                &errcode, (unsigned char *)err,
                ERR_SIZE, OCI_HTYPE_ERROR);
        fprintf(stderr, "OCILogon: failed: %d:%s\n",
                errcode, err);
        clean_up(1, *ocip);
        return NULL;
    }
    if (status == OCI_NO_DATA){
        printf("fetched none\n");
    } else {
        printf("fetched data\n");
    }

    list = NULL;
    if (have_data(status) == 0) {
        push_back(&list, "done!\n");
    }
    while (have_data(status)) {//check status
        push_back(&list, name_buffer);
        status = OCIStmtFetch(ocip->stmt_hp, ocip->error_hp, 
                            rows, OCI_FETCH_NEXT, OCI_DEFAULT);
    }
    free(name_buffer);
    OCIHandleFree(ocip->stmt_hp, OCI_HTYPE_STMT);
    return list;
}

static int sql_done(t_list* last_node) {
    char*   buf;

    buf = last_node->data;

    while (*buf && *buf != '\n')
        buf++;

    return *buf == '\n';
}

void    oracle_call_server(void* arg) {
    int ret;
    t_oci oci;
    memset(&oci, 0, sizeof oci);

    oci.client_socket = *(int*)arg;
    free(arg);
    oci.epoll_fd = epoll_create(OCI_EPOLL_SIZE);
    if (oci.epoll_fd == -1) {
        close(oci.client_socket);
        return ;
    }

    oci.event.events = EPOLLIN;
    oci.event.data.fd = oci.client_socket;
    ret = epoll_ctl(oci.epoll_fd, EPOLL_CTL_ADD, oci.client_socket, &(oci.event));
    if (ret != 0) {
        close(oci.epoll_fd);
        close(oci.client_socket);
        return ;
    }

    ret = set_connection(&oci);
    if (ret == 0) return ;
    
    t_list* list = NULL;
    
    while (1) {
        struct epoll_event  epoll_events[OCI_EPOLL_SIZE];
        int                 epoll_event_count;
        
        epoll_event_count = epoll_wait(oci.epoll_fd, epoll_events, EPOLL_EVENT_SIZE, -1);

        for (int i = 0; i < epoll_event_count; i++) {
            if (epoll_events[i].events && EPOLLIN) {
                char buf[BUF_SIZE] = {0, };
                int  read_bytes; 
                
                read_bytes = read(epoll_events[i].data.fd, buf, BUF_SIZE - 1);
                buf[read_bytes] = 0;
                if (read_bytes == 0) {
                    print_log("client disconnect\n");
                    clean_up(0, oci);
                    return ;
                }
                t_list* node = push_back(&list, buf);
                if (node == NULL) {
                    print_err("malloc failed on push_back()");
                    clean_up(0, oci);
                    return ;
                }

                if (sql_done(node)) {
                    t_list* tmp = list;
                    list = send_query(list, &oci);
                    if (list == NULL) return ;
                    
                    clear_list(tmp);

                    oci.event.events = EPOLLOUT;
                    int ret = epoll_ctl(oci.epoll_fd, EPOLL_CTL_MOD, oci.client_socket, &(oci.event));
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
                
                oci.event.events = EPOLLIN;
                int ret = epoll_ctl(oci.epoll_fd, EPOLL_CTL_MOD, oci.client_socket, &(oci.event));
            }
        }
    }
}
