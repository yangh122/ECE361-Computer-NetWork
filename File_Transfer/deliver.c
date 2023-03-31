#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <cassert>
#include <error.h>
#include <cerrno>
#include <time.h>
#include "packet_h.h"


void copy_filedata(char* from, char* to, int size, int index);
long get_file_size(FILE *stream);
int getLen(int num);
void makeMessage(struct packet* one_packet, char* message, int total_length);

int main(int argc, char const *argv[])
{



/*************initalize the buffers */

    const int BUFFER_SIZE = 100; // the buffer_size
    char buf [BUFFER_SIZE] = {0}; // buf to get the user input;
    char filename[BUFFER_SIZE] = {0}; // char to store the file name;
    char recv_buf[BUFFER_SIZE] = {0}; // buf to receive the message from the server





   

/*build udp socket*/
    int port = atoi(argv[2]);

    int socketfd;
    socketfd = socket(AF_INET,SOCK_DGRAM,0);
    if(socket < 0){
        printf("socket error\n");
        exit(1);
    }


    /* set address*/

    struct sockaddr_in addr_serv;
    int len;
    memset(&addr_serv,0,sizeof(addr_serv));// reset the value o
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(port);

    
    
    if(inet_aton(argv[1], &addr_serv.sin_addr) == 0){
        printf("IP Error\n");
        exit(1);
    }

    len = sizeof(addr_serv);
 


    /////////////////////////////// Set up time value and timeout;

    struct timeval time_out;
    time_out.tv_sec = 0;
    time_out.tv_usec = 1000;          //////////////////// set up timout value;
    



    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,&time_out,sizeof(time_out)) < 0) {
        perror("Error");
    }




    printf("Enter the input as the follow format\n");
    printf("ftp <filename>\n");

    while(1){
        fgets(buf, BUFFER_SIZE, stdin);
        if(buf[0] != 'f' || buf[1] != 't' || buf[2] != 'p' || buf[3] != ' ' || buf[4] == ' '){
            printf("Enter the input as the follow format\n");
            printf("ftp <filename>\n");
            memset(buf, 0, BUFFER_SIZE);
            continue;
        }
        break;
    }

    char* str1 = strtok(buf, " ");
    str1 = strtok(NULL, "\n");
    strcpy(filename, str1);

    if(access(filename, F_OK) == -1){
        printf("File does not exist\n");
        exit(1);
    }


    int recv_num;
    int send_num;



    clock_t start = clock();

    send_num = sendto(socketfd, "ftp", strlen("ftp"), 0, (struct sockaddr *)&addr_serv, sizeof(addr_serv));
    if(send_num == -1){
        printf("sendto error\n");
        exit(1);
    }

    printf("Send ftp successfully\n");

    memset(buf, 0 , BUFFER_SIZE); // reset the buffer;
    
    
    struct sockaddr_storage addr_servSt;
    socklen_t addr_servSt_size = sizeof(addr_servSt);

    recv_num = recvfrom(socketfd, recv_buf, BUFFER_SIZE, 0, (struct sockaddr *)&addr_servSt, &addr_servSt_size);


    clock_t end = clock();
    double clock_time = (double)(end - start);
    printf("The clock_time is %lf\n", clock_time);

    double wait_time = (clock_time/CLOCKS_PER_SEC) * 100000;

    if(recv_num == -1){
        printf("rece error\n");
        exit(1);
    }

    if(strcmp(recv_buf, "yes") == 0){
        printf("A file transfer can start.\n");
    }
    else{
        printf("A file transfer cant start.\n");
        exit(1);
    }

    memset(recv_buf, 0 , BUFFER_SIZE); // reset the buffer;


//***********************   Part2 Starts  **************************/
    char* send_buf;
    char read_file_buf[1001] = {0}; // buf to read the file data

    size_t bytes_read;
    FILE* fd_from;
    long total_size; // total size of the file
    int total_frag; // total frag number;
    int frag_number = 0; // current frag number;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = (long)wait_time * 1000;
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }



    fd_from = fopen(filename,"r");

    total_size = get_file_size(fd_from);

    

    if(total_size % 1000 == 0){
        total_frag = total_size/1000;
    }
    else{
        total_frag = total_size/1000 +1;
    }

    printf("total_size of %s is %d\n", filename, total_size);
    printf("total frag number of %s is %d\n", filename, total_frag);

    struct packet* packet_array = (struct packet*)malloc(sizeof(struct packet) * total_frag + sizeof(filename) * total_frag); /////// set pakcet_array

/************************Starts to Read File***********************/
    while((bytes_read = fread(read_file_buf,sizeof(char), sizeof(read_file_buf) - 1, fd_from)) > 0){


        struct packet new_packet;
        new_packet.total_frag = total_frag;
        new_packet.frag_no = frag_number + 1;
        new_packet.filename = (char*)malloc(sizeof(filename));
        strcpy(new_packet.filename, filename);
        new_packet.size = bytes_read;


        copy_filedata(read_file_buf,new_packet.filedata, sizeof(read_file_buf), 0);




        packet_array[frag_number] = new_packet;

        frag_number = frag_number+1;
        
        memset(read_file_buf, 0 , 1000);

    }
                         

