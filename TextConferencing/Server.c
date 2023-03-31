#include <stdio.h>   
#include <string.h>   
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>   
#include <sys/socket.h>  
#include <sys/time.h>
#include <netinet/in.h>   
#include <unistd.h>   
#include <errno.h>   
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <error.h>
#include <time.h>

#include "message.h"

//#define MAX_CLIENT 3

struct client{
    //int list_number;
    bool in_session;
    char session_id[100];
    //char* ip;
    //int port;
};
struct session{
    char id[100];
    int num_members;
    //int list_number;
    struct session* next;    
};
struct session* head = NULL;

//char* client_ID[] = {"charles","haochang","ta"};
//char* client_password[] = {"aaa","bbb","ccc"};
int client_connected[] = {0, 0, 0};
int client_fd[] = {-1, -1, -1};
struct client* client_list[MAX_CLIENT];

fd_set readfds;
fd_set master;

void decodeMessage(char* from, struct message* to);
void decodeMessage(char* from, struct message* to){
    int source_index = 0;
    for(int i=0; i<1100; i++){
        if(from[i]==':'){
            source_index++;
        }
        if(source_index==2){
            source_index=i+1;
            break;
        }
    }
    
    int data_index = 0;
    for(int i=0; i<1100; i++){
        if(from[i]==':'){
            data_index++;
        }
        if(data_index==3){
            data_index=i+1;
            break;
        }
    }

    to->type = atoi(strtok(from, ":"));
    to->size = atoi(strtok(NULL, ":"));
    int source_size = strlen(strtok(NULL, ":"));

    for(int i=0; i< source_size; i++){
        to->source[i] = from[i + source_index + 1];
    }

    for(int i=0; i< to->size; i++){
        to->data[i] = from[i + data_index + 1];
    }
}

void decodeMessage2(char* from, struct message* to);
void decodeMessage2(char* from, struct message* to){
    to->type = atoi(strtok(from, ":"));
    to->size = atoi(strtok(NULL, ":"));
    strcpy(to->source , strtok(NULL, ":") );
    strcpy(to->data , strtok(NULL, "\0")  );
}


