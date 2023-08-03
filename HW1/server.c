#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define READ_SIZE 256

const char *path =".";

void error_handling(char *message);

typedef struct {
    unsigned long int fileSize;
    char fileName[100];
    char fileContent[BUF_SIZE];
} FileInfo;


int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock; // 서버, 클라 소켓
    char command[10], dir[BUF_SIZE], buf[BUF_SIZE], fileName[BUF_SIZE], fileInfo[BUF_SIZE];
    char clnt_fileName[BUF_SIZE], clnt_fileName_c[BUF_SIZE];
    int str_len, i;

    struct sockaddr_in serv_adr, clnt_adr; // 서버, 클라 소켓 주소
    socklen_t clnt_adr_sz;


    // 구조체 포인터 (파일 정보 저장)
    FileInfo str_fileInfo[BUF_SIZE];
    FileInfo* ptr_fileInfo;

    if(argc !=2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    // 서버 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
    {
        error_handling("socket() error");
    }

    //주소정보 초기화&세팅
    memset(&serv_adr, 0 ,sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    //소켓 주소정보 할당 bind
    if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
    {
        error_handling("bind() error");
    }

    //서버소켓화 listen
    if(listen(serv_sock, 1)==-1)
    {
        error_handling("listen() error");
    }

    // 클라-서버 연결
    clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);    
    if(clnt_sock==-1)
    {  
        error_handling("accept() error");
    }else{
        printf("Connected client %d \n", i+1);
    }

    //디렉토리 모든 정보 받아오기
    int cnt;
    int idx;
    struct dirent **namelist;
    if((cnt = scandir(path, &namelist, NULL, NULL))== -1 )
    {
        error_handling("Directory Scan error");
    }
    // str_fileInfo[BUF_SIZE] = ((FileInfo) malloc(sizeof(FileInfo)));
    ptr_fileInfo = (FileInfo*) malloc(sizeof(FileInfo)*BUF_SIZE);
    // ptr_fileInfo = &str_fileInfo;

    // 서버-클라 송수신
    while(1)
    {
        memset(command, 0, sizeof(command));
        str_len=read(clnt_sock, command, 10);

        printf("client command : %s\n", command);
        if(str_len == -1){
            error_handling("command read error");
        }
        // q 명령어
        if(!strcmp(command, "q") || !strcmp(command, "Q\n")){
            int isExit = 0;
            read(clnt_sock, &isExit, sizeof(int));
            printf("------End------");
            close(clnt_sock);
            exit(0);
        }
        // fileInfo 명령어
        else if(!strcmp(command, "fileInfo\n")){
            // 파일 이름 정보 넣기
            char s[BUF_SIZE]={0,};
            FILE* file;
            size_t fsize;

            // 배열 초기화
            memset(fileName, 0, sizeof(fileName));
            memset(fileInfo, 0, sizeof(fileInfo));
            memset(str_fileInfo, 0, sizeof(str_fileInfo));

            // 파일 목록 개수 보내기
            write(clnt_sock, &cnt, sizeof(cnt));

            // 파일 이름 및 파일 크기 보내기
            for(idx=0; idx<cnt; idx++){
                strcpy(str_fileInfo[idx].fileName, namelist[idx]->d_name);  
                // *ptr_fileInfo[idx].fileName = &str_fileInfo[idx].fileName;
                // str_fileInfo[idx]->fileName = namelist[idx]->d_name;
                // printf("fileName : %s\n", str_fileInfo[idx].fileName);

                // printf("Check2\n");
                // 파일 오픈
                file = fopen(namelist[idx]->d_name, "rb");

                int isNul = 0;
                if(file == NULL){
                    error_handling("file NULL");
                }
                // 파일 크기 체크
                fseek(file, 0, SEEK_END);
                fsize = ftell(file);
                fseek(file, 0, SEEK_SET);

                fclose(file);

                str_fileInfo[idx].fileSize = fsize;
                 // 파일 정보 보내기
                write(clnt_sock, (char*)&str_fileInfo[idx], BUF_SIZE);
            }
           
            // write(clnt_sock, &cnt, sizeof(cnt));
            // write(clnt_sock, str_fileInfo, BUF_SIZE);
        // 해당 파일 전송
        }else if(!strcmp(command, "download\n")){ 
            
            // 변수 선언 & 초기화
            FILE* file1;
            size_t fsize1;
            int fileNum = 0;
            memset(clnt_fileName, 0, sizeof(clnt_fileName));
            memset(buf, 0, sizeof(buf));
            memset(clnt_fileName_c, 0, sizeof(clnt_fileName_c));
            
            // 파일 번호 받아오기 
            read(clnt_sock, &fileNum, sizeof(int));

            // 파일 열기
            strcat(clnt_fileName_c, str_fileInfo[fileNum].fileName);
            file1 = fopen(clnt_fileName_c, "rb");
            
            // 파일 존재 여부 확인
            int isNull = 0;
            if(file1 == NULL){
                isNull = 1;
                write(clnt_sock, &isNull, sizeof(isNull));
                fputs("파일이 없습니다.", stderr);
                fputc('\n', stderr);
                continue;
            }else{
                write(clnt_sock, &isNull, sizeof(isNull)); // 파일 존재 여부 전송

                // // 파일 데이터 읽어오기
                
                // 파일 전송
                int fileSize = 0;
                int fpsize = 1;
                while(1){
                    fpsize = fread(str_fileInfo[fileNum].fileContent, 1, BUF_SIZE, file1);
                    write(clnt_sock, str_fileInfo[fileNum].fileContent, BUF_SIZE);
                    // fileSize += fpsize;
                    if(fpsize < 1024){
                        break;
                    }
                }
                fclose(file1);
            }
        }else{
            close(clnt_sock);
            break;
        }
    }

    // 서버 소켓 닫기
    close(serv_sock);
    //메모리 해제
    for(i=0; i<cnt; i++){
        free(namelist[i]);
    }
    free(namelist);
    free(ptr_fileInfo);
    return 0;
}

// 예외처리
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}