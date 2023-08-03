#define _CRT_SECURE_NO_WARNINGS
#define CLOCKS_PER_SEC
#define _XOPEN_SOURCE 200  

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
   unsigned long int fileSize;
   char fileContent[BUF_SIZE];
} Ack;

void error_handling(char *message);

// SIGALRM 시 호출되는 함수
void timeout(int sig){
   if(sig == SIGALRM){
      puts("loss");
   }
   alarm(2);
}

int main(int argc, char *argv[])
{
   int sock;

   socklen_t serv_adr_sz;

   FILE* file;
   size_t  fsize;

   int fpsize = 0;
   int check = 0;

   double start, end;

   // // 구조체 변수 선언 & 초기화
   Ack ack;
   ack.seq = -1;
   ack.fileSize = 0;
   memset(ack.fileContent, 0, sizeof(ack.fileContent));
    
   // 알람 설정
   struct sigaction act;
   act.sa_handler = timeout;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   sigaction(SIGALRM, &act, 0);
   
   // 소켓 생성
   struct sockaddr_in serv_adr;
   if(argc!=3){
      printf("Usage : %s <IP> <port>\n", argv[0]);
      exit(1);
   }
   
   sock=socket(PF_INET, SOCK_DGRAM, 0);   
   if(sock==-1)
      error_handling("UDP socket creation error");
   
   // 주소 정보 저장
   memset(&serv_adr, 0, sizeof(serv_adr));
   serv_adr.sin_family=AF_INET;
   serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
   serv_adr.sin_port=htons(atoi(argv[2]));

   // 파일 오픈
   file = fopen(FILENAME, "rb");
   if(file == NULL){
      error_handling("파일이 없습니다.");
   }

   // 파일 크기 저장
   fseek(file, 0, SEEK_END);
   fsize = ftell(file);
   fseek(file, 0, SEEK_SET);
   ack.fileSize = fsize;

   int seq_check = -1;
   
   while(1) 
   {
      serv_adr_sz=sizeof(serv_adr);
      fpsize = fread(ack.fileContent, 1, BUF_SIZE, file);
      ack.fileSize = fpsize;
      while(1){
         if(fpsize<1024){
            // printf("check2\n");
            // printf("%s\n", ack.fileContent);
            sendto(sock, &ack, sizeof(Ack), 0, (struct sockaddr*)&serv_adr, serv_adr_sz);
            check = 1;
         }else{
            sendto(sock, &ack, sizeof(Ack), 0, (struct sockaddr*)&serv_adr, serv_adr_sz);
         }
         alarm(1);
         seq_check = recvfrom(sock, &ack.seq, sizeof(int), 0, (struct sockaddr*)&serv_adr, &serv_adr_sz);
         if(ack.seq == 0){
            start = clock();
         }
         printf("seq : %d, loss check: %d\n", ack.seq, seq_check);
         if(seq_check >=0){
            break;
         }
      }
      ack.seq++;
      if(check==1){
         end = (double)clock();
         printf("Throughput = %.1lf (bytes/ms)\n", (double)fsize/(end-start));
         break;
      }
   }   
   fclose(file);
   close(sock);
   return 0;
}

void error_handling(char *message)
{
   fputs(message, stderr);
   fputc('\n', stderr);
   exit(1);
}