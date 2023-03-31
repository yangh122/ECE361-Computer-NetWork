#ifndef Message_h
#define Message_h

#include <stdio.h>
#include <stdlib.h>


#define MAX_NAME 100
#define MAX_DATA 1800
#define BUFFER_SIZE 2000
#define BUF_SIZE 2000
#define LOGIN       1
#define LO_ACK      2
#define LO_NAK      3
#define EXIT        4
#define JOIN        5
#define JN_ACK      6
#define JN_NAK      7
#define LEAVE_SESS  8
#define NEW_SESS    9
#define NS_ACK      10
#define MESSAGE     11
#define QUERY       12
#define QU_ACK      13
#define OUT_ACK     14
#define LEAVE_ACK   15
#define MSG_ACK     16
#define INVITE      17
#define INVITE_ACK  18
#define INVITE_NAK  19
#define ACCEPT      20
#define DECLINE     21
#define PRIVATE     22
#define PRIVATE_ACK 23
#define PRIVATE_NAK 24
struct message {
    
    int type;
    int size;
    char source[MAX_NAME];
    char data[MAX_DATA];

};


struct invite_session{
    char invite[100];
    struct invite_session* next;
    //int index;
};


#endif