// LOGIN       1
// #define LO_ACK      2
// #define LO_NAK      3
// EXIT        4
// JOIN        5
// #define JN_ACK      6
// #define JN_NAK      7
// LEAVE_SESS  8
// NEW_SESS    9
// #define NS_ACK      10
// MESSAGE     11
// QUERY       12
// #define QU_ACK      13
void react(int fd, struct message* msg){
    printf("reacting: ");
    int recv_num = -1;
    int send_num = -1;
    char send_buf[BUF_SIZE] = {0};
    char recv_buf[BUF_SIZE] = {0};
    memset(recv_buf, 0 , BUF_SIZE);
    memset(send_buf, 0 , BUF_SIZE);

    if(msg->type == LOGIN){
        printf("login...\n");
        char ID[100] = {0}; 
        char password[100] = {0}; 
        strcpy(ID, strtok(msg->data, " "));
        strcpy(password, strtok(NULL, " "));
        printf("ID: %s\nPassword: %s\n", ID, password);
        bool success = false;

        for(int i=0;i<MAX_CLIENT;i++){
            //printf("dd: %d\n",i);
            if( (!strcmp(client_ID[i], ID)) && (!strcmp(client_password[i], password)) && client_connected[i]==0){
                //LO_ACK 
                client_connected[i] = 1;
                client_list[i]->in_session = false;
                client_fd[i] = fd;
                send_num = send(fd, "2:1:a:a", strlen("2:1:a:a"), 0);

                printf("LO_ACK sent.\n");
                success = true;
                break;
            }
        }
        if(!success){
            //LO_NAK 
            send_num = send(fd, "3:1:a:a", strlen("3:1:a:a"), 0);
            printf("LO_NAK sent.\n");
        }
    }
    else if(msg->type == EXIT){
        printf("exit...\n");

        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(msg->source, client_ID[i])){
                client_connected[i] = 0;
                client_list[i]->in_session = false;
                client_fd[i] = -1;
                break;
            }
        }
        send_num = send(fd, "14:1:a:a", strlen("14:1:a:a"), 0);//#define OUT_ACK     14
        printf("client %s exited.\n", msg->source);

        
        close(fd);
        FD_CLR(fd, &master);
    }
    else if(msg->type == JOIN || msg->type == ACCEPT){
        if(msg->type == JOIN) printf("joinning session...\n");
        if(msg->type == ACCEPT) printf("Invitation accepted, joinning session...\n");

        bool session_found = false;
        struct session* front = head;

        int client_num = -1;
        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(msg->source, client_ID[i])){
                client_num = i;
                break;
            }
        }

        if(head==NULL){
            printf("No session exists.");
            send_num = send(fd, "7:1:a:a", strlen("7:1:a:a"), 0);//#define JN_NAK 7
        }
        else{
            while(front!=NULL){
                if(!strcmp(front->id, msg->data)){
                    session_found = true;
                    front->num_members++;
                    strcpy(client_list[client_num]->session_id, msg->data);
                    client_list[client_num]->in_session = true;
                    break;
                }
                front = front->next;
            }

            if(session_found){
                send_num = send(fd, "6:1:a:a", strlen("6:1:a:a"), 0);//#define JN_ACK 6
                printf("Session joined.\n");
            }
            else{
                printf("No session found.");
                send_num = send(fd, "7:1:a:a", strlen("7:1:a:a"), 0);//#define JN_NAK 7
            }

        }

    }
    else if(msg->type == LEAVE_SESS){
        printf("leaving session...\n");

        struct session* front = head;
        struct session* follow = front;

        if(head==NULL){
            printf("No session exists.\n");
            return;
        }

        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(msg->source, client_ID[i])){
                client_list[i]->in_session = false;
            }
        }

        char sessionID[100] = {0};
        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(client_ID[i], msg->source)){
                strcpy(sessionID, client_list[i]->session_id);
                memset(&client_list[i]->session_id, 0 , sizeof(client_list[i]->session_id));////new added
                break;
            }
        }
        while(strcmp(front->id, sessionID)){
            follow = front;
            front = front->next;
        }

        front->num_members--;
        if(front->num_members==0){
            follow->next = front->next;
            free(front);
            if(front==head) head=NULL;
            front = NULL;

        }

        send_num = send(fd, "15:1:a:a", strlen("15:1:a:a"), 0);//#define LEAVE_ACK   15
        printf("client left.\n");
    }
    else if(msg->type == NEW_SESS){
        printf("create session...\n");
        
        /** Comment this below if want this: 
         ** when two client create session with the same name, they both join the same session */                                
        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(msg->data, client_list[i]->session_id)){
                //#define JN_NAK 7
                send_num = send(fd, "7:1:a:a", strlen("7:1:a:a"), 0);
                return;
            }
        }
        
        struct session* new = malloc(sizeof(struct session));
        memset(new->id, 0 , sizeof(new->id));
        strcpy(new->id, msg->data);
        new->num_members = 1;
        new->next = NULL;

        struct session* front = head;
        struct session* follow = front;
        while(front!=NULL){
            follow = front;
            front = front->next;
        }
        if(head!=NULL){
            follow->next = new;
        }
        else{
            head = new;
        }

        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(msg->source, client_ID[i])){
                 strcpy(client_list[i]->session_id, msg->data);
                 client_list[i]->in_session = true;
                 break;
            }
        }

        send_num = send(fd, "10:1:a:a", strlen("10:1:a:a"), 0);//#define NS_ACK  10
        printf("JN_ACK sent.\n");
    }
    else if(msg->type == MESSAGE){
        printf("sending message...\n");

        int client_number = -1;
        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(msg->source, client_ID[i])){
                client_number = i;
                break;
            }
        }

        if(client_list[client_number]->in_session == false){
            printf("Client is not in any session.\n");
            return;
        }

        char session_name[100] = {0};
        strcpy(session_name, client_list[client_number]->session_id);

        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(client_list[i]->session_id, session_name)){
            
                char reply[BUFFER_SIZE];
                sprintf(reply, "%d:%d:%s:%s", MSG_ACK, strlen(msg->data), msg->source, msg->data);
                int send_num = send(client_fd[i], reply, strlen(reply), 0);//#define MSG_ACK 16

            }
        }


    }
    else if(msg->type == QUERY){
        printf("client session list...\n");
    
        char list[BUFFER_SIZE] = {0};
        struct session* front = head;

        strcpy(list+strlen(list), "Sessions:\n");

        if(head==NULL){
            printf("No session exists.\n");
            strcpy(list+strlen(list), "none\n");
        }
        else{
            while(front!=NULL){
                char session_name[100] = {0};
                strcpy(session_name,front->id);
                session_name[strlen(session_name)-1] = '\0';

                strcpy(list+strlen(list), session_name);
                strcpy(list+strlen(list), ":");
                for(int i=0;i<MAX_CLIENT;i++){
                    if(!strcmp(client_list[i]->session_id, front->id)){
                        strcpy(list+strlen(list), " ");
                        strcpy(list+strlen(list), client_ID[i]);
                    }
                }
                strcpy(list+strlen(list), "\n");
                front = front->next;
            }
        }

        strcpy(list+strlen(list), "Online:");
        for(int i=0;i<MAX_CLIENT;i++){
            if(client_connected[i]==1){
                strcpy(list+strlen(list), " ");
                strcpy(list+strlen(list), client_ID[i]);
            }
        }
        strcpy(list+strlen(list), "\n");

        printf("%s\n", list);

        char reply[BUFFER_SIZE] = {0};
        sprintf(reply, "%d:%d:%s:%s", QU_ACK, strlen(list), msg->source, list);
        int send_num = send(fd, reply, strlen(reply), 0);//#define QU_ACK13

        printf("client session list sent\n");
    }
    else if(msg->type == INVITE){
        printf("Inviting...\n");

        int client_number_inviting = -1;
        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(msg->source, client_ID[i])){
                client_number_inviting = i;
                break;
            }
        }
        //Error checking
        if(client_list[client_number_inviting]->in_session == false){
            printf("Client is not in any session.\n");
            char NAK[BUFFER_SIZE] = {0};
            sprintf(NAK, "%d:1:1:1", INVITE_NAK);
            int send_num1 = send(client_fd[client_number_inviting], NAK, strlen(NAK), 0);
            return;
        }
        
        
        int client_number_invited = -1;
        msg->data[strlen(msg->data)-1] = '\0';
        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(msg->data, client_ID[i])){
                client_number_invited = i;
printf("client_number_invited = %d\n", i);
                break;
            }
        }

        char session_name[100] = {0};
        strcpy(session_name, client_list[client_number_inviting]->session_id);
        
        //send ACK to inviter
        char reply_inviting[BUFFER_SIZE] = {0};
        sprintf(reply_inviting, "%d:1:1:1", INVITE_ACK);
        int send_num2 = send(client_fd[client_number_inviting], reply_inviting, strlen(reply_inviting), 0);
        printf("content to sender: %s\n", reply_inviting);
        printf("%d bytes ACKed.\n", send_num2);

        //send invatation to invitee
        char reply_invited[BUFFER_SIZE] = {0};
        sprintf(reply_invited, "%d:%d:%s:%s", INVITE, strlen(session_name), msg->source, session_name);
        int send_num3 = send(client_fd[client_number_invited], reply_invited, strlen(reply_invited), 0);//#define INVITE_ACK 18
        printf("content to receiver: %s", reply_invited);
        printf("%s invited.\n", msg->data);

    }
    else if(msg->type == DECLINE){
        printf("client declined invitation.\n");
        return;
    }


    //accepting format:
    // type    : size : source    : date
    // PRIVATE : size : IDsending : IDreceving  
    else if(msg->type == PRIVATE ){
        printf("Private message...\n");

        int client_number_sending = -1;
        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(msg->source, client_ID[i])){
                client_number_sending = i;
                break;
            }
        }
        
        char private_receiver[MAX_NAME] = {0};
        char msg_data[MAX_NAME] = {0};
        strcpy(private_receiver , strtok(msg->data, ":") );
        strcpy(msg_data , strtok(NULL, "\0") );
        bool receiver_found = false;

        for(int i=0;i<MAX_CLIENT;i++){
            if(!strcmp(client_ID[i], msg->source)){

                char reply1[BUFFER_SIZE] = {0};
                sprintf(reply1, "%d:%d:%s:%s", PRIVATE_ACK, strlen(msg->data), msg->source, msg->data);
                int send_num = send(client_fd[i], reply1, strlen(reply1), 0);//#define MSG_ACK 16
                printf("ACK sent: %s", reply1);
            }
            if(!strcmp(client_ID[i], private_receiver)){
                receiver_found = true;
                char reply2[BUFFER_SIZE] = {0};
                sprintf(reply2, "%d:%d:%s:%s", PRIVATE, strlen(msg_data), msg->source, msg_data);
                int send_num = send(client_fd[i], reply2, strlen(reply2), 0);//#define MSG_ACK 16
                printf("message sent: %s", reply2);
            }
        }
        if(receiver_found == false){
            //#define PRIVATE_NAK 24
            int send_num = send(client_fd[client_number_sending], "24:1:1:1", strlen("24:1:1:1"), 0);//#define MSG_ACK 16
            printf("Receiver not found, NAK: %s", "24:1:1:1");
        }

    }
    else{}//Do nothing. Just wanted to keep the "if(msg->type == something)" format. 


}












