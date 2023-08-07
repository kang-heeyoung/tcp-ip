#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUF_SIZE 1024
#define READ_SIZE 256

void error_handling(char *message);

typedef struct {
    unsigned long int fileSize;
    char fileName[BUF_SIZE];
    char fileContent[BUF_SIZE];
} FileInfo;


int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE] = {0};
    char fileName[BUF_SIZE], buf[BUF_SIZE], fileName_c[BUF_SIZE];
    // char fileInfo[BUF_SIZE]={0};
    // char temp[5];
    // int str_len;
    struct sockaddr_in serv_adr;
    int fileNum = 0;
    char file_num[10] = {0};

    FILE* file;
    // size_t fsize;

    FileInfo str_fileInfo[BUF_SIZE];
    // str_fileInfo = (FileInfo *) malloc(sizeof(FileInfo)*BUF_SIZE);

    if(argc!=3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // 소켓 생성
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock==-1)
    {
        error_handling("socket() error");
    }

    //주소정보 초기화&세팅
    memset(&serv_adr, 0 ,sizeof(serv_adr));
    serv_adr.sin_family=AF_INET;
    serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    // 서버-클라 연결요청
    if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
    {
        error_handling("connect() error");
    }else{
        puts("Connected.............");
    }

    while(1)
    {
        memset(message, 0, sizeof(message));
        printf("\n<명령어> fileInfo, download, quit(Q or q)\n");
        fputs("명령어를 입력해주세요 : ", stdout);
        fgets(message, BUF_SIZE, stdin);

        // 사용자의 입력값이 종료값인지 확인
        if(!strcmp(message, "q\n") || !strcmp(message, "Q\n"))
        {   
            write(sock, message, strlen(message));
            int isExit = 1;
            write(sock, &isExit, sizeof(int)); // 종료값 보내기
            puts("------End-------");
            break;
        }else if(!strcmp(message, "download\n"))
        {
            // 명령어 전송
            write(sock, message, strlen(message));

            // fileName, buf 메모리 초기화
            memset(fileName, 0, sizeof(fileName));
            memset(buf, 0, sizeof(buf));
            memset(fileName_c, 0, sizeof(fileName_c));

            // 파일번호 입력
            fputs("다운로드할 파일번호를 입력해주세요 : ",stdout);
            fgets(file_num, 10, stdin);

            fileNum = atoi(file_num);
            
            // 파일번호 전송
            write(sock, &fileNum, sizeof(int));

            // 파일 존재 여부 수신
            int isNull = 0;
            read(sock, &isNull, sizeof(isNull));
            if(isNull == 1){
                error_handling("해당 파일은 존재하지 않습니다.");
            }

            // int fileSize = 0;
            // // 파일 크기 수신
            // read(sock, &fileNum, sizeof(int));
            printf("파일 크기 %ld\n", str_fileInfo[fileNum].fileSize);

            // 파일 오픈 & 생성오류 체크
            strcat(fileName_c, str_fileInfo[fileNum].fileName);
            file = fopen(fileName_c, "wb");

            if(file == NULL){
                fputs("파일 생성 오류.", stderr);
                fputc('\n', stderr);
                continue;
            }

            int byteCheck = 0;
            // int writeCheck = 0;
            // 파일 내용 수신
            while(byteCheck <= str_fileInfo[fileNum].fileSize){
                read(sock, (char*)&str_fileInfo[fileNum].fileContent, BUF_SIZE);
                
                if(str_fileInfo[fileNum].fileSize < 1024){
                    fwrite(str_fileInfo[fileNum].fileContent, 1, str_fileInfo[fileNum].fileSize, file);
                }else{
                    fwrite(str_fileInfo[fileNum].fileContent, 1, BUF_SIZE, file);
                }

                byteCheck +=1024;
                // fprintf(file, buf);
            }
            fclose(file);
            printf("다운로드가 완료되었습니다.");
        }else if(!strcmp(message, "fileInfo\n")){
            //서버와 값 주고받기
            write(sock, message, strlen(message));

            int cnt=0;
            read(sock, &cnt, sizeof(int));
            printf("%d\n", cnt);

            printf("[폴더 내 파일 정보 & 크기]\n");
            for(int i=0; i<cnt; i++){
                read(sock, (char*)&str_fileInfo[i], BUF_SIZE);
                printf("[%d] : 파일이름 : %s, 파일크기 : %ld\n", i, str_fileInfo[i].fileName, str_fileInfo[i].fileSize);
            }

            // str_len=read(sock, fileInfo, BUF_SIZE-1);
            // fileInfo[str_len]=0;
            // printf("[폴더 내 파일 정보 & 크기]\n%s", fileInfo);
        }

        
    }
    close(sock);
    // free(str_fileInfo[BUF_SIZE]);
    return 0;

}

// 예외처리
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}