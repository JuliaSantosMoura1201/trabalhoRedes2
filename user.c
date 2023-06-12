#include "common.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSZ 1024

#define COMMAND_CLOSE_CONNECTION "close connection"
#define COMMAND_LIST_USERS "list users"
#define COMMAND_SEND_TO "send to"
#define COMMAND_SEND_TO_ALL "send all"

void usage(int argc, char **argv){
    printf("usage: %s <server IP> <server port>", argv[0]);
    printf("example: %s 127.0.0.1 51511", argv[0]);
    exit(EXIT_FAILURE);
}

void identifyCommand(char *command, int s){
    // remove \n from command
    command[strcspn(command, "\n")] = 0;
    
    if (strncmp(command, COMMAND_CLOSE_CONNECTION, strlen(COMMAND_CLOSE_CONNECTION)) == 0) {
        printf("close connection\n");
    }else if(strncmp(command, COMMAND_LIST_USERS, strlen(COMMAND_LIST_USERS)) == 0) {
        printf("list users\n");
    }else if(strncmp(command, COMMAND_SEND_TO, strlen(COMMAND_SEND_TO)) == 0) {
        printf("send to\n");
    }else if(strncmp(command, COMMAND_SEND_TO_ALL, strlen(COMMAND_SEND_TO_ALL)) == 0) {
        printf("send all\n");
    } else {
        printf("command not identified\n");
    }
}

void *listenServer(void *socket){
    int s = *(int *)socket;
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    unsigned total = 0;
    while(1){
        size_t count = recv(s, buf + total, BUFSZ - total, 0);
        if(buf[strlen(buf) - 1] == ')'){
            // Connection terminated
            break;
        }
        total += count;
    }

    printf("received %u bytes\n", total);
    puts(buf);
}

int main(int argc, char **argv){
    if(argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if(addrparse(argv[1], argv[2], &storage) != 0){
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr*)(&storage);
    if(connect(s, addr, sizeof(storage)) != 0){
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    printf("connected to %s\n", addrstr);

    while(1){
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        printf("mensagem>");
        fgets(buf, BUFSZ -1, stdin);

        size_t count = send(s, buf, strlen(buf)+1, 0);
        if(count != strlen(buf)+1){
            logexit("send");
        }

        pthread_t tid;
        pthread_create(&tid, NULL, listenServer, &s);

        /*memset(buf, 0, BUFSZ);
        unsigned total = 0;
        while(1){
            count = recv(s, buf + total, BUFSZ - total, 0);
            if(buf[strlen(buf) - 1] == ')'){
                // Connection terminated
                break;
            }
            total += count;
        }

        printf("received %u bytes\n", total);
        puts(buf);
        memset(buf, 0, BUFSZ);*/
    }
    
    close(s);
    exit(EXIT_SUCCESS);
}