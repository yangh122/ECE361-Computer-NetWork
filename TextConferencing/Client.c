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
#include <stdbool.h>
#include <pthread.h>


#include "Message_h.h"


/*Initialize the variable for the while loop check*/
bool is_login = false;
bool is_join = false;
char* user_input = NULL;
char* command = NULL;
char* new_data = NULL;
char* client_id = NULL;
char* password = NULL;
char* server_IP = NULL;
char* server_port = NULL;

char client_id_st[100] = {0};
size_t size = 0;


/*Build Buffer and Checking Cariable*/

char recv_buf[BUFFER_SIZE] = {0};
char message_buf[BUFFER_SIZE] = {0};
int connect_check;
int thread_check;
int recv_num;
int send_num;



/*Build struct for the message*/

struct message new_message_send;
struct message new_message_rec;




/*Record length*/

int client_id_length = 0;
int password_length = 0;
int server_IP_length = 0;
int server_port_length = 0;

/*Socket Struct*/
int socketfd;
struct sockaddr_in serv_addr;


/*Variables for Thread*/
pthread_t message_listener_thread;
bool is_created = false;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*Variables for Struct*/
struct invite_session* head = NULL; 


void makeMessage(struct message* one_packet, char* message, int size);
void decodeMessage(char* from, struct message* to);
void *message_listener(void *arg);
bool get_session_id(char* session_id);
void add_to_tail(struct invite_session* new_session);
void print_all_invite();
void clear_all_invite();


