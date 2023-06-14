#pragma once 
#include <stdlib.h>
#include <arpa/inet.h>

#define COMMAND_CLOSE_CONNECTION "REQ_REM"
#define COMMAND_OPEN_CONNECTION  "REQ_ADD"
#define COMMAND_MESSAGE "MSG"
#define COMMAND_ERROR "ERROR"
#define COMMAND_LIST_USERS "RES_LIST"

char *formatId(int id, char *str);
uint16_t getPort(const struct sockaddr *addr);
void logexit(const char *msg);
int addrparse(
    const char *addrstr, 
    const char *portstr,
    struct sockaddr_storage *storage
);
void addrtostr(
    const struct sockaddr *addr,
    char *str,
    size_t strsize
);
int server_sockadd_init(
    const char *porto,
    const char *portstr,
    struct sockaddr_storage *storage
);