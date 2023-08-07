#define _XOPEN_SOURCE 200  
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

#define BUF_SIZE 1024

#define FILENAME "desktop.jpg"

typedef struct {
   int seq;
   int fileSize;
   char fileContent[BUF_SIZE];
} Pkt;

void error_handling(char *message);

// SIGALRM 시 호출되는 함수
void timeout(int sig){
   if(sig == SIGALRM){
      puts("Receiver End");
   }
   exit(1);
}


int main(int argc, char *argv[])
{
   int serv_sock;
   // char message[BUF_SIZE];
   // int str_len;
   socklen_t clnt_adr_sz;
   FILE* file;
   int pri_num =-2;

   // 구조체 변수 선언 & 초기화
   Pkt pkt;
   pkt.seq = 0;
   pkt.fileSize = 0;
   memset(pkt.fileContent, 0, sizeof(pkt.fileContent));

   // 알람 설정
   struct sigaction act;
   act.sa_handler = timeout;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   sigaction(SIGALRM, &act, 0);
  
   // socket 함수 생성
   struct sockaddr_in serv_adr, clnt_adr;
   if(argc!=2){
      printf("Usage : %s <port>\n", argv[0]);
      exit(1);
   }
   
   serv_sock=socket(PF_INET, SOCK_DGRAM, 0);
   if(serv_sock==-1)
      error_handling("socket() error");
   
   // 주소정보 저장
   memset(&serv_adr, 0, sizeof(serv_adr));
   memset(&clnt_adr, 0, sizeof(clnt_adr));
   serv_adr.sin_family=AF_INET;
   serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
   serv_adr.sin_port=htons(atoi(argv[1]));
   
   if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
      error_handling("bind() error");

   file = fopen(FILENAME, "wb");
   if(file == NULL){
      error_handling("파일 생성 오류");
   }

   int loss_check = 0;

   int str_len;
   while(1)
   {
      clnt_adr_sz = sizeof(clnt_adr);
      alarm(10);
      str_len = recvfrom(serv_sock, &pkt, sizeof(Pkt), 0, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
      if(str_len == -1){
         error_handling("receive error()");
      }

      // printf("%d, %d\n", pri_num, pkt.seq);
      // printf("%s", pkt.fileContent);
      printf("%d", pkt.fileSize);
      if(pri_num < pkt.seq){
         if(pkt.fileSize < BUF_SIZE){
            fwrite(pkt.fileContent, 1, pkt.fileSize, file);
            sendto(serv_sock, &pri_num, sizeof(int), 0, (struct sockaddr*)&clnt_adr, clnt_adr_sz);
            // printf("Receiver Ack seq : %d\n", pri_num);
            loss_check++;
            if(loss_check >1){
               break;
            }
         }else {
            fwrite(pkt.fileContent, 1, BUF_SIZE, file);
            // printf("file check1\n");
         }
         // printf("file check2\n");
      }
      pri_num = pkt.seq;
      sendto(serv_sock, &pri_num, sizeof(int), 0, (struct sockaddr*)&clnt_adr, clnt_adr_sz);
      // printf("receiver seq : %d\n", pri_num);
   }   
   fclose(file);
   close(serv_sock);
   return 0;
}

void error_handling(char *message)
{
   fputs(message, stderr);
   fputc('\n', stderr);
   exit(1);
}