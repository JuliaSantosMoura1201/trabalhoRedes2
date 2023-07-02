#include "common.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define BUFSZ 1024

User users[MAX_OF_USERS];
int amountOfUsers = 0;
int amountOfValidUsers = 0;
int nextId = 1;

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

User addUserToList(User *usersList, uint16_t port, int sock) {

    for(int i = 0; i < amountOfUsers; i++){
        if(users[i].id == -1){
            users[i].id = nextId;
            users[i].port = port;
            users[i].sock = sock;

            nextId++;
            amountOfUsers++;
            amountOfValidUsers++;
            return users[i];
        }
    }

    User newUser;
    newUser.port = port;
    newUser.id = nextId;
    newUser.sock = sock;
    
    usersList[amountOfUsers] = newUser;

    nextId++;
    amountOfUsers++;
    amountOfValidUsers++;
    return newUser;
}

void sendListOfUsers(int sock){
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "%s(", COMMAND_LIST_USERS);

    for(int i = 0; i < amountOfUsers - 1; i++){
        char temp[BUFSZ];
        memset(temp, 0, BUFSZ);
        sprintf(temp, "%d,", users[i].id);
        
        strcat(buf, temp);
    }
    strcat(buf, ")");
    buf[strlen(buf)] = '\0';

    printf("%s\n", buf);

    sendMessage(sock, buf);
}

void broadcast(char *message){
    printf("Amount of users %d\n", amountOfUsers);
    for(int i = 0; i < amountOfUsers; i++){
        printf("ID users[%d].id = %d\n", i, users[i].id );
        if(users[i].id != -1){
            printf("  broadcast sock %i %s\n", users[i].sock, message);
            sendMessage(users[i].sock, message);
        }
    }
}

void sendErrorMessage(int sock, char* errorCode){
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "%s(%s)", COMMAND_ERROR, errorCode);

    sendMessage(sock, buf);
}

void openConnection(struct sockaddr *caddr, int csock){
    if(amountOfValidUsers == MAX_OF_USERS){
        sendErrorMessage(csock, ERROR_CODE_USER_LIMIT_EXCEED);
        return;
    }
    User newUser = addUserToList(users, getPort(caddr), csock);
    
    char idFormatted[10];
    formatId(newUser.id, idFormatted);
    printf("User %s added\n", idFormatted);
    sendListOfUsers(csock);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "MSG(%d, NULL, User %s joined the group!)", newUser.id, idFormatted);
    buf[strlen(buf)] = '\0';
    broadcast(buf);

    printf("  open connection before send list\n");
}

void removeUser(int userPosition){
    amountOfValidUsers--;
    users[userPosition].id = -1;
}

void findUser(int id, int sock){
    bool foundUser = false;
    for(int i = 0; i < amountOfUsers; i++){
        if(users[i].id  == id){
            removeUser(i);
            sendMessage(sock, OK);
            close(sock);

            char idFormatted[10];
            formatId(id, idFormatted);
            printf("User %s removed\n", idFormatted);
            foundUser = true;
        }
    }

    if(!foundUser){ 
        sendErrorMessage(sock, ERROR_CODE_USER_NOT_FOUND);
    }
}

void closeConnection(char *command, int sock){
    char userFormatedId[BUFSZ];
    memset(userFormatedId, 0, BUFSZ);
    getsMessageContent(command, userFormatedId, COMMAND_CLOSE_CONNECTION);

    int id = atoi(userFormatedId);
    findUser(id, sock);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "%s(%s)",COMMAND_CLOSE_CONNECTION, userFormatedId);
    puts(buf);
    broadcast(buf);
}

void readPrivateMessage(char **items, int sock, int amountOfItems){
    bool foundUser = false;
    for(int i = 0; i < amountOfUsers; i++){
        if(users[i].id  == atoi(items[0])){
            foundUser = true;
        }
    }

    if(foundUser){
        sendMessage(sock, P_OK);
    }else{
        sendErrorMessage(sock, ERROR_CODE_RECEIVER_NOT_FOUND);
    }

    for (int i = 0; i < amountOfItems; i++) {
        free(items[i]);
    }

    free(items);
}

