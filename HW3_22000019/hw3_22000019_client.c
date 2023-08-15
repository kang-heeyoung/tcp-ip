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

const char *path = ".";

void error_handling(char *message);

typedef struct {
    unsigned int fileSize;
    unsigned int isFile;
    char fileName[BUF_SIZE];
    char fileContent[BUF_SIZE];
} FileInfo;


int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE] = {0};
    char fileName[BUF_SIZE], buf[BUF_SIZE], fileName_c[BUF_SIZE];
    int str_len, cnt;
    struct sockaddr_in serv_adr;
    int fileNum = 0;
    char file_num[10] = {0};
    char uploadFileName[30] = {0};

    int upload_size = 0;
    int fpsize = 0;
    char uploadContent[BUF_SIZE];

    FILE* file;
    FILE* upload_file;
    FileInfo str_fileInfo[BUF_SIZE];

    struct dirent **namelist;
    struct stat sb;
    
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
        cnt=0;
        // printf("%d\n", cnt);
        printf("\n-----------------------------------------\n");
        printf("[폴더 내 파일 정보 & 크기]\n");
        write(sock, &cnt, sizeof(int));
        read(sock, &cnt, sizeof(int));
        for(int i=0; i<cnt; i++){
            str_len = recv(sock, (char*)&str_fileInfo[i], sizeof(str_fileInfo), MSG_PEEK);
            while(str_len < BUF_SIZE){
                str_len = recv(sock, (char*)&str_fileInfo[i], sizeof(str_fileInfo), MSG_PEEK);
            }
            read(sock, (char*)&str_fileInfo[i], BUF_SIZE);
            // printf("str_len: %d\n", str_len);
            // printf("cnt : %d, i: %d\n",cnt, i);
            // printf("%d\n", str_fileInfo[i].isFile);
            if(str_fileInfo[i].isFile == 1){
                printf("[%d] File - 이름 : %s, 크기 : %d\n", i, str_fileInfo[i].fileName, str_fileInfo[i].fileSize);
            }else{
                printf("[%d] Dir - 이름 : %s, 크기 : %d\n", i, str_fileInfo[i].fileName, str_fileInfo[i].fileSize);
            }
        }

        memset(message, 0, sizeof(message));
        printf("\n<명령어> cd, download, upload quit(Q or q)\n");
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
            printf("파일 크기 %d\n", str_fileInfo[fileNum].fileSize);

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
                // printf("%s\n", str_fileInfo[fileNum].fileContent);
                
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
        }else if(!strcmp(message, "cd\n")){
            write(sock, message, strlen(message));

            while(1){
                // 파일번호 입력
                fputs("이동할 폴더번호를 입력해주세요 : ",stdout);
                fgets(file_num, 10, stdin);

                fileNum = atoi(file_num);

                if(str_fileInfo[fileNum].isFile == 0){
                    break;
                }else{
                    printf("폴더번호를 입력해주세요\n");
                    continue;
                }
            
            }
            // 파일번호 전송
            write(sock, &fileNum, sizeof(int));
        }else if(!strcmp(message, "upload\n")){
            // FILE* cl_file;
            write(sock, message, strlen(message));
            
            fileNum = 0;
            int cnt = 0;

            cnt = scandir(path, &namelist, NULL, NULL);
            if(cnt == -1){
                error_handling("Directory Scan error");
            }

            printf("\n[파일 목록]\n");
            for(int i =0; i<cnt; i++){
                if(stat(namelist[i]->d_name, &sb) == -1){
                    error_handling("stat error");
                }
                if(S_ISREG(sb.st_mode)){
                    printf("[%d] File - 파일 이름 : %s\n", i,namelist[i]->d_name);
                }else if(S_ISDIR(sb.st_mode)){
                    printf("[%d] Dir - 폴더 이름 : %s\n", i,namelist[i]->d_name);
                }   
            }

            while(1){
                fputs("업로드할 파일 이름을 입력해주세요 : ", stdout);
                fgets(uploadFileName, 30, stdin);
                uploadFileName[strlen(uploadFileName)-1]='\0';
                stat(uploadFileName, &sb);

                if(S_ISREG(sb.st_mode)){
                    break;
                }else if(S_ISDIR(sb.st_mode)){
                    continue;
                }   
            }

            write(sock, uploadFileName, 30);

            upload_size = sb.st_size;

            write(sock, &upload_size, sizeof(int));
            upload_file = fopen(uploadFileName, "rb");

            if(upload_file == NULL){
                fputs("파일이 없습니다.", stderr);
                fputc('\n', stderr);
            }
            int fileSizeCheck = 0;

            while(1){
                fpsize = fread(uploadContent, 1, BUF_SIZE, upload_file);
                write(sock, uploadContent, fpsize);

                if(fileSizeCheck == upload_size){
                    printf("파일 전송이 완료되었습니다.\n");
                    break;
                }
                fileSizeCheck += fpsize;
                printf("%d, %d\n", fpsize, upload_size);
            }
            fclose(upload_file);
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