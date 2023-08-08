#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>

#define path "data.txt"
#define BUF_SIZE 100
#define MAX_CLNT 256

// 클라이언트에 보낼 data
typedef struct sendData{
    char data[100];
    int cnt;
}sendData;

// trie에서 중복된 값의 완성 검색어와 검색횟수 저장
typedef struct dataInfo{
    char data[100];
    int cnt;
}dataInfo;

// node
typedef struct node{
    struct node* child[26]; // node
    // char *data[100]; 
    struct dataInfo dataArr[100]; // 저장된 단어 struct(완전한 단어 str)
    int word; // node의 끝인지 확인
    int cnt; // 검색횟수
    int num; // 저장된 단어 개수
}node;

void error_handling(char *message);

node* newNode();
void fileRead();
void showtree(node* now, char* str, int depth);
void insert(node* root, char* str, int cnt, const char* data);
int search(node* root, const char* str);
void recommend(node* root, char* str);
void suffixList(node* root, int cnt, const char* str, const char* data);
void sort();

void* handle_clnt(void *arg);
void send_msg(char * msg, int len);

// 서버 소켓 생성
int servSocket(char* argv);


// 검색된 검색어 개수
int data_num = 0;
// 검색된 검색어와 검색횟수 
sendData send_data[10];

node* root;

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char *argv[]){
    // char tmp[100];
    root = newNode();

    int serv_sock, clnt_sock;
    struct sockaddr_in clnt_adr;
    socklen_t clnt_adr_sz;
    pthread_t t_id;

    pthread_mutex_init(&mutx, NULL);
    serv_sock = servSocket(argv[1]);

    memset(send_data, 0, sizeof(send_data));
    // if(argc !=2){
    //     error_handling("input error");
    // }
    fileRead(root);

    while(1){
        clnt_adr_sz=sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++]=clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    // CHECK: send_data에 들어간 값 main에서 확인
    // for(int i=0; i<data_num; i++){
    //     printf("%d %d: %s, %d\n", i, data_num, send_data[i].data, send_data[i].cnt);
    // }
    
    
    return 0;
}

void error_handling(char *message)
{
   fputs(message, stderr);
   fputc('\n', stderr);
   exit(1);
}

// init
node* newNode() {
    node* new = (node *)malloc(sizeof(node));
    new->word = 0;
    new->num = 0;
    dataInfo* data = (dataInfo*)malloc(sizeof(dataInfo));
    data->cnt = 0;
    memset(data->data, 0 ,sizeof(data->data));
    // new->dataArr = (dataInfo*)malloc(sizeof(dataInfo));
    memset(new->dataArr, 0,sizeof(new->dataArr));
    for(int i=0; i<26; i++){
        new->child[i]=0;
    }
    return new;
}

// trie 값 넣기
void insert(node* root, char* str, int cnt, const char * data){
    int len = strlen(str);
    node* now = root;
    char data_cp[100];
    strcpy(data_cp, (char*)data);

    for(int i=0; i<len; i++){
        if(str[i] >=65 && str[i] <=90){
            str[i] += 32;
        }
        if(!now->child[str[i]-'a']){
            now->child[str[i]-'a']=newNode();
        }
        now=now->child[str[i]-'a'];
    }
    now->word=1;
    
    now->dataArr[now->num].cnt = cnt;
    strcpy(now->dataArr[now->num].data, data_cp);
    // CHECK: trie에 들어가는 값 확인하는 print
    // printf("%d: %s -> %s\n", now->num, str, now->dataArr[now->num].data);

    now->num++;
}

// data.txt 파일 읽기
void fileRead(node* root) {

    FILE *fp = NULL;
    char str[100];
    char str_cp[100];
    char *p_str[10];
    char txt_str[100];
    int cnt;
    char *p;
    int i=0;

    fp=fopen(path, "r");    
    if(fp == NULL){
        error_handling("file open error");
    }

    while (!feof(fp)){
        memset(str, 0, sizeof(str));
        i=0;
        memset(p_str, 0, sizeof(p_str));
        memset(txt_str, 0, sizeof(txt_str));

        fgets(str, 100, fp);

        p = strtok(str, " ");
        while(p !=NULL){
            p_str[i] = p;
            p = strtok(NULL, " ");
            i++;
        }

        cnt = atoi((char*)p_str[i-1]);
        strcat(txt_str, str);
        strcat(txt_str, " ");
        for(int j=1; j<i-1; j++){
            strcpy(str_cp, (char*)p_str[j]);
            strcat(str, str_cp);
            strcat(txt_str, str_cp);
        }
        printf("%s\n", txt_str);
        // insert(root, str, cnt, txt_str);
        suffixList(root, cnt, str, txt_str);
    }
    fclose(fp);
}

