#ifndef packet_h
#define packet_h

#include <stdio.h>
#include <stdlib.h>

struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
    int index;
};

const int BUF_SIZE = 1000;
const double TIME_OUT = 150;

#endif
