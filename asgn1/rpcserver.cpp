#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>

int64_t mathFun(uint8_t buffer[], char op, uint8_t *ifError){
    int64_t a;
    int64_t b;

    for(int i=6; i<14; i++){
        a = a << 8 | (int64_t) buffer[i];
    }

    for(int j=14; j<22; j++){
        b = b << 8 | (int64_t) buffer[j];
    }

    if(op == '+'){
        if(a > (LONG_MAX - b) && b > 0){
            *ifError = 0x22;
        }
        else if(a < (LONG_MIN - b) && b < 0){
            *ifError = 0x22;
        }
        else{
            *ifError = 0;
        }
        return a+b;
    }
    else if(op == '-'){
        if(a > (LONG_MAX + b) && b < 0){
            *ifError = 0x22;
        }
        else if(a < (LONG_MIN + b) && b > 0){
            *ifError = 0x22;
        }
        else{
            *ifError = 0;
        }
        return a-b;
    }
    else{
        if(a > LONG_MAX / b){
            *ifError = 0x22;
        }
        else if(a < LONG_MIN / b){
            *ifError = 0x22;
        }
        else if(a == LONG_MIN && b == -1){
            *ifError = 0x22;
        }
        else if(b == LONG_MIN && a == -1){
            *ifError = 0x22;
        }
        else{
            *ifError = 0;
        }
        return a*b;
    }
    
    return 0;
}

// int64_t fileMod(uint8_t buffer[]){
//     char* filename;
//     uint64_t offset;
//     uint16_t bufsize;
//     uint8_t *buffer;
// }

// int64_t fileFun(uint8_t buffer[]){
//     char* filename;
//     uint64_t offset;
//     uint16_t bufsize;
//     uint8_t *buffer;
// }



int main(int argc, char* argv[]){
    char* token;
    const char* s = ":";
    token = strtok(argv[1], s);
    token = strtok(NULL, s);

    struct hostent *hent = gethostbyname("localhost" /* eg "localhost" */); //setup sockets
    struct sockaddr_in addr;
    memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);
    addr.sin_port = htons(atoi(token));
    addr.sin_family = AF_INET;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    int enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    listen(sock, 0);
    

    uint8_t recvBuf[16384];
    uint8_t sendBuf[16384];
    uint16_t function;
    uint32_t identifier;
    int64_t result;
    size_t recvSize;
    uint8_t ifError;
    do{
        int cl = accept(sock, NULL, NULL);
        recvSize = read(cl, &recvBuf, 16384);
        if(recvSize > 0){
            for(size_t i=0; i<recvSize; i++){
                printf("%02x", recvBuf[i]);
            }
            printf("\nBytes recvied: %zu\n", recvSize);

            for(size_t t=0; t<4; t++){
                sendBuf[t] = recvBuf[t+2];
            }

            function = (uint16_t)recvBuf[0] << 8 | recvBuf[1];
            printf("Function called: %04x\n", function);

            for(int j=2; j<6; j++){
                identifier = identifier << 8 | recvBuf[j];
            }
            printf("identifier: %08x\n", identifier);

            switch(function){
                case 0x0101 :
                    result = mathFun(recvBuf, '+', &ifError);
                    sendBuf[4] = ifError;
                    printf("Add function called: %04x, get %016lx, size %lu\n", function, result, sizeof(result));
                    break;
                case 0x0102 :
                    result = mathFun(recvBuf, '-', &ifError);
                    sendBuf[4] = ifError;
                    printf("Sub function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0103 :
                    result = mathFun(recvBuf, '*', &ifError);
                    sendBuf[4] = ifError;
                    printf("Mul function called: %04x\n", function);
                    break;
                case 0x0201 :
                    printf("Read function called: %04x\n", function);
                case 0x0202 :
                    printf("Write function called: %04x\n", function);
                case 0x0210 :
                    printf("Create function called: %04x\n", function);
                case 0x0220 :
                    printf("Filesize function called: %04x\n", function);
                default:
                    break;
            }

            if(sendBuf[4] == 0){
                for(size_t k=5; k<sizeof(result)+5; k++){
                    sendBuf[k] = result >> 8*(sizeof(result) - k + 4) & 0xff;
                }
            }
            if(write(cl, sendBuf, sizeof(sendBuf)) == -1){
                printf("Write failed");
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