/**************************** Starts to Send File**************************************/

    
    int totalLength;
    int total_frag_length;
    int frag_no_length;
    int size_length;
    int filename_length;
    int index_Length;

    send_buf = (char*)malloc(sizeof(char) * totalLength);

    int current_frag = 0;


    struct packet test_packet;

    while(current_frag < total_frag){
        

        total_frag_length = getLen(packet_array[current_frag].total_frag);
        frag_no_length = getLen(packet_array[current_frag].frag_no);
        size_length = getLen(packet_array[current_frag].size);
        filename_length = strlen(packet_array[current_frag].filename);
        index_Length = total_frag_length + frag_no_length + size_length + filename_length + 4;
        totalLength = total_frag_length + frag_no_length + size_length + filename_length + packet_array[current_frag].size + 4;
        
        makeMessage(&packet_array[current_frag], send_buf, index_Length);

        if(current_frag == 367){
            for(int i = 0; i < packet_array[current_frag].size; i++){
                printf("%c-------%c\n", packet_array[current_frag].filedata[i], send_buf[i + index_Length +1]);
            }
        }
        
        send_num = sendto(socketfd, send_buf, totalLength + 1, 0, (struct sockaddr *)&addr_serv, sizeof(addr_serv));
        if(send_num == -1){
            printf("sendto error\n");
            exit(1);
        }
        

        memset(send_buf, 0 , totalLength);



        printf("Send Packet %d, the size is %d\n",packet_array[current_frag].frag_no, packet_array[current_frag].size);

        recv_num = recvfrom(socketfd, recv_buf, BUFFER_SIZE, 0, (struct sockaddr *)&addr_servSt, &addr_servSt_size);

        if(recv_num == -1){
            printf("rece error\n");
            exit(1);
        }
        else if(strcmp(recv_buf, "ACK") == 0){
            current_frag = current_frag + 1;
            memset(recv_buf, 0 , BUFFER_SIZE); // reset the buffer;
        }
        else if(strcmp(recv_buf, "NACK") == 0){
            memset(recv_buf, 0 , BUFFER_SIZE); // reset the buffer;
        }
        else if(recv_num == 0){
            memset(recv_buf, 0 , BUFFER_SIZE); // reset the buffer;
        }
    }

    printf("All Packet_Send\n");

    free(packet_array);


    close(socketfd);
    fclose(fd_from);
    return 0;




}




long get_file_size(FILE *stream)
{
	long file_size = -1;
	long cur_offset = ftell(stream);

    
    fseek(stream, 0, SEEK_END);
	file_size = ftell(stream);
	fseek(stream, cur_offset, SEEK_SET);

	return file_size;
}

void copy_filedata( char* from, char* to, int size, int index){
    for(int i = 0; i < size; i++){
        to[index + i] = from[i];
    }
}

int getLen( int number){
    int count = 0;
    while(number != 0){
        number /= 10;
        count++;
    }
    return count;
}

void makeMessage(struct packet* one_packet, char* message, int index_length){

    char total_frag_ToChar[1000] = {0};
    char frag_no_ToChar[1000] = {0};
    char size_ToChar[1000] = {0};
    memset(total_frag_ToChar, 0, 1000);
    memset(frag_no_ToChar, 0, 1000);
    memset(size_ToChar, 0, 1000);

    sprintf(total_frag_ToChar, "%d", one_packet->total_frag);
    sprintf(frag_no_ToChar, "%d", one_packet->frag_no);
    sprintf(size_ToChar, "%d", one_packet->size);
    int size_total_frag = strlen(total_frag_ToChar);
    int size_frag_no = strlen(frag_no_ToChar);
    int size_size = strlen(size_ToChar);
    int size_name = strlen(one_packet->filename);


    int offset = 0;
    //3:
    memcpy(message, total_frag_ToChar, size_total_frag);

    offset += size_total_frag;
    memcpy(message + offset, ":", 1);
    offset += + 1;
    //3:2:
    memcpy(message + offset, frag_no_ToChar, size_frag_no);
    offset += size_frag_no;
    memcpy(message + offset, ":", 1);
    offset += + 1;
    //3:2:10:
    memcpy(message + offset, size_ToChar, size_size);
    offset += size_size;
    
    memcpy(message + offset, ":", 1);
    offset += + 1;
    //3:2:10:foobar.txt:
    memcpy(message + offset, one_packet->filename, size_name);
    offset += size_name;
    memcpy(message + offset, ":", 1);
    offset += + 1;
    //data

    //printf("%s\n", message);

    copy_filedata(one_packet->filedata, message, one_packet->size, index_length + 1);



    return; 
    
}
