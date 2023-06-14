#include "common.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#define BUFSZ 1024

#define USER_COMMAND_CLOSE_CONNECTION "close connection"
#define USER_COMMAND_LIST_USERS "list users"
#define USER_COMMAND_SEND_TO "send to"
#define USER_COMMAND_SEND_TO_ALL "send all"

// Variável global para indicar se as threads devem continuar em execução
bool running = true; 

void usage(int argc, char **argv){
    printf("usage: %s <server IP> <server port>", argv[0]);
    printf("example: %s 127.0.0.1 51511", argv[0]);
    exit(EXIT_FAILURE);
}

void closeConnection(){
     printf("close connection\n");
}

void listUsers(){
     printf("list users\n");
}

void sendTo(){
     printf("send to\n");
}

void sendToAll(){
     printf("send all\n");
}

void identifyCommand(char *command, int s){
    // remove \n from command
    command[strcspn(command, "\n")] = 0;
    
    if (strncmp(command, USER_COMMAND_CLOSE_CONNECTION, strlen(USER_COMMAND_CLOSE_CONNECTION)) == 0) {
        closeConnection();
    }else if(strncmp(command, USER_COMMAND_LIST_USERS, strlen(USER_COMMAND_LIST_USERS)) == 0) {
        listUsers();
    }else if(strncmp(command, USER_COMMAND_SEND_TO, strlen(USER_COMMAND_SEND_TO)) == 0) {
        sendTo();
    }else if(strncmp(command, USER_COMMAND_SEND_TO_ALL, strlen(USER_COMMAND_SEND_TO_ALL)) == 0) {
        sendToAll();
    } else {
        printf("command not identified\n");
    }
}

void listenServerUnicast(int s){
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
    
    pthread_exit(NULL);
}

void *readKeyboard(void *socket){
    int s = *(int *)socket;

    while(1){
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        printf("mensagem>");
        fgets(buf, BUFSZ -1, stdin);

        size_t count = send(s, buf, strlen(buf)+1, 0);
        if(count != strlen(buf)+1){
            logexit("send");
        }
    }
    /*
        identifica o comando do user aqui
        close threads muda running para false
    */
    
    pthread_exit(NULL);
}
/*
void openConnection(){
    char buf[BUFSZ] = COMMAND_OPEN_CONNECTION;
    size_t count = send(s, buf, strlen(buf)+1, 0);
    if(count != strlen(buf)+1){
        logexit("send");
    }
}
*/

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
    //openConnection(s);

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    printf("connected to %s\n", addrstr);
    
    /*
    pthread_t threadListenServer;
    pthread_create(&threadListenServer, NULL, listenServer, &s);

    pthread_t readListenKeyboard;
    pthread_create(&readListenKeyboard, NULL, readKeyboard, &s);

    pthread_join(threadListenServer, NULL);
    pthread_join(readListenKeyboard, NULL);
    */

    while(1){
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        printf("mensagem>");
        fgets(buf, BUFSZ -1, stdin);

        size_t count = send(s, buf, strlen(buf)+1, 0);
        if(count != strlen(buf)+1){
            logexit("send");
        }

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


    close(s);
    exit(EXIT_SUCCESS);
    
}