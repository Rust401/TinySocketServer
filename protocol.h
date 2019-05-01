#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>

#define WELCOME 0
#define TEST 0
#define TIME_REQUEST 1
#define TIME_RESPONSE 11
#define NAME_REQUEST 2
#define NAME_RESPONSE 12
#define LIST_REQUEST 3
#define LIST_RESPONSE 13
#define MESSAGE_SEND 4
#define MESSAGE_SEND_COMFIRM 14
#define MESSAGE 24

#define REFUSE_CONNECTION 41

typedef struct header{
    unsigned char begin[2];
    unsigned char type;
    char name[22];
    size_t length;
}Header;
#endif // !PROTOCOL_H
