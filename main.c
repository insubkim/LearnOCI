#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "oci.h"

#define OCI_CHKSUCC(c)  (c == OCI_SUCCESS || c == OCI_SUCCESS_WITH_INFO)


#define USER_ID     "insub"
#define USER_PW     "123"

#define SQL_SIZE        1024
#define ERR_SIZE        1024

int OCI_funtion_return = OCI_ERROR;

OCIEnv*     env_hp;
OCIError*   error_hp;
OCISvcCtx*  service_hp;
OCIServer*  server_hp;
OCIStmt*    stmt_hp = 0;
OCIDefine*  define = 0;  

char sql[SQL_SIZE];
char err[ERR_SIZE];

void    clean_up(int exit_code) {
    int ret = OCISessionEnd(service_hp, error_hp, 0, OCI_DEFAULT);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCISessionEnd: failed\n");
    }
    ret = OCIServerDetach(server_hp, error_hp, OCI_DEFAULT );
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIServerDetach: failed\n");
    }
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

int send_query(void) {
    int ret = OCI_ERROR;

    //statement 할당
    ret = OCIHandleAlloc(env_hp, (dvoid **)&stmt_hp, 
                    OCI_HTYPE_STMT, 0, 0);
    if (!OCI_CHKSUCC(ret)) {
        fprintf(stderr, "OCIHandleAlloc: failed\n");
        clean_up(1);
    }

    text      sql_statement[1024] = {0, };
    printf("%s", "sql 입력 :");
    gets(sql_statement);
    printf("[%s]\n", sql_statement);

    //statement 준비
    OCIStmtPrepare(stmt_hp, error_hp, sql_statement, 
                strlen((const char*)sql_statement), 
                OCI_NTV_SYNTAX, OCI_DEFAULT); 
    //set buffer pos
    ub4         name_buffer_len = 1023;                            
    char*       name_buffer     = malloc(sizeof(char) * (name_buffer_len + 1));    
    OCIDefineByPos(stmt_hp, &define, error_hp, 
                (ub4)1, name_buffer, name_buffer_len + 1, 
                (ub2)SQLT_STR, 0, 0, 0, OCI_DEFAULT);
    printf("\nbind 변수 갯수 입력 :");
    int bind_cnt = -1;
    scanf("%d", &bind_cnt);
    assert(bind_cnt != -1);

    for (int i = 0; i < bind_cnt; i++) {
        //bind variable
        OCIBind *bind_p = NULL;
        
        size_t  ename_len   = 127;
        text*   ename       = malloc(sizeof(char) * (ename_len + 1)); 
        memset(ename, 0, sizeof ename);
        
        size_t  evar_len    = 127;
        text*   evar        = malloc(sizeof(char) * (evar_len + 1));
        memset(evar, 0, sizeof evar);
        printf("입력 변수 name : value 입력\n");
        scanf("%s", ename);
        scanf("%s", evar);

        ret = OCIBindByName(stmt_hp, &bind_p, error_hp, 
                        (text*)ename, strlen(ename), 
                        evar, evar_len + 1, (ub2)SQLT_STR, 
                        0, 0, 0, 0, 0, OCI_DEFAULT);
        if (!OCI_CHKSUCC(ret)) {
            fprintf(stderr, "OCIHandleAlloc: failed\n");
            clean_up(1);
        }
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
        // printf("name : %s\n", ename);
        printf("fetched [%s]\n", name_buffer);
        status = OCIStmtFetch(stmt_hp, error_hp, 
                            rows, OCI_FETCH_NEXT, OCI_DEFAULT);
        printf("status : %d\n", status);
        // assert(status == OCI_NO_DATA);
    }
    OCIHandleFree(stmt_hp, OCI_HTYPE_STMT);
    return 1;
}

int main(void) {
    set_connection();
    send_query();
    clean_up(0);
}