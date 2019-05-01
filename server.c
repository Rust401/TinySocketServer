#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include "protocol.h"
#include "utils.h"

#define MAXPENDING 16
#define BUFFSIZE 8192
#define PORT 4069
char *name = "Rust401 server\n";

char *welcome = "Server Connected! Welcome!\n";

void get_time(char *time_string, int buffer_size)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strncpy(time_string, asctime(timeinfo), buffer_size);
    time_string[buffer_size] = '\0';
}

typedef struct client_info
{
    pthread_t thread_id;
    int clientsock;
    int index;
    const char *ip;
    int port;
} Client_info;

Client_info clients[MAXPENDING];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void get_list(char *string, int buffer_size)
{
    int cur = 0;
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAXPENDING; i++)
    {
        if (clients[i].index != -1)
        {
            sprintf(string + cur, "%1d ", clients[i].index);
            cur += 2;
            strncpy(string + cur, clients[i].ip, strlen(clients[i].ip));
            cur += strlen(clients[i].ip);
            strncpy(string + cur, ":", strlen(":"));
            cur += strlen(":");
            sprintf(string + cur, "%5d\n", clients[i].port);
            cur += 6;
        }
    }
    string[cur] = 0;
    pthread_mutex_unlock(&clients_mutex);
}

void getConnectedClient(char* buffer,int bufferSize)
{
    char* cur=buffer;
    int n;
    
    pthread_mutex_lock(&clients_mutex);
    for(int i=0;i<MAXPENDING;++i)
    {
        if(clients[i].index!=-1)
        {   
            if((cur-buffer)>=bufferSize)break;
            n=sprintf(cur,"sockfd:%d ip:%s port:%d index:%d\n",clients[i].clientsock,clients[i].ip,clients[i].port,clients[i].index);
            cur+=n;  
        }
    }
    *cur='\0';
    pthread_mutex_unlock(&clients_mutex);

    return;
}

void *handleClient(void *client)
{
    char buffer[BUFFSIZE];

    int sock = ((Client_info *)client)->clientsock;
    const char *ip = ((Client_info *)client)->ip;
    int index = ((Client_info *)client)->index;
    // int port = ((Client_info *)client)->port;

    char *myname = "server";

    Header h = {
        .begin = {0x3f, 0x3f},
        .type = WELCOME,
        .length = strlen(welcome)};
    strcpy(h.name, myname);
    send_msg(sock, welcome, &h);

    while (1)
    {
        recv_msg(sock, buffer, &h);

        if (h.type == TIME_REQUEST)
        {
            char* time_string=(char*)malloc(100*sizeof(char));
            get_time(time_string, 100);

            h.type = TIME_RESPONSE;
            h.length = strlen(time_string);
            strcpy(h.name, myname);
            send_msg(sock, time_string, &h);
            free(time_string);
        }
        else if (h.type == NAME_REQUEST)
        {
            h.type = NAME_RESPONSE;
            h.length = strlen(name);
            strcpy(h.name, myname);
            send_msg(sock, name, &h);
        }
        else if (h.type == LIST_REQUEST)
        {
            h.type = LIST_RESPONSE;
            //get_list(buffer, BUFFSIZE);
            getConnectedClient(buffer,BUFFSIZE);
            h.length = strlen(buffer);
            strcpy(h.name, myname);
            send_msg(sock, buffer, &h);
        }
        else if (h.type == MESSAGE_SEND)
        {
            int index = -1;
            char* message=(char*)malloc(BUFFSIZE*sizeof(char));
            sscanf(buffer, "chat %d %[^\n]", &index, message);
            message[strlen(message)] = '\n';
            if (index < MAXPENDING && clients[index].index != -1)
            {
                Header h_receiver = {
                    .begin = {0x3f, 0x3f},
                    .type = MESSAGE,
                    .length = strlen(message)};
                sprintf(h_receiver.name, "%s:%d", clients[index].ip, clients[index].port);

                send_msg(clients[index].clientsock, message, &h_receiver);

                Header h_sender = {
                    .begin = {0x3f, 0x3f},
                    .type = MESSAGE_SEND_COMFIRM,
                    .length = strlen("message has been sent.\n")};
                strcpy(h_sender.name, myname);
                send_msg(sock, "message has been sent.\n", &h_sender);
            }
            else
            {
                Header h_sender = {
                    .begin = {0x3f, 0x3f},
                    .type = MESSAGE_SEND_COMFIRM,
                    .length = strlen("no such client.\n")};
                strcpy(h_sender.name, myname);
                send_msg(sock, "no such client.\n", &h_sender);
            }
            free(message);
        }
        else
        {
            fprintf(stdout, "%s: %s", h.name, buffer);
        }
    }
    fprintf(stdout, "connection from %s has been shut down!\n", ip);
    close(sock);
    pthread_mutex_lock(&clients_mutex);
    clients[index].index = -1;
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
}

void init()
{
    for (int i = 0; i < MAXPENDING; i++)
    {
        clients[i].index = -1;
    }
    fprintf(stdout, "initialize success\n");
}

int main(int argc, char *argv[])
{
    init();
    int serversock, clientsock;
    struct sockaddr_in echoserver, echoclient;

    /* Create the TCP socket */
    if ((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        die("Failed to create socket");
    }
    /* Construct the server sockaddr_in structure */
    memset(&echoserver, 0, sizeof(echoserver));     /* Clear struct */
    echoserver.sin_family = AF_INET;                /* Internet/IP */
    echoserver.sin_addr.s_addr = htonl(INADDR_ANY); /* Incoming addr */
    echoserver.sin_port = htons(PORT);              /* server port */
    /* Bind the server socket */
    if (bind(serversock, (struct sockaddr *)&echoserver,
             sizeof(echoserver)) < 0)
    {
        die("Failed to bind the server socket");
    }
    /* Listen on the server socket */

    if (listen(serversock, MAXPENDING) < 0)
    {
        die("Failed to listen on server socket");
    }

    /* Run until cancelled */

    while (1)
    {
        unsigned int clientlen = sizeof(echoclient);
        if ((clientsock = accept(serversock, (struct sockaddr *)&echoclient,&clientlen)) < 0)
        {
            die("Failed to accept client connection");
        }

        const char *ip = inet_ntoa(echoclient.sin_addr);
        int port = echoclient.sin_port;

        pthread_t thid;
        void *ret;

        int index = -1;

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAXPENDING; i++)
        {
            if (clients[i].index == -1)
            {
                clients[i].index = i;
                index = i;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (index == -1)
        {
            //TODO: decide protocal to refuse connection
            fprintf(stdout, "MAX PENDING");

            continue;
        }

        clients[index].clientsock = clientsock;
        clients[index].ip = ip;
        clients[index].port = port;

        fprintf(stdout, "Client connected: %s %d\n", ip, port);

        if (pthread_create(&(clients[index].thread_id), NULL, handleClient, &clients[index]) != 0)
        {
            die("pthread_create() error");
        }
        pthread_detach(clients[index].thread_id);
    }
}