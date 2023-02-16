#include <stdio.h>   
#include <sys/types.h>   
#include <sys/socket.h>   
#include <netinet/in.h>   
#include <unistd.h>   
#include <errno.h>   
#include <string.h>   
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <error.h>
#include <time.h>

#include "packet_h.h"


void decodeMessage(char* from, struct packet* to);

int main(int argc, char const*argv[])
{

if(argc != 2){
    printf("Input Format Error\n");
    exit(0);
}

/*build udp socket*/
int port = atoi(argv[1]);
int socketfd;
socketfd = socket(AF_INET,SOCK_DGRAM,0);
if(socketfd < 0){
    printf("socket error");
    exit(1);
}

/*bind to socket*/
struct sockaddr_in serv_addr;
int len;
memset(&serv_addr, 0 , sizeof(struct sockaddr_in));
serv_addr.sin_family = AF_INET;
serv_addr.sin_port = htons(port);
serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
len = sizeof(serv_addr);

if(bind(socketfd, (struct sockaddr *)&serv_addr, len) == -1){
    printf("bind error\n");
    exit(1);
}

int recv_num;
int send_num;
char buf[BUF_SIZE] = {0};
char recv_buf[1200] = {0};


struct sockaddr_storage deliver_addr;
socklen_t deliver_size = sizeof(deliver_addr);

recv_num = recvfrom(socketfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&deliver_addr, &deliver_size);

printf("Receive Message\n");

if(recv_num == -1){
    printf("recv error");
    exit(1);
}



if(strcmp(recv_buf, "ftp") == 0){
    send_num = sendto(socketfd, "yes", strlen("yes"), 0 , (struct sockaddr *) &deliver_addr, deliver_size);
    if(send_num == -1){
        printf("sendto yes error\n");
        exit(1);
    }
    printf("send Yes\n");
}
else{
    send_num = sendto(socketfd, "no", strlen("no"), 0 , (struct sockaddr *) &deliver_addr, deliver_size);
        if(send_num == -1){
        printf("sendto no error\n");
        exit(1);
    }
    printf("send No\n");
}


memset(recv_buf, 0 , BUF_SIZE);
struct packet receive_packet;

recv_num = recvfrom(socketfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&deliver_addr, &deliver_size);

printf("\nStart Reciving File\n\n");
decodeMessage(recv_buf, &receive_packet);



//////test file////////
FILE* fd_file = fopen(receive_packet.filename,"w+");

size_t bytes_writeen = fwrite(receive_packet.filedata, sizeof(char), receive_packet.size, fd_file);
memset(recv_buf, 0 , BUF_SIZE);

while(1){

    send_num = sendto(socketfd, "ACK", strlen("ACK"), 0 , (struct sockaddr *) &deliver_addr, deliver_size);
    if(receive_packet.frag_no == receive_packet.total_frag){

        break;
    }

    recv_num = recvfrom(socketfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&deliver_addr, &deliver_size);
    decodeMessage(recv_buf, &receive_packet);
    printf("Successfuly receive packet %d \n", receive_packet.frag_no);
    size_t bytes_writeen = fwrite(receive_packet.filedata, 1, receive_packet.size, fd_file);

    memset(recv_buf, 0 , BUF_SIZE);

}


printf("Finished\n");

fclose(fd_file);
close(socketfd);
return 0;
}



void decodeMessage(char* from, struct packet* to){

    int index = 0;
    for(int i=0; i<1100; i++){
        if(from[i]==':'){
            index++;
        }
        if(index==4){
            index=i+1;
            break;
        }
    }
    
    


    to->total_frag = atoi(strtok(from, ":"));
    to->frag_no = atoi(strtok(NULL, ":"));
    to->size = atoi(strtok(NULL, ":"));
    to->filename = strtok(NULL, ":");

    for(int i=0; i< to->size; i++){
        to->filedata[i] = from[i+index +1];
    }

    return;

}