int main(){
    while(1){


        //reset every buffer
        if(user_input != NULL){
            memset(user_input, 0, sizeof(user_input));
        }
        if(command != NULL){
            memset(command, 0, sizeof(command));
        }
        if(new_data != NULL){
            memset(new_data, 0, sizeof(new_data));
        }

        memset(message_buf, 0, sizeof(message_buf));

        //Get user input
        getline(&user_input, &size, stdin);
        char text[size];
        strncpy(text, user_input, size);
        // store the user input into a new char* to send it as message

        //get the command;
        char *ptr = strchr(user_input, ' '); // Find first occurrence of space
        if (ptr != NULL) { // If space found
            new_data = strdup(ptr+1); // Duplicate substring after space
        }
        command = strtok(user_input, " ");
        
        if(strcmp(command, "/login") == 0 && is_login == false){
            client_id = strtok(NULL, " ");
            password = strtok(NULL, " ");
            server_IP = strtok(NULL, " ");
            server_port = strtok(NULL, "\n");

            strcpy(client_id_st, client_id);

            if( client_id == NULL || password == NULL || server_IP == NULL || server_port == NULL){
                printf("Invalid input format\n");
                printf("Please input as /login <client ID> <password> <server-IP> <server-port> \n");
            }

            else{
                int port = atoi(server_port);
                pthread_mutex_lock(&mutex);          
                socketfd = socket(AF_INET,SOCK_STREAM,0);
                pthread_mutex_unlock(&mutex);
                if(socketfd < 0){
                    printf("Socket Create Error\n");
                    continue;
                }
               

                
                memset(&serv_addr, 0 , sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(port);

                if(inet_aton(server_IP, &serv_addr.sin_addr) == 0){
                    printf("IP Error\n");
                    continue;
                }
                connect_check = connect(socketfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                if(connect_check == -1){
                    printf("Establish Connection Error\n");
                    if(user_input != NULL){
                        memset(user_input, 0, sizeof(user_input));
                    }
                    if(command != NULL){
                        memset(command, 0, sizeof(command));
                    }
                    if(new_data != NULL){
                        memset(new_data, 0, sizeof(new_data));
                    }
                    continue;
                    
                }
                
                //if(is_created == false){
                    thread_check = pthread_create(&message_listener_thread, NULL, message_listener, NULL);

                    if (thread_check != 0) {
                        printf("Failed to create thread\n");
                        exit(EXIT_FAILURE);
                    }
                    //is_created = true;
                //}

                
                
                client_id_length = strlen(client_id);
                password_length = strlen(password);
                server_IP_length = strlen(server_IP);
                server_port_length = strlen(server_port);

                new_message_send.type = LOGIN;
                new_message_send.size = strlen(new_data);
                strcpy(new_message_send.data, new_data);
                strcpy(new_message_send.source, client_id);


                makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

                send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);
                if(send_num == -1){
                    printf("Send Buffer Error");
                    continue;
                }                
            }
        }
        else if((strcmp(command, "/logout\n") == 0) && (is_login == true) && (is_join == false)){
            new_message_send.type = EXIT;
            new_message_send.size = 0;
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, "N/A");


            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }
        }
        else if((strcmp(command, "/joinsession") == 0) && (is_login == true) && (is_join == false)){
            new_message_send.type = JOIN;
            new_message_send.size = strlen(new_data);
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, new_data);


            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }          
        }
        else if((strcmp(command, "/createsession") == 0) && (is_login == true) && (is_join == false)){
            new_message_send.type = NEW_SESS;
            new_message_send.size = strlen(new_data);
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, new_data);


            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }

                      
        }
        else if((strcmp(command, "/leavesession\n") == 0) && (is_login == true)&& (is_join == true)){
            new_message_send.type = LEAVE_SESS;
            new_message_send.size = 0;
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, "N/A");


            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }            
        }
        else if((strcmp(command, "/list\n") == 0) && (is_login == true) && (is_join == false)){
            new_message_send.type = QUERY;
            new_message_send.size = 0;
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, "N/A");


            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }
            
        }
    
        else if((strcmp(command, "/invite") == 0) && (is_login == true) && (is_join == true)){
            new_message_send.type = INVITE;
            new_message_send.size = strlen(new_data);
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, new_data);

            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }
        }
        else if((strcmp(command, "/accept") == 0) && (is_login == true) && (is_join == false)){
            pthread_mutex_lock(&mutex); 
            if(get_session_id(new_data) == false){
                printf("Could Not find the session id: %s\n", new_data);
                pthread_mutex_unlock(&mutex);
                continue;
            }
            pthread_mutex_unlock(&mutex);
            new_message_send.type = ACCEPT;
            new_message_send.size = strlen(new_data);
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, new_data);

            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }
        }
        else if((strcmp(command, "/accept") == 0) && (is_login == true) && (is_join == true)){
            new_message_send.type = LEAVE_SESS;
            new_message_send.size = 0;
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, "N/A");


            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }


            memset(message_buf, 0, sizeof(message_buf));
            sleep(1);
            pthread_mutex_lock(&mutex); 
            if(get_session_id(new_data) == false){
                printf("Could Not find the session id: %s\n", new_data);
                pthread_mutex_unlock(&mutex);
                continue;
            }
            pthread_mutex_unlock(&mutex);
            new_message_send.type = ACCEPT;
            new_message_send.size = strlen(new_data);
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, new_data);

            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);
            // printf("total send %d", send_num);
            // printf("send messgae %s", )

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }
            
        }

        else if((strcmp(command, "/decline") == 0) && (is_login == true)){
            pthread_mutex_lock(&mutex); 
            if(get_session_id(new_data) == false){
                printf("Could Not find the session id: %s\n\n", new_data);
                continue;
            }
            pthread_mutex_unlock(&mutex);
            new_message_send.type = DECLINE;
            new_message_send.size = strlen(new_data);
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, new_data);

            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }
        }

        else if((strcmp(command, "/list_invite\n") == 0) && (is_login == true)){
            printf("Invitation you receive:\n");
            print_all_invite();
            printf("\n");
        }
        else if((strcmp(command, "/clear_invite\n") == 0) && (is_login == true)){
            printf("Clean all invitation:\n\n");
            clear_all_invite();
        }
        else if((strcmp(command, "/private") == 0) && (is_login == true)){
            new_message_send.type = PRIVATE;
            new_message_send.size = strlen(new_data);
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, new_data);
            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);
            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);
            
            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }

        }
        else if((strcmp(command, "/quit\n") == 0) && is_login == false && is_join == false){
            close(socketfd);
            return 0;
        }
        else if(is_login == true && is_join == false){
            printf("Please join a session first\n\n");
        }
        else if(is_join == true && is_login == true){
            new_message_send.type = MESSAGE;
            new_message_send.size = strlen(text);
            strcpy(new_message_send.source, client_id_st);
            strcpy(new_message_send.data, text);
            makeMessage(&new_message_send, message_buf,BUFFER_SIZE);

            send_num = send(socketfd, message_buf, BUFFER_SIZE, 0);

            if(send_num == -1){
                printf("Send Buffer Error");
                continue;
            }
        }
        else{
            printf("Please login first\n");
        }
    


    }


    
/*Build TCP Socket*/

    if(send_num == -1){
        printf("Send Buffer Error");
        exit(1);
    }


    if(strcmp(recv_buf , "ACK") == 0){
        printf("Receive ACK\n");
    }

    return 1;

}







void makeMessage(struct message* one_packet, char* message, int size){
    char tem_char[100] = {0};
    sprintf(tem_char, "%d:", one_packet->type);
    strcpy(message, tem_char);

    sprintf(tem_char, "%d:", one_packet->size);
    strcat(message, tem_char);
    strcat(message, one_packet->source);
    strcat(message, ":");
    strcat(message, one_packet->data);

    printf("%s\n", message);

}



