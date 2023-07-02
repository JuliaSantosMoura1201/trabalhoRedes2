#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

void logexit(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

int addrparse(
    const char *addrstr, 
    const char *portstr,
    struct sockaddr_storage *storage
){
    if(addrstr == NULL || portstr == NULL){
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr);
    if(port == 0){
        return -1;
    }
    port = htons(port); //host to network short

    struct in_addr inaddr4; //32 bits IP address
    if(inet_pton(AF_INET, addrstr, &inaddr4)){
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; //128 bits IP address
    if(inet_pton(AF_INET6, addrstr, &inaddr6)){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize){
    int version;
    char addrstr[INET6_ADDRSTRLEN+1] = "";
    uint16_t port;

    if(addr->sa_family == AF_INET){
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if(!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET6_ADDRSTRLEN+ 1)){
            logexit("ntop");
        }
        port = ntohs(addr4->sin_port); //network to host short
    }else if(addr->sa_family == AF_INET6){
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if(!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN+ 1)){
            logexit("ntop");
        }
        port = ntohs(addr6->sin6_port); //network to host short
    }else{
        logexit("unknown protocol family.");
    }

    if(str){
        snprintf(str, strsize, "IPV%d %s %hu", version, addrstr, port);
    }
    
}

uint16_t getPort(const struct sockaddr *addr){
    uint16_t port;

    if(addr->sa_family == AF_INET){
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        port = ntohs(addr4->sin_port); //network to host short
    }else if(addr->sa_family == AF_INET6){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        port = ntohs(addr6->sin6_port); //network to host short
    }else{
        logexit("unknown protocol family.");
    }

    return port;
}


int server_sockadd_init(
    const char *porto,
    const char *portstr,
    struct sockaddr_storage *storage
){
    uint16_t port = (uint16_t)atoi(portstr);
    if(port == 0){
        return -1;
    }
    port = htons(port); //host to network short

    memset(storage, 0, sizeof(*storage));
    if(0 == strcmp(porto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr.s_addr = INADDR_ANY;
        return 0;
    } else if(0 == strcmp(porto, "v6")){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6->sin6_addr = in6addr_any;
        return 0;
    } else{
        return -1;
    }

}

void formatId(int id, char *str){
    if(id <= 9){
        sprintf(str, "0%d", id);
    }else{
        sprintf(str, "%d", id);
    }
}

char** splitString(const char* str, const char* delimiter, int* itemCount, int maxOfItems) {
    char* copy = strdup(str);
    char* token;
    char** items = malloc(maxOfItems * sizeof(char*));
    int count = 0;

    token = strtok(copy, delimiter);

    while (token != NULL && count < maxOfItems) {
        items[count] = strdup(token);
        count++;

        token = strtok(NULL, delimiter);
    }

    *itemCount = count;

    free(copy);

    return items;
}

void getsMessageContent(char *command, char *destination, char *pattern){
    int startIndex = strlen(pattern);
    int endIndex = strlen(command) - 2;
    
    memcpy(destination, command + startIndex + 1, endIndex - startIndex);
    destination[endIndex - startIndex] = '\0'; 
}

void sendMessage(int sock, char* buf){
    size_t count = send(sock, buf, strlen(buf)+1, 0);
   // printf("  sendMessage message = %s count = %zu strlen= %zu\n", buf, count, strlen(buf) + 1);
    if(count != strlen(buf) + 1){
        logexit("send");
    }
}