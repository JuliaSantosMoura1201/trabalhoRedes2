#include "common.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSZ 1024

User users[MAX_OF_USERS];
int amountOfUsers = 0;
int nextId = 1;

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

User addUserToList(User *usersList, uint16_t port, int sock) {
    // adiciono na última posição disponível
    // id 
    User newUser;
    newUser.port = port;
    newUser.id = nextId;
    newUser.sock = sock;

    usersList[amountOfUsers] = newUser;

    nextId++;
    amountOfUsers++;
    return newUser;
}

void sendMessage(int sock, char* buf){
    size_t count = send(sock, buf, strlen(buf)+1, 0);
    if(count != strlen(buf) + 1){
        logexit("send");
    }
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

    size_t count = send(sock, buf, strlen(buf)+1, 0);
    if(count != strlen(buf) + 1){
        logexit("send");
    }
}

void broadcast(int id, char *idFormatted){
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "MSG(%d, NULL, User %s joined the group!)", id, idFormatted);

    for(int i = 0; i < amountOfUsers - 1; i++){
        size_t count = send(users[i].sock, buf, strlen(buf)+1,0);
        if(count != strlen(buf) + 1){
            logexit("send");
        }
    }
}

void sendMaxNumberOfUsersError(int sock){
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "%s(%s)", COMMAND_ERROR, ERROR_CODE_USER_LIMIT_EXCEED);

    sendMessage(sock, buf);
}

void openConnection(struct sockaddr *caddr, int csock){
    if(amountOfUsers == MAX_OF_USERS){
        sendMaxNumberOfUsersError(csock);
        return;
    }
    User newUser = addUserToList(users, getPort(caddr), csock);
    
    char idFormatted[100];
    formatId(newUser.id, idFormatted);
    printf("User %s added\n", idFormatted);

    broadcast(newUser.id, idFormatted);
    sendListOfUsers(csock);
}

void identifyCommand(char *command, struct sockaddr *caddr, struct client_data *cdata){
    // remove \n from command
    command[strcspn(command, "\n")] = 0;
    
    if (strncmp(command, COMMAND_OPEN_CONNECTION, strlen(COMMAND_OPEN_CONNECTION)) == 0) {
        openConnection(caddr, cdata->csock);
    }else if(strncmp(command, COMMAND_CLOSE_CONNECTION, strlen(COMMAND_CLOSE_CONNECTION)) == 0) {
        printf("COMMAND_CLOSE_CONNECTION\n");
    }else if(strncmp(command, COMMAND_MESSAGE, strlen(COMMAND_MESSAGE)) == 0) {
        printf("COMMAND_MESSAGE\n");
    }else if(strncmp(command, COMMAND_ERROR, strlen(COMMAND_ERROR)) == 0) {
        printf("COMMAND_ERROR\n");
    }else if(strncmp(command, COMMAND_LIST_USERS, strlen(COMMAND_LIST_USERS)) == 0) {
         printf("COMMAND_LIST_USERS\n");
    } else {
        printf("command not identified\n");
    }
}

void *client_thread(void *data){
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    size_t count = recv(cdata->csock, buf, BUFSZ, 0);
    if(count < 0){
        logexit("recv");
    }

    identifyCommand(buf, caddr, cdata);

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
        
        pthread_join(tid, NULL);
    }
    exit(EXIT_SUCCESS);
}
