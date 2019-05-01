#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "utils.h"

// void mysend(int sock, const void *bytes, size_t size, Header header)
void send_msg(int sock, const char *buffer, const Header *header)
{
    //send header
    send_header(sock, header);
    //send data

    if (send(sock, buffer, header->length, 0) == -1)
    {
        die("Failed to send data");
    }
}

void recv_msg(int sock, char *buffer, Header *header)
{
    //send header
    recv_header(sock, header);
    //send data
    if (recv(sock, buffer, header->length, 0) == -1)
    {
        die("Failed to recv data");
    }else{
        buffer[header->length] = 0;
    }
}

void send_header(int sock, const Header *header)
{

    if (send(sock, header, sizeof(Header), 0) == -1)
    {
        die("Failed to send header");
    }
}

void recv_header(int sock, Header *header)
{
    int ret = recv(sock, header, sizeof(Header), 0);
    if (ret == -1)
    {
        die("Failed to receive header");
    }
    else if (ret == 0)
    {
        // if (header->begin[0] != 0x3d || header->begin[1] != 0x3f)
        // {
        //     fprintf(stderr, "%d, %d  ", header->begin[0], header->begin[1]);
        //     die("wrong header");
        // }
        fprintf(stdout, "The host has shutdown!\n");
        pthread_exit(NULL);
    }
}

void die(char *mess)
{
    perror(mess);
    pthread_exit(NULL);
}