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
#include <err.h>
#include <limits.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include "hashTable.h"
#include "functions.h"

//sample code thread from https://piazza.com/class/kex6x7ets2p35c?cid=291
struct Thread {
    sem_t mutex;
    int id;
    int cl;
    int I;
    char* d;
    HashTable* ht;
};
typedef struct Thread Thread;

sem_t mainMutex;

void process(int cl, HashTable* ht, int I, char* d) {
    uint8_t recvBuf[65536];
    uint8_t sendBuf[65536];
    uint8_t* recvPos = recvBuf;
    uint8_t f1;
    uint8_t f2;
    uint16_t function; //for testing
    uint32_t identifier; //for testing
    int64_t result;
    size_t recvSize;
    uint16_t fileNameSize;
    uint8_t ifError; //store error#
    uint8_t readBuf[1]; //store read data
    size_t pointer = 0; //tracks the recvBuf position
    size_t sPointer = 0; //tracks the sendBuf position
    size_t sendSize; 
    size_t cmdLength = 0; 
    uint8_t timer = 0;
    uint8_t alreadySent = 0;
    uint32_t magicN;
    uint8_t done = 0;


    while(cmdLength < 8){ //if recv size is not enough for function check, identifier, file size, read more
        recvSize = read(cl, recvPos, 65536); //read from client
        cmdLength += recvSize;
        recvPos = cmdLength + recvBuf;
        timer += 1;
        if(timer > 50){ //get out of loop if client don't provide enough information
            break;
        }
    }
    timer = 0;

    if(cmdLength >= 8 && cmdLength <= 65536){
        do{
            if(pointer!=0){ //reset variables if get more than one argument from client
                memset(sendBuf, 0, sizeof(sendBuf));
                pointer = 0;
                sPointer = 0;
                ifError = 0;
                done = 0;
                printf("reset\n");
            }
            for(size_t i=0; i<cmdLength; i++){ //for testing print what received
                printf("%02x", recvBuf[i]);
            }
            printf("\nBytes recvied: %zu\n", cmdLength);

            function = (uint16_t)recvBuf[0] << 8 | recvBuf[1];
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

            switch(function){ //check functions without flag
                case 0x0101 : //add
                    while(cmdLength < 22){ //read to 22 bytes for math functions
                        recvSize = read(cl, recvPos, 65536); //read from client
                        cmdLength += recvSize;
                        recvPos = cmdLength + recvBuf;
                        timer += 1;
                        if(timer > 50){
                            break;
                        }
                    }
                    timer = 0;
                    result = mathFun(recvBuf, '+', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Add function called: %04x, get %016lx, error %02x\n", function, result, ifError);
                    break;
                case 0x0102 : //sub
                    while(cmdLength < 22){ //read to 22 bytes for math functions
                        recvSize = read(cl, recvPos, 65536); //read from client
                        cmdLength += recvSize;
                        recvPos = cmdLength + recvBuf;
                        timer += 1;
                        if(timer > 50){
                            break;
                        }
                    }
                    timer = 0;
                    result = mathFun(recvBuf, '-', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Sub function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0103 : //mul
                    while(cmdLength < 22){ //read to 22 bytes for math functions
                        recvSize = read(cl, recvPos, 65536); //read from client
                        cmdLength += recvSize;
                        recvPos = cmdLength + recvBuf;
                        timer += 1;
                        if(timer > 50){
                            break;
                        }
                    }
                    timer = 0;
                    result = mathFun(recvBuf, '*', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Mul function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0104 : //div
                    while(cmdLength < 22){ //read to 22 bytes for math functions
                        recvSize = read(cl, recvPos, 65536); //read from client
                        cmdLength += recvSize;
                        recvPos = cmdLength + recvBuf;
                        timer += 1;
                        if(timer > 50){
                            break;
                        }
                    }
                    timer = 0;
                    result = mathFun(recvBuf, '/', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Div function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0105 : //mod 
                    while(cmdLength < 22){ //read to 22 bytes for math functions
                        recvSize = read(cl, recvPos, 65536); //read from client
                        cmdLength += recvSize;
                        recvPos = cmdLength + recvBuf;
                        timer += 1;
                        if(timer > 50){
                            break;
                        }
                    }
                    timer = 0;
                    result = mathFun(recvBuf, '%', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Mod function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0108 : //getv
                    result = getv(recvBuf, &ifError, &pointer, ht, sendBuf);
                    sendBuf[sPointer] = ifError;
                    sendSize = 6+result;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Getv function called: %04x, get %02lx\n", function, result);
                    break;
                case 0x0109 : //setv
                    result = setv(recvBuf, &ifError, &pointer, ht, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 5;
                    done = 1;
                    printAll(ht);
                    printf("Setv function called: %04x\n", function);
                    break;
                case 0x010f : //del
                    result = mathFun(recvBuf, '*', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 5;
                    done = 1;
                    printAll(ht);
                    printf("Del function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0201 : //read
                    sendSize = 7;
                    result = fileMod(recvBuf, 'r', &ifError, readBuf, sendBuf, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 7+result;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printf("Read function called: %04x, get: %zu\n", function, result);
                    break;
                case 0x0202 : //write
                    sendSize = 5;
                    result = fileMod(recvBuf, 'w', &ifError, readBuf, sendBuf, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 5;
                    printf("Write function called: %04x\n", function);
                    break;
                case 0x0210 : //create
                    fileNameSize = (uint16_t)recvBuf[pointer] << 8 | recvBuf[pointer+1]; //retrive file name size
                    while(cmdLength < 8+fileNameSize){ //read to 8+fileNameSize bytes for create functions
                        recvSize = read(cl, recvPos, 65536); //read from client
                        cmdLength += recvSize;
                        recvPos = cmdLength + recvBuf;
                        timer += 1;
                        if(timer > 50){
                            break;
                        }
                    }
                    timer = 0;
                    result = fileFun(recvBuf, 'c', &ifError, &pointer, ht);
                    sendBuf[sPointer] = ifError;
                    sendSize = 5;
                    printf("Create function called: %04x, get %016lx, %d\n", function, result, ifError);
                    break;
                case 0x0220 : //filesize
                    fileNameSize = (uint16_t)recvBuf[pointer] << 8 | recvBuf[pointer+1]; //retrive file name size
                    while(cmdLength < 8+fileNameSize){ //read to 8+fileNameSize bytes for filesize functions
                        recvSize = read(cl, recvPos, 65536); //read from client
                        cmdLength += recvSize;
                        recvPos = cmdLength + recvBuf;
                        timer += 1;
                        if(timer > 50){
                            break;
                        }
                    }
                    timer = 0;
                    result = fileFun(recvBuf, 's', &ifError, &pointer, ht);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printf("Filesize function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0301 : //dump
                    fileNameSize = (uint16_t)recvBuf[pointer] << 8 | recvBuf[pointer+1]; //retrive file name size
                    while(cmdLength < 8+fileNameSize){ //read to 8+fileNameSize bytes for filesize functions
                        recvSize = read(cl, recvPos, 65536); //read from client
                        cmdLength += recvSize;
                        recvPos = cmdLength + recvBuf;
                        timer += 1;
                        if(timer > 50){
                            break;
                        }
                    }
                    timer = 0;
                    result = fileFun(recvBuf, 'd', &ifError, &pointer, ht);
                    sendBuf[sPointer] = ifError;
                    sendSize = 5;
                    printf("Dump function called: %04x\n", function);
                    break;
                case 0x0302 : //load
                    fileNameSize = (uint16_t)recvBuf[pointer] << 8 | recvBuf[pointer+1]; //retrive file name size
                    while(cmdLength < 8+fileNameSize){ //read to 8+fileNameSize bytes for filesize functions
                        recvSize = read(cl, recvPos, 65536); //read from client
                        cmdLength += recvSize;
                        recvPos = cmdLength + recvBuf;
                        timer += 1;
                        if(timer > 50){
                            break;
                        }
                    }
                    timer = 0;
                    result = fileFun(recvBuf, 'l', &ifError, &pointer, ht);
                    sendBuf[sPointer] = ifError;
                    sendSize = 5;
                    printAll(ht);
                    printf("Load function called: %04x, %d\n", function, ifError);
                    break;
                case 0x0310 : //clear
                    for(size_t j=pointer; j<pointer+4; j++){ //for testing
                        magicN = magicN << 8 | recvBuf[j];
                    }                        
                    pointer += 4;
                    if(magicN == 0x0badbad0){
                        freeTable(ht);
                        printf("clear\n");
                    }else{
                        ifError = 22;
                    }
                    sendBuf[sPointer] = ifError; 
                    sendSize = 5;
                    printAll(ht);
                    printf("Clear function called: %04x\n", function);
                default :
                    break;
            }

            f1 = recvBuf[0];
            f2 = recvBuf[1] & 0x0f;
            function = (uint16_t)f1 << 8 | f2; //record the function called flag

            switch(function){ //goto different function call with flag
                case 0x0101 : //add with flag
                    if(done==1){break;}
                    result = mathFun(recvBuf, '+', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printAll(ht);
                    printf("Add function called: %04x, get %016lx, error %02x\n", function, result, ifError);
                    break;
                case 0x0102 : //sub with flag
                    if(done==1){break;}
                    result = mathFun(recvBuf, '-', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printAll(ht);
                    printf("Sub function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0103 : //mul with flag
                    if(done==1){break;}
                    result = mathFun(recvBuf, '*', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printAll(ht);
                    printf("Mul function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0104 : //div with flag
                    if(done==1){break;}
                    result = mathFun(recvBuf, '/', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printAll(ht);
                    printf("Div function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0105 : //mod with flag
                    if(done==1){break;}
                    result = mathFun(recvBuf, '%', &ifError, &pointer, ht, I, cl, recvPos, cmdLength, d);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printAll(ht);
                    printf("Mod function called: %04x, get %016lx\n", function, result);
                    break;
                default:
                    break;
            }

            
            if(sendBuf[sPointer] == 0 && function != 0x0210 && function != 0x0201 && function != 0x0108){ //if no error and function is not create or read write the result to the send buffer
                sPointer += 1;
                for(size_t k=1; k<=sizeof(result); k++){ //store the result to the send buffer
                    sendBuf[k+sPointer-1] = result >> 8*(sizeof(result) - k) & 0xff;
                }
                sPointer += sizeof(result);
            }
            if(sendBuf[sPointer] == 0  && function == 0x0201 && result < 65536){ //if read function called
                sPointer += 1;
                for(size_t k=1; k<=2; k++){ //store the buffer length to the send buffer
                    sendBuf[k+sPointer-1] = (uint16_t) result >> 8*(2 - k) & 0xff;
                }
                sPointer += 2;

                sPointer += result;
            }else if(sendBuf[sPointer] == 0  && function == 0x0108 && result < 65536){  //if getv function called
                sPointer += 1;
                sendBuf[sPointer] = (uint8_t) result & 0xff;
                sPointer += 1;
            }else if(sendBuf[sPointer] == 0  && function == 0x0201 && result > 65536){ //if write function called and alread sent in function
                alreadySent = 1;
            }

            for(size_t i=0; i<sendSize; i++){ //for testing print what sent
                    printf("%02x", sendBuf[i]);
            }
            printf("\nBytes sent: %zu\n", sendSize);

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
                recvSize = read(cl, recvPos, 65536); //read from client again
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
            dumpDir(ht, d); //dump to directory 
        }while(cmdLength > 0 && cmdLength < 65536); //if there's more from the client repeat

        alreadySent = 0;
        cmdLength = 0;  //reset variables
        memset(recvBuf, 0, sizeof(recvBuf));
        recvPos = recvBuf;
        pointer = 0;
        sPointer = 0;
        timer = 0;
        ifError = 0;
        done = 0;
    }
    else if(cmdLength==0){ //for testing
        printf("Connection closed\n");
    }
    else{ //for testing
        printf("Recv failed\n");
        return;
    }
    return;
}

//set new thread to wait and do process after signaled
void* start(void* arg) {
    Thread* thread = (Thread*)arg;
    while (true) {
        printf("Thread%d waiting\n", thread->id);
        if (0 != sem_wait(&thread->mutex)) err(2,"sem_wait in thread");
        process(thread->cl, thread->ht, thread->I, thread->d);
        thread->cl = 0;
        if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
    }
    return 0;
}

//return the available thread index
int findWorker(Thread* tl, int N){ 
    for(int i=0; i<N; i++){
        Thread* t = tl+i;
        printf("thread # %d\n", t->id);
        if(t->cl == 0){ //return thread # in the list if cl = 0
            return i;
        }
    }
    return -1;
}

int main(int argc, char* argv[]){
    int opt; //get H, N, I, and d from argv
    int N = 4;
    int H = 32;
    char* d = "data";
    int I = 50;
    while((opt = getopt(argc,argv, "H:N:I:d: ")) != -1){
        switch(opt){
            case 'N':
                N = atoi(optarg);
                break;
            case 'H':
                H = atoi(optarg);
                break;
            case 'd':
                d = optarg;
                break;
            case 'I':
                I = atoi(optarg);
                break;
        }
    }
    printf("%d, %d, %d, %s\n", H, N, I, d);

    char* port; //get hostname and port number from argv
    char* hostname;
    const char* s = ":";
    hostname = strtok(argv[optind], s);
    port = strtok(NULL, s);

    Thread threads[N];    
    HashTable* ht = createTable(H);
    int availableT = 0;
    if(strcmp(d,"data") == 0){
    }else{
        if(loadDir(ht, d)<0){
            return(EXIT_FAILURE);
        }
    }

    //cited from https://piazza.com/class/kex6x7ets2p35c?cid=291 
    if (0 != sem_init(&mainMutex, 0, 0)) err(2,"sem_init mainMutex");
    pthread_t threadPointer;

    // initialize and start the threads
    for (int i = 0; i < N; i++) {
        Thread* thread = threads+i;
        if (0 != sem_init(&(thread->mutex), 0, 0)) err(2,"sem_init for thread");
        thread->id = i;
        thread->cl = 0;
        thread->I = 50;
        thread->d = d;
        thread->ht = ht;
        if (0 != pthread_create(&threadPointer,0,start,thread)) err(2,"pthread_create"); //pass both thread and hash table to start function
    }

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
    
    do{
        int cl = accept(sock, NULL, NULL);
        availableT = findWorker(threads,N); //find available thread
        printf("availableT: %d\n", availableT);
        while(availableT == -1){ // wait until find one
            if (0 != sem_wait(&mainMutex)) err(2,"sem_wait of main by thread");
            availableT = findWorker(threads,N);
        } 
        Thread* thread = threads+availableT;
        thread->cl = cl; //set thread cl to cl
        thread->I = I;
        if (0 != sem_post(&(thread->mutex))) err(2,"sem_post for thread"); // signal the thread to start process

        dumpDir(ht, d);
    }while(true);

    dumpDir(ht, d);
    return(EXIT_SUCCESS);
}