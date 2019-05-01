#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

#include "utils.h"
#include "mq.h"

#define BUFFSIZE 8192
struct Queue *q;

void *handle_server_data(void *sock)
{
    int mysock = *(int *)sock;
    char buffer[BUFFSIZE];
    char new_buffer[BUFFSIZE];
    Header h;
    while (1)
    {
        recv_msg(mysock, buffer, &h);
        buffer[h.length] = '\0';
        switch (h.type)
        {
        case TIME_RESPONSE:
            snprintf(new_buffer, BUFFSIZE, "time: %s", buffer);
            mq_send(q, new_buffer);
            break;
        case NAME_RESPONSE:
            snprintf(new_buffer, BUFFSIZE, "name: %s", buffer);
            mq_send(q, new_buffer);
            break;
        case LIST_RESPONSE:
            mq_send(q, buffer);
            break;
        case MESSAGE:
            snprintf(new_buffer, BUFFSIZE, "%s : %s", h.name, buffer);
            mq_send(q, new_buffer);
            break;
        case MESSAGE_SEND_COMFIRM:
            mq_send(q, buffer);
            break;
        }
    }
}

void connect_to_server(struct sockaddr_in *echoserver, int *sock, char *ip, int port, char *buffer)
{
    if ((*sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        die("Failed to create socket");
    }

    memset(echoserver, 0, sizeof(*echoserver));
    echoserver->sin_family = AF_INET;
    echoserver->sin_port = htons(port);
    echoserver->sin_addr.s_addr = inet_addr(ip);

    if (connect(*sock, (struct sockaddr *)echoserver, sizeof(*echoserver)) < 0)
    {
        die("Failed to connect with server");
    }

    Header h;
    recv_msg(*sock, buffer, &h);
    if (h.type == WELCOME)
    {
        fprintf(stdout, "WELCOME: %s\n", buffer);
        fprintf(stdout, "type menu for possible command\n");
    }
    else
    {
        fprintf(stderr, "wrong welcome type, %d", h.type);
    }

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_server_data, sock) != 0)
    {
        die("pthread_create() error");
    }
    //pthread_detach(thread_id);
}

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in echoserver;
    char buffer[BUFFSIZE];
    // unsigned int echolen;
    // int received = 0;
    //TODO: print menu
    char ip[30];
    int port;

    fprintf(stdout, "type menu for possible command\n");

    q = mq_new(100);

    Header h;
    int has_connected = 0;
    while (1)
    {
        usleep(100000);
        char *resp;
        for (int i = 0; i < 101; i++)
        {
            if (mq_recv_async(q, (void **)&resp) == 0)
            {
                fprintf(stdout, "%s", resp);
                if (strcmp("The host has shutdown!", resp) == 0)
                {
                    has_connected = 0;
                }
            }else{
                fprintf(stdout, "%d responsees!\n", i);
                break;
            }
        }

        fprintf(stdout, "> ");
        if (fgets(buffer, BUFFSIZE, stdin) == NULL)
        {
            break;
        }

        if (strcmp("menu\n", buffer) == 0)
        {
            if (has_connected)
            {
                fprintf(stdout, "1. disconnect: disconnect the server\n");
                fprintf(stdout, "2. quit: quit client\n");
                fprintf(stdout, "3. time: get time\n");
                fprintf(stdout, "4. name: get server name\n");
                fprintf(stdout, "5. list: the clients list\n");
                fprintf(stdout, "6. chat <client_index> <message>: send message to client with index and message\n");
            }
            else
            {
                fprintf(stdout, "1. connect <server_ip> <port>: connect to a server\n");
                fprintf(stdout, "2. quit: quit client\n");
            }
            continue;
        }
        else if (strncmp("connect ", buffer, 8) == 0)
        {
            sscanf(buffer, "connect %s %d", ip, &port);
            connect_to_server(&echoserver, &sock, ip, port, buffer);
            has_connected = 1;
            continue;
        }
        else if (strcmp("quit\n", buffer) == 0)
        {
            if (has_connected)
            {
                close(sock);
            }
            has_connected = 0;
            break;
        }
        else if (strcmp("disconnect\n", buffer) == 0)
        {
            if (has_connected)
            {
                close(sock);
            }
            else
            {
                fprintf(stderr, "not connected!\n");
            }
            has_connected = 0;
            continue;
        }
        else if (strcmp("time\n", buffer) == 0)
        {
            h.type = TIME_REQUEST;
            for (int i = 0; i < 100; i++)
            {
                buffer[strcspn(buffer, "\r\n") + 1] = 0;
                h.length = strlen(buffer);
                send_msg(sock, buffer, &h);
            }
            continue;
        }
        else if (strcmp("name\n", buffer) == 0)
        {
            h.type = NAME_REQUEST;
        }
        else if (strcmp("list\n", buffer) == 0)
        {
            h.type = LIST_REQUEST;
        }
        else if (strncmp("chat ", buffer, 5) == 0)
        {
            h.type = MESSAGE_SEND;
        }
        else if (strncmp("\n", buffer, 1) == 0)
        {
            continue;
        }
        else
        {
            fprintf(stdout, "unknown command\n");
            continue;
        }
        buffer[strcspn(buffer, "\r\n") + 1] = 0;
        h.length = strlen(buffer);
        send_msg(sock, buffer, &h);
    }

    close(sock);
    exit(0);
}
