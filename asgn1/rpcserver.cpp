#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>



int main(int argc, char* argv[]){
    struct hostent *hent = gethostbyname(SERVER_NAME_STRING /* eg "localhost" */); //setup sockets
    struct sockaddr_in addr;
    memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);
    addr.sin_port = htons(PORT_NUMBER);
    addr.sin_family = AF_INET;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    int enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(sock, 0);
    int cl = accept(sock, NULL, NULL);

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));


    uint8_t type8;
    uint16_t type16;
    uint32_t type32;
    uint64_t type64;
    uint8_t *words;

}