#include "common.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSZ 1024
#define MAX_OF_USERS 15

typedef struct User{
    uint16_t port;
    int id;
    int sock;
} User;

User users[MAX_OF_USERS];
int amountOfUsers = 0;
int nextId = 1;

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

/*
void identifyCommand(char *command, int s){
    // remove \n from command
    command[strcspn(command, "\n")] = 0;
    
    if (strncmp(command, COMMAND_OPEN_CONNECTION, strlen(COMMAND_OPEN_CONNECTION)) == 0) {
        closeConnection();
    }else if(strncmp(command, COMMAND_CLOSE_CONNECTION, strlen(COMMAND_CLOSE_CONNECTION)) == 0) {
        listUsers();
    }else if(strncmp(command, COMMAND_MESSAGE, strlen(COMMAND_MESSAGE)) == 0) {
        sendTo();
    }else if(strncmp(command, COMMAND_ERROR, strlen(COMMAND_ERROR)) == 0) {
        sendToAll();
    }}else if(strncmp(command, COMMAND_LIST_USERS, strlen(COMMAND_LIST_USERS)) == 0) {
        sendToAll();
    } else {
        printf("command not identified\n");
    }
}
*/

User addUserToList(uint16_t port, int sock) {
    // adiciono na última posição disponível
    // id 
    User newUser;
    newUser.port = port;
    newUser.id = nextId;
    newUser.sock = sock;

    users[amountOfUsers] = newUser;

    nextId++;
    amountOfUsers++;
    return newUser;
}

void unicast(int sock){

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    sprintf(buf, "RES_LIST(");

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

void openConnection(struct sockaddr *caddr, int csock){
    User newUser = addUserToList(getPort(caddr), csock);
    
    char idFormatted[100];
    formatId(newUser.id, idFormatted);
    printf("User %s added\n", idFormatted);

    broadcast(newUser.id, idFormatted);
    unicast(csock);
}

void *client_thread(void *data){
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    size_t count = recv(cdata->csock, buf, BUFSZ, 0);
    printf("[msg] %s, %d bytes: %s \n", caddrstr, (int)count, buf);

    openConnection(caddr, cdata->csock);
    /*
    sprintf(buf, "remote endpoint: %.1000s\n", caddrstr);
    count = send(cdata->csock, buf, strlen(buf)+1,0);
    if(count != strlen(buf) + 1){
        logexit("send");
    }
    
    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
    */
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