void sendMessageWithTimer(char *idUserTranmmissor, char *originalMessage, int sock){
    time_t currentTime;
    struct tm* localTime;
    currentTime = time(NULL);
    localTime = localtime(&currentTime);
    printf("Current time: %02d:%02d\n", localTime->tm_hour, localTime->tm_min);

    
    char messageToUsers[BUFSZ];
    memset(messageToUsers, 0, BUFSZ);
    sprintf(messageToUsers, "[%02d:%02d] %s:%s", localTime->tm_hour, localTime->tm_min, idUserTranmmissor, originalMessage);
    puts(messageToUsers);

    char finalMessageToUsers[BUFSZ];
    memset(finalMessageToUsers, 0, BUFSZ);
    sprintf(finalMessageToUsers, "MSG(%d, NULL, [%02d:%02d] %s:%s)", atoi(idUserTranmmissor), localTime->tm_hour, localTime->tm_min, idUserTranmmissor, originalMessage);

    for(int i = 0; i < amountOfUsers; i++){
        if(users[i].id != -1 && users[i].id != sock){
            sendMessage(users[i].sock, finalMessageToUsers);
        }
    }

    
    char messageToTransmiter[BUFSZ];
    memset(messageToTransmiter, 0, BUFSZ);
    sprintf(messageToTransmiter, "MSG(%d, NULL, [%02d:%02d] -> all:%s)", atoi(idUserTranmmissor), localTime->tm_hour, localTime->tm_min, originalMessage);
    printf("Message to transmiter\n");
    puts(messageToTransmiter);
    sendMessage(sock, messageToTransmiter);
}

void sendPrivateMessage(char **items){
    time_t currentTime;
    struct tm* localTime;
    currentTime = time(NULL);
    localTime = localtime(&currentTime);

    char privateMessage[BUFSZ];
    memset(privateMessage, 0, BUFSZ);
    sprintf(privateMessage, "MSG(%s, %s, P [%02d:%02d] %s:%s)", items[0], items[1], localTime->tm_hour, localTime->tm_min, items[0], items[2]);

    for(int i = 0; i < amountOfUsers; i++){
        if(users[i].id  == atoi(items[1])){        
            sendMessage(users[i].sock, privateMessage);
        }
    }
}

void sendPublicMessage(char **items, char *command, int sock){
    printf("Item 0: %s Item 1: %s Item 2: %s\n", items[0], items[1], items[2]);
    
    if(strcmp(" NULL", items[1]) == 0){
        sendMessageWithTimer(items[0], items[2], sock);
    }else {
        sendPrivateMessage(items);
    }
    
}

void readMessage(char *command, int sock){
    char msg[BUFSZ];
    memset(msg, 0, BUFSZ);
    getsMessageContent(command, msg, COMMAND_MESSAGE);

    int amountOfItems = 3;
    char **items = splitString(msg, ",", &amountOfItems, amountOfItems);

    if(amountOfItems == 2){
        printf("PRIVATE\n");
        readPrivateMessage(items, sock, amountOfItems);
    }else if(amountOfItems == 3){
        printf("PUBLIC\n");
        
        sendPublicMessage(items, command, sock);
    }
}


void identifyCommand(char *command, struct sockaddr *caddr, struct client_data *cdata){
    // remove \n from command
    command[strcspn(command, "\n")] = 0;
    
    if (strncmp(command, COMMAND_OPEN_CONNECTION, strlen(COMMAND_OPEN_CONNECTION)) == 0) {
        openConnection(caddr, cdata->csock);
    }else if(strncmp(command, COMMAND_CLOSE_CONNECTION, strlen(COMMAND_CLOSE_CONNECTION)) == 0) {
        closeConnection(command, cdata->csock);
    }else if(strncmp(command, COMMAND_MESSAGE, strlen(COMMAND_MESSAGE)) == 0) {
       readMessage(command, cdata->csock);
    }else if(strncmp(command, COMMAND_ERROR, strlen(COMMAND_ERROR)) == 0) {
        printf("COMMAND_ERROR\n");
    }else if(strncmp(command, COMMAND_LIST_USERS, strlen(COMMAND_LIST_USERS)) == 0) {
         printf("COMMAND_LIST_USERS\n");
    } else {
        //printf("command not identified\n");
    }
}

void *client_thread(void *data){
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    
    while(1){
        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);

        size_t count = recv(cdata->csock, buf, BUFSZ, 0);
        if(count < 0){
            logexit("recv");
        }

        identifyCommand(buf, caddr, cdata);
    }
    

    pthread_exit(NULL);
}

void usage(int argc, char **argv){
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv){
    if(argc < 3){
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if(server_sockadd_init(argv[1], argv[2], &storage) != 0){
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    int enable = 1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))!= 0){
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(bind(s, addr, sizeof(storage)) != 0){
        logexit("bind");
    }

    if(listen(s, MAX_OF_USERS) != 0){
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections \n", addrstr);

    while(1){
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if(csock == -1){
            logexit("accept");
        }

        struct client_data *cdata = malloc(sizeof(*cdata));
        if(!cdata){
            logexit("malloc");
        }
        cdata->csock = csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
        
        //pthread_join(tid, NULL);
    }
    exit(EXIT_SUCCESS);
}