void decodeMessage(char* from, struct message* to){
    to->type = atoi(strtok(from, ":"));
    to->size = atoi(strtok(NULL, ":"));
    strcpy(to->source , strtok(NULL, ":"));
    strcpy(to->data , strtok(NULL, "\0"));
}



void *message_listener(void *arg)
{

    while(1){
        recv_num = recv(socketfd, recv_buf, BUF_SIZE, 0);
        //printf("Get into the thread successfully\n");
        if (recv_num < 0) {
            printf("Receive Error");
            break;        
        } else if (recv_num == 0) {
            printf("Server closed connection\n");
            is_join = false;
            is_login = false;
            pthread_cancel(message_listener_thread);
            pthread_join(message_listener_thread, NULL);
            break;
        } else {
            printf("Received message: %s\n", recv_buf);
            decodeMessage(recv_buf, &new_message_rec);
            printf("Decode Message Successfully\n");
            if(new_message_rec.type == 2){
                printf("Login successfully\n\n");
                is_login = true;
            }
            else if(new_message_rec.type == 3){
                printf("Login failed\n\n");
            }
            else if(new_message_rec.type == OUT_ACK){
                printf("logout successfully\n\n");
                is_login = false;
            }
            else if(new_message_rec.type == JN_ACK){
                int index = strlen(new_data);
                new_data[index - 1] = '\0';
                printf("Join seesion %s successfully\n\n", new_data);
                is_join = true;
            }
            else if(new_message_rec.type == JN_NAK){
                printf("Join seesion %s failed\n\n", new_data);
            }

            else if(new_message_rec.type == NS_ACK){
                printf("Create seesion and successfully and join it\n\n");
                is_join = true;
            }
            else if(new_message_rec.type == LEAVE_ACK){
                printf("leave successfully\n\n");
                is_join = false;
            }
            else if(new_message_rec.type == QU_ACK){
                printf("Receive the List\n");
                printf("%s\n", new_message_rec.data);
            }
            else if(new_message_rec.type == MSG_ACK){
                printf("%s: ", new_message_rec.source);
                printf("%s\n", new_message_rec.data);
            }
            else if(new_message_rec.type == INVITE){
                printf("Get invitation From %s to session %s\n", new_message_rec.source, new_message_rec.data);

                pthread_mutex_lock(&mutex);
                struct invite_session* new_seesion = malloc(sizeof(struct invite_session));
                strcpy(new_seesion->invite, new_message_rec.data);
                new_seesion->next = NULL;
                add_to_tail(new_seesion);
                pthread_mutex_unlock(&mutex);

            }
            else if(new_message_rec.type == INVITE_NAK){
                printf("Invite failed\n\n");
            }
            else if(new_message_rec.type == INVITE_ACK){
                printf("Invite successfully\n\n");
            }
            else if(new_message_rec.type == PRIVATE_NAK){
                printf("Send Private Message Failed. Clinet DNE\n\n");
            }
            else if(new_message_rec.type == PRIVATE_ACK){
                printf("Send Private Message Successfully\n\n");
            }
            else if(new_message_rec.type == PRIVATE){
                printf("Private_message:\n%s:%s\n",new_message_rec.source, new_message_rec.data);
            }
        }
        memset(recv_buf, 0, sizeof(recv_buf));
    }
    return NULL;
}


bool get_session_id(char* session_id){
    printf("%s\n", head->invite);

    if(strcmp(head->invite, session_id) == 0){
        head = NULL;
        return true;
    }

    struct invite_session* tem = head;
    struct invite_session* rec = head;

    while(tem != NULL){
        if(strcmp(tem->invite,session_id) == 0){
            rec->next = tem->next;
            free(tem);
            tem = NULL;
            return true;
        }
        else{
            rec = tem;
            tem = tem->next;
        }
    }
    return false;
}


void add_to_tail(struct invite_session* new_session){
    if(head == NULL){
        free(head);
        head = new_session;
        return;
    }
    
    struct invite_session* tem = head;
    while(tem->next != NULL){
        tem = tem->next;
    }
    tem->next = new_session;
    return;
}

void print_all_invite(){
    struct invite_session* cur = head;

    while(cur != NULL){
        printf("Invitation to session: %s\n", cur->invite);
        cur = cur->next;
    }

    return;

}


void clear_all_invite(){
    struct invite_session* cur = head;
    struct invite_session* rec = head;

    while(cur != NULL){
      rec = cur->next;
      free(cur);
      cur = rec;
    }

    head = NULL;
}



