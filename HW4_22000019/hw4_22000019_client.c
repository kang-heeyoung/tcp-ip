#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#define BUF_SIZE 100
#define COLOR_RED "\033[38;2;255;0;0m"
#define COLOR_RESET "\033[0m"

typedef struct recvData{
    char data[100];
    int cnt;
}recvData;


void error_handling(char *message);
void* send_msg(void *arg);

void highLight(char* recv_chr, char* msg);
// void* recv_msg(void *arg);
// void exclusiveprint();

int clntSocket(char* argv1, char* argv2);

int getch();

int recv_data_num = 0;
recvData recv_data;

// pthread_mutex_t mutx1;
// pthread_mutex_t mutx2;


int main(int argc, char *argv[]){

    int sock;
    pthread_t snd_thread;
    void * thread_return;

    if(argc !=3){
        error_handling("input error");
    }

    sock = clntSocket(argv[1], argv[2]);
    // pthread_mutex_init(&mutx2, NULL);
    // pthread_mutex_init(&mutx1, NULL);

    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
    // pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
    pthread_join(snd_thread, &thread_return);
    // pthread_join(rcv_thread, &thread_return);
    close(sock);
    
    
    return 0;
}

void error_handling(char *message)
{
   fputs(message, stderr);
   fputc('\n', stderr);
   exit(1);
}
int clntSocket(char* argv1, char* argv2){
    int sock;
    struct sockaddr_in serv_addr;

    sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr(argv1);
    serv_addr.sin_port=htons(atoi(argv2));

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("connect() error");

    return sock;
}

void* send_msg(void * arg){
    int sock=*((int*)arg);
    int str_len;
    int key_value_int;
    char key_value_char[100];
    char msg[100];
    char input[100];
    printf("search data: ");
    while(1){
        // printf("search data: ");
        recv_data_num = 0;
        // fgets(msg, 100, stdin);
        key_value_int = getch();
        write (1, "\033[2J\033[1;1H", 11);
        // backspace
        if(key_value_int == 127){
            msg[strlen(msg)-1]='\0';
            input[strlen(input)-1]='\0';
            // 소문자
        }else if(key_value_int >=97 && key_value_int <=122){
            sprintf(key_value_char, "%c", key_value_int);
            strcat(msg, key_value_char);
            strcat(input, key_value_char);
            //대문자
        }else if(key_value_int >=65 && key_value_int <=90){
            sprintf(key_value_char, "%c", key_value_int);
            strcat(input, key_value_char);
            key_value_int +=32;
            sprintf(key_value_char, "%c", key_value_int);
            strcat(msg, key_value_char);
            // space
        }else if(key_value_int == 32){
            sprintf(key_value_char, "%c", key_value_int);
            strcat(input, key_value_char);
            //esc
        }else if(key_value_int == 27){
            close(sock);
            exit(0);
        }
        printf("search data: %s\n", input);
        //CHECK: 입력받은 문자열 값 체크
        // printf("msg : %s\n", msg);
        // pthread_mutex_unlock(&mutx2);
        if(msg[0] !='\0'){
            write(sock, msg, strlen(msg));

            read(sock, &recv_data_num, sizeof(int));
            // printf("recv : %d\n", recv_data_num);
            for(int i=0; i<recv_data_num; i++){
                memset(&recv_data, 0, sizeof(recv_data));
                str_len=read(sock, &recv_data, sizeof(recv_data));
                if(str_len == -1) 
                    return (void*) -1;
                highLight(recv_data.data, input);
                // printf("%s\n", recv_data.data);
            }
        }
        // exclusiveprint();
    }
    return NULL;
}

int getch()
{
    int c;
    struct termios oldattr, newattr;

    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);
    newattr.c_cc[VMIN] = 1;
    newattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    c = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    return c;
}

void highLight(char* recv_chr, char* msg){
    char cp_recv_data[100];
    char* ptr1;
    char* ptr2;
    // char hightLight_data[100];
    int data_len = strlen(recv_chr);
    int msg_len = strlen(msg);
    int check=0;
    ptr2 = &cp_recv_data[0];

    strcpy(cp_recv_data, recv_chr);
    for(int i=0; i<data_len; i++){
        if(isupper(cp_recv_data[i]))
            cp_recv_data[i]=tolower(cp_recv_data[i]);
    }
    ptr1 = strstr(cp_recv_data, msg);

    for(int i=0; i<data_len; i++){
        if(ptr1 == ptr2){
            check++;
            // printf("check\n");
            printf("%s%c%s", COLOR_RED, recv_chr[i],COLOR_RESET);
            if(msg_len > check){
                ptr1++;
            }
            
        }else{
            printf("%c", recv_chr[i]);
        }
        ptr2++;
    }
    printf("\n");
    // printf("strstr : %s\n",strstr(cp_recv_data, msg));
}