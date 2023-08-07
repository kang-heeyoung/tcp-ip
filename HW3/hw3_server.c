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
#include <sys/time.h>
#include <sys/select.h>

#define BUF_SIZE 1024
#define READ_SIZE 256

char *path =".";

void error_handling(char *message);
int dir_cnt(int cnt);

typedef struct {
    unsigned int fileSize;
    unsigned int isFile;
    char fileName[100];
    char fileContent[BUF_SIZE];
} FileInfo;


int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock; // 서버, 클라 소켓
    char command[10], buf[BUF_SIZE], fileName[BUF_SIZE], fileInfo[BUF_SIZE];
    char clnt_fileName[BUF_SIZE], clnt_fileName_c[BUF_SIZE];
    int str_len, i;
    int fileNum;
    int send_cnt;

    char uploadFile_name[30] = {0};
    char uploadFile_content[BUF_SIZE];
    int uploadFile_size = 0;
    FILE* uploadFile_fd;

    struct sockaddr_in serv_adr, clnt_adr; // 서버, 클라 소켓 주소
    struct stat sb;
    socklen_t clnt_adr_sz;

    // 구조체 포인터 (파일 정보 저장)
    FileInfo str_fileInfo[BUF_SIZE];
    FileInfo* ptr_fileInfo;

    struct timeval timeout;
    fd_set reads, cpy_reads;
    int fd_max, fd_num;
    
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


    int cnt;
    int idx;
    int fd_check;

    struct dirent **namelist;
    // str_fileInfo[BUF_SIZE] = ((FileInfo) malloc(sizeof(FileInfo)));
    ptr_fileInfo = (FileInfo*) malloc(sizeof(FileInfo)*BUF_SIZE);
    // ptr_fileInfo = &str_fileInfo;

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

    // 서버-클라 송수신
    while(1){
        cpy_reads = reads;
        timeout.tv_sec=5;
        timeout.tv_usec=5000;
        fd_num=select(fd_max+1, &cpy_reads, 0, 0, &timeout);
        if(fd_num == -1){
            perror("select error : ");
            break;
        }
        if(fd_num==0){
            continue;
        }
        for(fd_check=0; fd_check<fd_max+1; fd_check++){
            if(FD_ISSET(fd_check, &cpy_reads)){
                if(fd_check==serv_sock){
                    clnt_adr_sz = sizeof(clnt_adr);
                    clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                    FD_SET(clnt_sock, &reads);
                    if(fd_max<clnt_sock){
                        fd_max=clnt_sock;
                    }
                }else{
                        // fileInfo 수신
                        clnt_sock = fd_check;
                        read(clnt_sock, &cnt, sizeof(cnt));
                        cnt =0;
                        int mode = R_OK | W_OK;
                        send_cnt = 0;
                        send_cnt = dir_cnt(cnt);
                        if((cnt = scandir(path, &namelist, NULL, NULL))== -1 )
                            {
                                error_handling("Directory Scan error");
                            }
                        // 배열 초기화
                        memset(fileName, 0, sizeof(fileName));
                        memset(fileInfo, 0, sizeof(fileInfo));
                        memset(str_fileInfo, 0, sizeof(str_fileInfo));

                        // 파일 목록 개수 보내기
                        write(clnt_sock, &send_cnt, sizeof(cnt));

                        // 파일 이름 및 파일 크기 보내기
                        for(idx=0; idx<cnt; idx++){
                            
                            strcpy(str_fileInfo[idx].fileName, namelist[idx]->d_name);  
                            // 파일 오픈

                            if(stat(str_fileInfo[idx].fileName, &sb) == -1){
                                error_handling("stat error");
                            } 

                            str_fileInfo[idx].fileSize = sb.st_size;

                            if(S_ISREG(sb.st_mode)){
                                str_fileInfo[idx].isFile = 1;
                            }else if(S_ISDIR(sb.st_mode)){
                                str_fileInfo[idx].isFile = 0;
                            }
                            if(access(str_fileInfo[idx].fileName, mode)== 0){
                                write(clnt_sock, (char*)&str_fileInfo[idx], BUF_SIZE);
                            }
                            
                            
                            
                        }


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
                            printf("End\n");
                            FD_CLR(fd_check,&reads);
                            close(fd_check);
                            exit(0);
                        }else if(!strcmp(command, "download\n")){ 
                            
                            // 변수 선언 & 초기화
                            FILE* file1;
                            // size_t fsize1;
                            fileNum = 0;
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

                        }else if(!strcmp(command, "cd\n")){
                            fileNum = 0;
                            read(clnt_sock, &fileNum, sizeof(int));

                            int ch = chdir(str_fileInfo[fileNum].fileName);

                            if(ch !=0){
                                error_handling("change directory error");
                            }

                        }else if(!strcmp(command, "upload\n")){
                            memset(uploadFile_content, 0, sizeof(uploadFile_content));
                            memset(uploadFile_name, 0, sizeof(uploadFile_name));

                            read(clnt_sock, &uploadFile_name, 30);
                            printf("upload file name :%s\n", uploadFile_name);
                            read(clnt_sock, &uploadFile_size, sizeof(int));

                            uploadFile_fd = fopen(uploadFile_name, "wb");

                            if(uploadFile_fd == NULL){
                                fputs("파일 업로드 오류", stderr);
                                fputc('\n', stderr);
                                continue;
                            }

                            int byteCheck =0;

                            while(byteCheck <= uploadFile_size){
                                read(clnt_sock, uploadFile_content, BUF_SIZE);

                                if(uploadFile_size < BUF_SIZE){
                                    fwrite(uploadFile_content, 1, uploadFile_size, uploadFile_fd);
                                }else{
                                    fwrite(uploadFile_content, 1, BUF_SIZE, uploadFile_fd);
                                }
                                byteCheck+=1024;
                            }
                            fclose(uploadFile_fd);
                            printf("업로드가 완료되었습니다.");
                        }else{
                            pause();
                        }
                    
                }
            }
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

int dir_cnt(int cnt) {
    int i;
    struct dirent **namelist;
    cnt = scandir(path, &namelist, NULL, NULL);
    i = 0;
    int mode = R_OK | W_OK;
    
    for(int x=0; x<cnt; x++){
        if(access(namelist[x]->d_name, mode) == 0){
            i++;                 
        }
    }
    // printf("i : %d\n", i);
    return i;
}