int main(int argc, char const*argv[])
{
    //Sanity test.
    printf("hello world from server!\n\n");
    if(argc != 2){
        printf("Input Format Error\n");
        exit(0);
    }

    //Initializations.
    for(int i=0; i<MAX_CLIENT; i++){
        client_list[i] = malloc(sizeof(struct client));
        client_list[i]->in_session = false;
        //client_list[i]->list_number = i;
    }

    /*build TCP socket*/
    int port = atoi(argv[1]);
    int initial_socketfd;
    initial_socketfd = socket(AF_INET,SOCK_STREAM,0);
    if(initial_socketfd < 0){
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

    if(bind(initial_socketfd, (struct sockaddr *)&serv_addr, len) == -1){
        printf("bind error\n");
        exit(1);
    }


    /*listen to and accept connection*/
    int max_connection = 100;
    if(listen(initial_socketfd, max_connection) == -1){
        printf("listen error\n");
        exit(1);
    }


    //////////////receiving///////////////
    int recv_num;
    int send_num;
    char send_buf[BUF_SIZE] = {0};
    char recv_buf[BUF_SIZE] = {0};
    memset(recv_buf, 0 , BUF_SIZE);
    memset(send_buf, 0 , BUF_SIZE);

    struct message received_message;


    int fdmax = initial_socketfd;
    FD_ZERO(&readfds);
    FD_ZERO(&master);
    FD_SET(initial_socketfd, &master);


    int respondto_fd = -1;
    int iteration = 1;
    while(1){
        printf("//////iteration %d//////\n", iteration); 

        fdmax = initial_socketfd;
        for(int i = 0; i < MAX_CLIENT; i++){ 
            if(client_connected[i]==1){ 
                FD_SET(client_fd[i], &master);

                if(client_fd[i] > fdmax){
                    fdmax = client_fd[i];
                }
            }
        }

        readfds = master;
        if (select(fdmax+1, &readfds, NULL, NULL, NULL) == -1) {
            printf("select error\n");
            perror("select error");
            exit(4);
        }


        for(int i=0; i <= fdmax; i++) {
            if (FD_ISSET(i, &readfds)) {

                if(i==initial_socketfd){//client socket--listenning

                    struct sockaddr_storage client_addr;
                    socklen_t client_addr_size = sizeof(client_addr); //size of struct sockaddr_storage.

                    int accept_fd = accept(initial_socketfd, (struct sockaddr *)&client_addr, &client_addr_size);
                    if(accept_fd == -1){
                        printf("accept error");
                        exit(1);
                    }
                    FD_SET(accept_fd, &master);
                    respondto_fd = accept_fd;
                }
                else{
                    respondto_fd = i;

                }


                recv_num = recv(respondto_fd, recv_buf, BUF_SIZE, 0);
                //recv_num = recv(respondto_fd, recv_buf, 100, 0);
                if(recv_num == -1){
                    printf("recv error");
                    exit(1);
                }

                printf("%dth recv: recv_num %d\n", iteration, recv_num); 
                printf("buffer: %s", recv_buf);
                
                decodeMessage2(recv_buf, &received_message);
                printf("decode: %d:%d:%s:%s\n", received_message.type, received_message.size
                                              , received_message.source, received_message.data);
                // printf("%d:", received_message.size);
                // printf("%s:", received_message.source);
                // printf("%s", received_message.data);printf("\n");

                memset(recv_buf, 0 , BUF_SIZE);

                react(respondto_fd, &received_message);       

            }
        }


            

        iteration++;
        printf("\n");
    }


}