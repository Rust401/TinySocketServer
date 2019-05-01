#ifndef UTILS_H
#define UTILS_H

#include "protocol.h"
#include <stdlib.h>

void send_msg(int sock, const char *buffer, const Header* header);

void recv_msg(int sock, char *buffer, Header* header);

void send_header(int sock, const Header* header);

void recv_header(int sock, Header *header);

void die(char *mess);


#endif // !UTILS_H


