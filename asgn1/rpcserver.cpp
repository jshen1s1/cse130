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
    struct hostent *hent = gethostbyname("localhost" /* eg "localhost" */); //setup sockets
    struct sockaddr_in addr;
    memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_family = AF_INET;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    int enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(sock, 0);
    int cl = accept(sock, NULL, NULL);

    uint8_t recvBuf[16384];
    uint16_t function;
    uint32_t identifier;
    size_t recvSize;
    do{
        recvSize = read(cl, &recvBuf, 16384);
        if(recvSize > 0){
            for(size_t i=0; i<recvSize; i++){
                printf("%02x", recvBuf[i]);
            }
            printf("\nBytes recvied: %d\n", recvSize);

            function = (uint16_t)recvBuf[0] << 8 | recvBuf[1];
            printf("\nFunction called: %04x\n", function);

            for(int j=2; j<6; j++){
                identifier = identifier << 8 | recvBuf[j];
            }
            printf("\nidentifier: %08x\n", identifier);

            switch(function){
                case 0x0101 :
                    math(recvBuf);
                    printf("\nAdd function called: %04x\n", function);
                    break;
                case 0x0102 :
                    printf("\nSub function called: %04x\n", function);
                    break;
                case 0x0103 :
                    printf("\nMul function called: %04x\n", function);
                    break;
            }
        }
        else if(recvSize==0){
            printf("Connection closed\n");
        }
        else{
            printf("Recv failed\n");
            exit(EXIT_FAILURE);
        }
    }while(recvSize > 0);

    return(EXIT_SUCCESS);
}

int64_t math(uint8_t buffer[]){

}