// trie 전체값 보여주기
void showtree(node* now, char* str, int depth){
    if(now->word){
        for(int i=0; i<now->num; i++){
            // CHECK: trie에서 검색한 값 출력
            // printf("check\n");
            // printf("%s\n", str);
            // printf("%s, %d\n", now->dataArr[i].data, now->dataArr[i].cnt);
            if(data_num < 10){
                send_data[data_num].cnt = now->dataArr[i].cnt;
                strcpy(send_data[data_num].data, now->dataArr[i].data);
                //CHECK: trie에서 찾은 값 send_data에 들어가는지 확인하는 출력값
                // printf("%d, %s, %d\n", i, send_data[data_num].data, send_data[data_num].cnt);
                data_num++;
            }else{
                sort();
                if(send_data[data_num-1].cnt <now->dataArr[i].cnt){
                    strcpy(send_data[data_num-1].data, now->dataArr[i].data);
                    send_data[data_num-1].cnt = now->dataArr[i].cnt;
                }
            }
        }
    }

    for(int i=0; i<26; i++){
        if(now->child[i]){
            str[depth]=i+'a';
            str[depth+1]=0;
            showtree(now->child[i], str, depth+1);
        }
    }
}

// 단어인지 아닌지 학인
int search(node* root, const char* str){
    int len = strlen(str);
    node* now = root;

    for(int i=0; i<len; i++){
        if(!now->child[str[i]-'a']){
            return 0;
        }
        now = now->child[str[i]-'a'];
    }
    return now->word;
}

// 탐색
void recommend(node* root, char* str){
    int len = strlen(str);
    node* now = root;
    data_num = 0;
    for(int i=0; i<len; i++){
        if(!now->child[str[i]-'a']){
            return;
        }
        now=now->child[str[i]-'a'];
    }
    showtree(now, str, len);
}
// suffix char 만들기
void suffixList(node* root, int cnt, const char* str, const char* data) {
    node* now = root;
    char suffixList[100];
    char suffixList_cp[100];
    int len = strlen(str);
    strcpy(suffixList_cp, (char*)str);


    for(int i=0; i<len; i++){
        strcpy(suffixList, suffixList_cp+i);
        insert(now, suffixList, cnt, data);
    }
}

// 클라에게 보낼 값 정렬해주는 함수
void sort() {
    sendData cp_send_data;
    int i=0, j=0;
    for(i=data_num; i>1; i--){
        for(j=1; j<i; j++){
            if(send_data[j-1].cnt < send_data[j].cnt){
                cp_send_data = send_data[j-1];
                send_data[j-1] = send_data[j];
                send_data[j]=cp_send_data;
            }
        }
    }
}

// 서버소켓 생성 
int servSocket(char* argv){
    int serv_sock;
    struct sockaddr_in serv_addr;
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(atoi(argv));

    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("bind() error");
    
    if(listen(serv_sock,  5)== -1)
        error_handling("listen() error");
    
    return serv_sock;

}

void * handle_clnt(void *arg){
    int clnt_sock=*((int*)arg);
    int str_len = 0;
    char msg[100];

    while(1){
        memset(msg,0,sizeof(msg));
        // printf("start : %s\n", msg);
        str_len = read(clnt_sock, msg, sizeof(msg));
        if(str_len == 0){
            break;
        }
        //CHECK: 클라에서 넘겨준 검색어
        // printf("after read : %s %d\n", msg, str_len);
        recommend(root, msg);
        sort();
        send_msg(msg,str_len);
    }

    pthread_mutex_lock(&mutx);
    for(int i=0; i<clnt_cnt; i++){
        if(clnt_sock == clnt_socks[i]){
            while(i++<clnt_cnt-1)
                clnt_socks[i]=clnt_socks[i+1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}

void send_msg(char * msg, int len){
    pthread_mutex_lock(&mutx);
    for(int i=0; i<clnt_cnt; i++){
        write(clnt_socks[i], &data_num, sizeof(int));
        for(int j=0; j<data_num; j++){
            //CHECK : 보내는 데이터 값
            // printf("%s, %d\n", send_data[j].data, send_data[j].cnt);
            write(clnt_socks[i], &send_data[j], sizeof(send_data[j]));
        }
        // write(clnt_socks[i], msg, len);
    }
    pthread_mutex_unlock(&mutx);
}
