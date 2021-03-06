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

int64_t mathFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer){ //contains add, sub, mul 
    int64_t a;
    int64_t b;

    for(size_t i= *pointer; i< *pointer+8; i++){ //retrive first 8 bytes as a
        a = a << 8 | (int64_t) buffer[i];
    }
    *pointer += 8;

    for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
        b = b << 8 | (int64_t) buffer[j];
    }
    *pointer += 8;

    if(op == '+'){
        if(a > (LONG_MAX - b) && b > 0){ //overflow test
            *ifError = 22; //set ifError to 22 if overflow
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
        if(a > (LONG_MAX + b) && b < 0){ //overflow test
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
        if(a == 0 | b == 0){ //overflow test
            return 0;
        }

        if(abs(a) > LONG_MAX / abs(b) && a!=0 && b!=0){
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

int64_t fileMod(uint8_t buffer[], char op, uint8_t *ifError, uint8_t *readBuf, uint8_t *sendBuf, int cl){
    size_t pointer = 8;
    char* fileName;
    uint64_t offset;
    uint16_t bufsize;
    uint8_t fileBuf[8192];
    uint16_t fileNameSize;
    int64_t fd;
    //uint16_t count = 0;
    int64_t res = 0;

    fileNameSize = (uint16_t)buffer[6] << 8 | buffer[7]; //retrive filename size
    char fname[fileNameSize+1];
    for(size_t i=0; i<fileNameSize; i++){ //retrive filename
        fname[i] = (char) buffer[i+8];
        pointer += 1;
    }
    fname[fileNameSize] = (char) '\0';
    fileName = fname;
    printf("file name: %s\n", fileName); //for testing

    for(size_t j=pointer; j<(pointer+8); j++){ //retrive offset
        offset = offset << 8 | (int64_t) buffer[j];
    }
    pointer += 8;
    printf("offset: %016lx\n", offset);
    

    bufsize = (uint16_t)buffer[pointer] << 8 | buffer[pointer+1]; //retrive bufsize
    pointer += 2;
    printf("bufsize: %04x\n", bufsize);

    fd = open(fileName, O_RDWR); //open file
    if(fd < 0){
        *ifError = 2;
        return 0;
    }
    if(lseek(fd, offset, SEEK_SET)<0){ //set to offset
        *ifError = errno;
        return 0;
    }

    if(op == 'r'){
        while(read(fd, readBuf, 1) > 0 && res < bufsize){ //read from file and store to read buffer         
            res += 1;
            if(res+7-1 >= 16377){ //if read more than sendBuf can store
                sendBuf[5] = bufsize >> 8 & 0xff;
                sendBuf[6] = bufsize & 0xff;
                write(cl,sendBuf, 16384); //send back current buffer
                while(read(fd, readBuf, 1) > 0 && res <= bufsize){
                    res += 1;
                    write(cl,readBuf,1); //send back whatever read
                }
                res -= 1;
            }else{
                sendBuf[res+7-1] = readBuf[0];
            }
        }
        
        if(res<0){
            *ifError = errno;
            return 0;
        }
    }else{
        for(size_t k=0; k<bufsize; k++){ //retrive the write buffer
            fileBuf[k] = buffer[pointer];
            pointer += 1;
        }
        res += write(fd, fileBuf, bufsize); //write to the file
        if(res<0){
            *ifError = errno;
            return 0;
        }
    }

    close(fd);
    return res;
}

int64_t fileFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer){ //contains create and filesize
    char* fileName;
    uint16_t fileNameSize;

    fileNameSize = (uint16_t)buffer[*pointer] << 8 | buffer[*pointer+1]; //retrive file name size
    *pointer += 2;

    char fname[fileNameSize+1]; //retrive file name
    for(size_t i=0; i<fileNameSize; i++){
        fname[i] = (char) buffer[i+*pointer];
    }
    fname[fileNameSize] = (char) '\0';
    fileName = fname;
    *pointer += fileNameSize;
    

    if(op == 'c'){
        if(open(fileName, O_CREAT | O_EXCL) < 0){ //create the file 
            *ifError = 17; //set ifError to 17 if already exist
            return -1;
        }
    }else{
        struct stat st; //use stat to get file size
		if(stat(fileName, &st)<0){
            *ifError = 2; //set ifError to 2 if no such file
            return -1;
        }
		return st.st_size;
    }

    return 0;
}



int main(int argc, char* argv[]){
    char* port; //get hostname and port number from argv
    char* hostname;
    const char* s = ":";
    hostname = strtok(argv[1], s);
    port = strtok(NULL, s);

    struct hostent *hent = gethostbyname(hostname); //setup sockets
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
    uint8_t* recvPos = recvBuf;
    uint16_t function; //for testing
    uint32_t identifier; //for testing
    int64_t result;
    size_t recvSize;
    uint16_t fileNameSize;
    uint8_t ifError; //store error#
    uint8_t readBuf[1]; //store read data
    size_t pointer; //tracks the recvBuf position
    size_t sPointer; //tracks the sendBuf position
    size_t sendSize; 
    size_t cmdLength = 0; 
    uint8_t timer = 0;
    uint8_t alreadySent = 0;
    do{
        int cl = accept(sock, NULL, NULL);
        while(cmdLength < 8){ //if recv size is not enough for function check, identifier, file size, read more
            recvSize = read(cl, recvPos, 16384); //read from client
            cmdLength += recvSize;
            recvPos = cmdLength + recvBuf;
            timer += 1;
            if(timer > 50){ //get out of loop if client don't provide enough information
                break;
            }
        }
        timer = 0;

        if(cmdLength >= 8 && cmdLength <= 16384){
            do{
                if(pointer!=0){ //reset variables if get more than one argument from client
                    memset(sendBuf, 0, sizeof(sendBuf));
                    pointer = 0;
                    sPointer = 0;
                    ifError = 0;
                }
                 for(size_t i=0; i<cmdLength; i++){ //for testing print what received
                     printf("%02x", recvBuf[i]);
                 }
                printf("\nBytes recvied: %zu\n", cmdLength);

                function = (uint16_t)recvBuf[pointer] << 8 | recvBuf[pointer+1]; //record the function called
                pointer += 2;
                printf("Function called: %04x\n", function);

                for(size_t t=0; t<4; t++){ //store the identiier to the send buffer
                    sendBuf[t+sPointer] = recvBuf[pointer+t];
                }
                sPointer += 4;
                
                for(size_t j=pointer; j<pointer+4; j++){ //for testing
                    identifier = identifier << 8 | recvBuf[j];
                }
                pointer += 4;
                printf("identifier: %08x\n", identifier);

                switch(function){ //goto different function call
                    case 0x0101 :
                        while(cmdLength < 22){ //read to 22 bytes for math functions
                            recvSize = read(cl, recvPos, 16384); //read from client
                            cmdLength += recvSize;
                            recvPos = cmdLength + recvBuf;
                            timer += 1;
                            if(timer > 50){
                                break;
                            }
                        }
                        timer = 0;
                        result = mathFun(recvBuf, '+', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        printf("Add function called: %04x, get %016lx, size %lu\n", function, result, sizeof(result));
                        break;
                    case 0x0102 :
                        while(cmdLength < 22){ //read to 22 bytes for math functions
                            recvSize = read(cl, recvPos, 16384); //read from client
                            cmdLength += recvSize;
                            recvPos = cmdLength + recvBuf;
                            timer += 1;
                            if(timer > 50){
                                break;
                            }
                        }
                        timer = 0;
                        result = mathFun(recvBuf, '-', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        printf("Sub function called: %04x, get %016lx\n", function, result);
                        break;
                    case 0x0103 :
                        while(cmdLength < 22){ //read to 22 bytes for math functions
                            recvSize = read(cl, recvPos, 16384); //read from client
                            cmdLength += recvSize;
                            recvPos = cmdLength + recvBuf;
                            timer += 1;
                            if(timer > 50){
                                break;
                            }
                        }
                        timer = 0;
                        result = mathFun(recvBuf, '*', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        printf("Mul function called: %04x, get %016lx\n", function, result);
                        break;
                    case 0x0201 :
                        sendSize = 7;
                        result = fileMod(recvBuf, 'r', &ifError, readBuf, sendBuf, cl);
                        sendBuf[sPointer] = ifError;
                        sendSize = 7+result;
                        printf("Read function called: %04x, get: %zu\n", function, result);
                        break;
                    case 0x0202 :
                        sendSize = 5;
                        result = fileMod(recvBuf, 'w', &ifError, readBuf, sendBuf, cl);
                        sendBuf[sPointer] = ifError;
                        sendSize = 5;
                        printf("Write function called: %04x\n", function);
                        break;
                    case 0x0210 :
                        fileNameSize = (uint16_t)recvBuf[pointer] << 8 | recvBuf[pointer+1]; //retrive file name size
                        while(cmdLength < 7+fileNameSize){ //read to 7+fileNameSize bytes for create functions
                            recvSize = read(cl, recvPos, 16384); //read from client
                            cmdLength += recvSize;
                            recvPos = cmdLength + recvBuf;
                            timer += 1;
                            if(timer > 50){
                                break;
                            }
                        }
                        timer = 0;
                        result = fileFun(recvBuf, 'c', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        sendSize = 5;
                        printf("Create function called: %04x, get %016lx, %d\n", function, result, ifError);
                        break;
                    case 0x0220 :
                        fileNameSize = (uint16_t)recvBuf[pointer] << 8 | recvBuf[pointer+1]; //retrive file name size
                        while(cmdLength < 7+fileNameSize){ //read to 7+fileNameSize bytes for filesize functions
                            recvSize = read(cl, recvPos, 16384); //read from client
                            cmdLength += recvSize;
                            recvPos = cmdLength + recvBuf;
                            timer += 1;
                            if(timer > 50){
                                break;
                            }
                        }
                        timer = 0;
                        result = fileFun(recvBuf, 's', &ifError, &pointer);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        printf("Filesize function called: %04x, get %016lx\n", function, result);
                        break;
                    default:
                        break;
                }
                
                if(sendBuf[sPointer] == 0 && function != 0x0210 && function != 0x0201){ //if no error and function is not create or read write the result to the send buffer
                    sPointer += 1;
                    for(size_t k=1; k<=sizeof(result); k++){ //store the result to the send buffer
                        sendBuf[k+sPointer-1] = result >> 8*(sizeof(result) - k) & 0xff;
                    }
                    sPointer += sizeof(result);
                }
                if(sendBuf[sPointer] == 0  && function == 0x0201 && result < 16378){ //if read function called
                    sPointer += 1;
                    for(size_t k=1; k<=2; k++){ //store the buffer length to the send buffer
                        sendBuf[k+sPointer-1] = (uint16_t) result >> 8*(2 - k) & 0xff;
                    }
                    sPointer += 2;

                    sPointer += result;
                }else if(sendBuf[sPointer] == 0  && function == 0x0201 && result > 16378){
                    alreadySent = 1;
                }

                //  for(size_t i=0; i<sendSize; i++){ //for testing print what sent
                //      printf("%02x", sendBuf[i]);
                //  }
                // printf("\nBytes sent: %zu\n", sendSize);

                if(alreadySent == 0){
                    if(write(cl, sendBuf, sendSize) <= 0){ //send the sendBuf to client
                        printf("Write failed");
                    }
                }

                cmdLength = 0;  //reset variables
                alreadySent = 0;
                memset(recvBuf, 0, sizeof(recvBuf));
                recvPos = recvBuf;
                while(cmdLength < 8){
                    recvSize = read(cl, recvPos, 16384); //read from client again
                    cmdLength += recvSize;
                    recvPos = cmdLength + recvBuf;
                    timer += 1;
                    if(timer > 50){
                        break;
                    }
                }
                if(timer > 50){
                    break;
                }
                timer = 0;
            }while(cmdLength > 0 && cmdLength < 16384); //if there's more from the client repeat

            alreadySent = 0;
            cmdLength = 0;  //reset variables
            memset(recvBuf, 0, sizeof(recvBuf));
            recvPos = recvBuf;
            pointer = 0;
            sPointer = 0;
            timer = 0;
        }
        else if(cmdLength==0){ //for testing
            printf("Connection closed\n");
        }
        else{ //for testing
            printf("Recv failed\n");
            exit(EXIT_FAILURE);
        }
    }while(true);

    return(EXIT_SUCCESS);
}