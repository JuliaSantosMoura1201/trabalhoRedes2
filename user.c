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

int amountOfUsers = 0;
char** users;
int myId;
char destinationId[BUFSZ];
char privateMessage[BUFSZ];

// Variável global para indicar se as threads devem continuar em execução
bool running = true; 

void usage(int argc, char **argv){
    printf("usage: %s <server IP> <server port>", argv[0]);
    printf("example: %s 127.0.0.1 51511", argv[0]);
    exit(EXIT_FAILURE);
}

void closeConnection(int s){
    char message[BUFSZ];
    memset(message, 0, BUFSZ);

    sprintf(message, "%s(%d)", COMMAND_CLOSE_CONNECTION, myId);
    sendMessage(s, message);
}

void aksUsersList(){
     printf("list users\n");
}

void sendTo(char *command, int sock){
    memset(destinationId, 0, BUFSZ);
    int startIndex = strlen(USER_COMMAND_SEND_TO);
    int endIndex = strlen(USER_COMMAND_SEND_TO) + 3;
    memcpy(destinationId, command + startIndex + 1, endIndex - startIndex);
    destinationId[endIndex - startIndex] = '\0';

    char idFormatted[10];
    formatId(myId, idFormatted);

    memcpy(privateMessage, command + endIndex + 1, strlen(command));
    privateMessage[strlen(command) - endIndex] = '\0';

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "MSG(%s, %s)", destinationId, idFormatted);
    sendMessage(sock, buf);
}

void sendToAll(char *command, int sock){
    int startIndex = strlen(USER_COMMAND_SEND_TO_ALL);
    char message[BUFSZ];
    memcpy(message, command + startIndex + 1, strlen(command));
    message[strlen(command) - startIndex] = '\0';

    char idFormatted[10];
    formatId(myId, idFormatted);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "MSG(%s, NULL, %s)", idFormatted, message);
    sendMessage(sock, buf);
}

void readMessage(char *command){
    char msg[BUFSZ];
    memset(msg, 0, BUFSZ);
    getsMessageContent(command, msg, COMMAND_MESSAGE);

    int amountOfItems = 3;
    char **items = splitString(msg, ",", &amountOfItems, amountOfItems);

    // Adiciona usuário
    if(amountOfUsers == 0){
        users = malloc(30 * sizeof(char*));
        myId = atoi(items[0]);
    }

    users[amountOfUsers] = items[0];
    amountOfUsers++;

    // Imprime mensagem
    memset(msg, 0, BUFSZ);
    memcpy(msg, items[2] + 1, strlen(items[2]) - 1);
    puts(msg);
    
    for (int i = 0; i < amountOfItems; i++) {
        free(items[i]);
    }

    free(items);
}

void receiveUsersOnServer(char *command){
    char usersList[BUFSZ];
    memset(usersList, 0, BUFSZ);
    getsMessageContent(command, usersList, COMMAND_LIST_USERS);
    users = splitString(usersList, ",", &amountOfUsers, 50);
}

void identifyError(char *errorCode){
    if (strncmp(errorCode, ERROR_CODE_USER_LIMIT_EXCEED, strlen(ERROR_CODE_USER_LIMIT_EXCEED)) == 0) {
        puts( "User limit exceeded");
    }else if(strncmp(errorCode, ERROR_CODE_USER_NOT_FOUND, strlen(ERROR_CODE_USER_NOT_FOUND)) == 0) {
        puts( "User not found");
    }else if(strncmp(errorCode, ERROR_CODE_RECEIVER_NOT_FOUND, strlen(ERROR_CODE_RECEIVER_NOT_FOUND)) == 0) {
        puts( "Receiver not found");
    }else{
        logexit("Error not identified");
    }
}

void handleError(char *command){
    char errorCode[BUFSZ];
    memset(errorCode, 0, BUFSZ);
    getsMessageContent(command, errorCode, COMMAND_ERROR);
    identifyError(errorCode);
}

void handleConnectionClosed(int s){
    printf("Removed Successfully\n");
    close(s);
    exit(EXIT_SUCCESS);
}

void handleAnotherUserLeftingTheGroup(char *command){
    char userId[BUFSZ];
    memset(userId, 0, BUFSZ);
    getsMessageContent(command, userId, COMMAND_CLOSE_CONNECTION);

    for(int i; i<amountOfUsers; i++){
        if(strcmp(users[i], userId) == 0){
            users[i] = "-1";
        }
    }
    printf("User %s left the group!\n", userId);
}

void sendPrivateMessage(int sock){
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    char idFormatted[10];
    formatId(myId, idFormatted);

    sprintf(buf, "MSG(%s, %s, %s)", idFormatted, destinationId, privateMessage);
    sendMessage(sock, buf);
    printf("Private message\n");
}

void identifyCommand(char *command, int s){
    // remove \n from command
    command[strcspn(command, "\n")] = 0;
    
    if (strncmp(command, USER_COMMAND_CLOSE_CONNECTION, strlen(USER_COMMAND_CLOSE_CONNECTION)) == 0) {
        closeConnection(s);
    }else if(strncmp(command, USER_COMMAND_LIST_USERS, strlen(USER_COMMAND_LIST_USERS)) == 0) {
        aksUsersList();
    }else if(strncmp(command, USER_COMMAND_SEND_TO, strlen(USER_COMMAND_SEND_TO)) == 0) {
        sendTo(command, s);
    }else if(strncmp(command, USER_COMMAND_SEND_TO_ALL, strlen(USER_COMMAND_SEND_TO_ALL)) == 0) {
        sendToAll(command, s);
    }else if(strncmp(command, COMMAND_MESSAGE, strlen(COMMAND_MESSAGE)) == 0) {
        readMessage(command);
    }else if(strncmp(command, COMMAND_LIST_USERS, strlen(COMMAND_LIST_USERS)) == 0) {
        receiveUsersOnServer(command);
    }else if(strncmp(command, COMMAND_ERROR, strlen(COMMAND_ERROR)) == 0) {
        handleError(command);
    }else if(strncmp(command, OK, strlen(OK)) == 0) {
        handleConnectionClosed(s);
    }else if(strncmp(command, P_OK, strlen(P_OK)) == 0) {
        sendPrivateMessage(s);
    }else if(strncmp(command, COMMAND_CLOSE_CONNECTION, strlen(COMMAND_CLOSE_CONNECTION)) == 0) {
        handleAnotherUserLeftingTheGroup(command);
    }
    else {
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
    puts(buf);
}

void *listenServer(void *socket){
    int s = *(int *)socket;
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    while(1){
        unsigned total = 0;
        while(1){
            size_t count = recv(s, buf + total, BUFSZ - total, 0);
            if(buf[strlen(buf) - 1] == ')'){
                // Connection terminated
                break;
            }
            total += count;
        }
        puts(buf);
        identifyCommand(buf, s);
        memset(buf, 0, BUFSZ);    
    }
    pthread_exit(NULL);
}

void openConnection(int s){
    sendMessage(s, COMMAND_OPEN_CONNECTION);
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

    openConnection(s);

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    printf("connected to %s\n", addrstr);
   
    pthread_t threadListenServer;
    pthread_create(&threadListenServer, NULL, listenServer, &s);

    while(1){
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        fgets(buf, BUFSZ -1, stdin);
        identifyCommand(buf, s);

        sendMessage(s, buf);
        
    }

    close(s);
    exit(EXIT_SUCCESS);
    
}