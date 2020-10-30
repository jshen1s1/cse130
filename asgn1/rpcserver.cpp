#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>

int64_t mathFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer){
    int64_t a;
    int64_t b;

    for(size_t i= *pointer; i< *pointer+8; i++){
        a = a << 8 | (int64_t) buffer[i];
    }
    *pointer += 8;

    for(size_t j= *pointer; j< *pointer+8; j++){
        b = b << 8 | (int64_t) buffer[j];
    }
    *pointer += 8;

    if(op == '+'){
        if(a > (LONG_MAX - b) && b > 0){
            *ifError = 22;
        }
        else if(a < (LONG_MIN - b) && b < 0){
            *ifError = 22;
        }
        else{
            *ifError = 0;
        }
        return a+b;
    }
    else if(op == '-'){
        if(a > (LONG_MAX + b) && b < 0){
            *ifError = 22;
        }
        else if(a < (LONG_MIN + b) && b > 0){
            *ifError = 22;
        }
        else{
            *ifError = 0;
        }
        return a-b;
    }
    else{
        if(a > LONG_MAX / b){
            *ifError = 22;
        }
        else if(a < LONG_MIN / b){
            *ifError = 22;
        }
        else if(a == LONG_MIN && b == -1){
            *ifError = 22;
        }
        else if(b == LONG_MIN && a == -1){
            *ifError = 22;
        }
        else{
            *ifError = 0;
        }
        return a*b;
    }
    
    return 0;
}

int64_t fileMod(uint8_t buffer[], char op, uint8_t *ifError){
    size_t pointer = 8;
    char* fileName;
    uint64_t offset;
    uint16_t bufsize;
    uint8_t *fileBuf;
    uint16_t fileNameSize;
    int64_t fd;
    int64_t res;

    fileNameSize = (uint16_t)buffer[6] << 8 | buffer[7];
    char fname[fileNameSize+1];
    for(size_t i=0; i<fileNameSize; i++){
        fname[i] = (char) buffer[i+8];
        pointer += 1;
    }
    fname[fileNameSize] = (char) '\0';
    fileName = fname;
    printf("file name: %s, %d\n", fileName, pointer);

    for(size_t j=pointer; j<(pointer+8); j++){
        offset = offset << 8 | (int64_t) buffer[j];
    }
    pointer += 8;
    printf("offset: %016x, %d\n", offset, pointer);
    

    bufsize = (uint16_t)buffer[pointer] << 8 | buffer[pointer+1];
    pointer += 2;
    printf("bufsize: %04x, %zu\n", bufsize, pointer);
    /*
    for(size_t k=0; k<bufsize; k++){
        fileBuf[k] = buffer[pointer];
        pointer += 1;
    }*/

    fd = open(fileName, O_RDONLY);
    if(fd < 0){
        *ifError = 2;
        return -1;
    }
    lseek(fd, offset, SEEK_SET);

    if(op == 'r'){
        res = read(fd, fileBuf, bufsize);
    }else{
        res = write(fd, fileBuf, bufsize);
    }

    close(fd);
    return res;
}

int64_t fileFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer){
    char* fileName;
    uint16_t fileNameSize;

    fileNameSize = (uint16_t)buffer[*pointer] << 8 | buffer[*pointer+1];
    *pointer += 2;

    char fname[fileNameSize+1];
    for(size_t i=0; i<fileNameSize; i++){
        fname[i] = (char) buffer[i+*pointer];
    }
    fname[fileNameSize] = (char) '\0';
    fileName = fname;
    *pointer += fileNameSize;
    

    if(op == 'c'){
        if(open(fileName, O_CREAT | O_EXCL) < 0){
            *ifError = 17;
            return -1;
        }
    }else{
        struct stat st;
		if(stat(fileName, &st)<0){
            *ifError = 2;
            return -1;
        }
		return st.st_size;
    }

    return 0;
}



int main(int argc, char* argv[]){
    char* port;
    char* hostname;
    const char* s = ":";
    hostname = strtok(argv[1], s);
    port = strtok(NULL, s);

    struct hostent *hent = gethostbyname(hostname /* eg "localhost" */); //setup sockets
    struct sockaddr_in addr;
    memcpy(&addr.sin_addr.s_addr, hent->h_addr, hent->h_length);
    addr.sin_port = htons(atoi(port));
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
    //uint8_t *readBuf[8192];
    size_t pointer;
    size_t sPointer;
    do{
        int cl = accept(sock, NULL, NULL);
        recvSize = read(cl, &recvBuf, 16384);
        if(recvSize > 0){
            do{
                for(size_t i=0; i<recvSize; i++){
                    printf("%02x", recvBuf[i]);
                }
                printf("\nBytes recvied: %zu\n", recvSize);

                function = (uint16_t)recvBuf[pointer] << 8 | recvBuf[pointer+1];
                pointer += 2;
                printf("Function called: %04x\n", function);

                for(size_t t=0; t<4; t++){
                    sendBuf[t+sPointer] = recvBuf[pointer+t];
                }
                sPointer += 4;
                
                for(size_t j=pointer; j<pointer+4; j++){
                    identifier = identifier << 8 | recvBuf[j];
                }
                pointer += 4;
                printf("identifier: %08x\n", identifier);

                switch(function){
                    case 0x0101 :
                        result = mathFun(recvBuf, '+', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        printf("Add function called: %04x, get %016lx, size %lu\n", function, result, sizeof(result));
                        break;
                    case 0x0102 :
                        result = mathFun(recvBuf, '-', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        printf("Sub function called: %04x, get %016lx\n", function, result);
                        break;
                    case 0x0103 :
                        result = mathFun(recvBuf, '*', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        printf("Mul function called: %04x\n", function);
                        break;
                    case 0x0201 :
                        result = fileMod(recvBuf, 'r', &ifError);
                        sendBuf[sPointer] = ifError;
                        printf("Read function called: %04x\n", function);
                        break;
                    case 0x0202 :
                        result = fileMod(recvBuf, 'w', &ifError);
                        sendBuf[sPointer] = ifError;
                        printf("Write function called: %04x\n", function);
                        break;
                    case 0x0210 :
                        result = fileFun(recvBuf, 'c', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        printf("Create function called: %04x, get %016lx, %d\n", function, result, ifError);
                        break;
                    case 0x0220 :
                        result = fileFun(recvBuf, 's', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        printf("Filesize function called: %04x, get %016lx\n", function, result);
                        break;
                    default:
                        break;
                }
                if(sendBuf[sPointer] == 0 && function != 0x0210){
                    sPointer += 1;
                    for(size_t k=1; k<=sizeof(result); k++){
                        sendBuf[k+sPointer-1] = result >> 8*(sizeof(result) - k) & 0xff;
                        printf("%02x %d", sendBuf[k+sPointer], k+sPointer);
                    }
                    sPointer += sizeof(result);
                }
            }while(recvBuf[pointer+1] != '\0');

            pointer = 0;
            sPointer = 0;

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