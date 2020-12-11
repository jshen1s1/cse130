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

//sample code thread from https://piazza.com/class/kex6x7ets2p35c?cid=291
struct Thread {
    sem_t mutex;
    int cl;
    int I;
};
typedef struct Thread Thread;

sem_t mainMutex;

struct thread_data { //stores a thread and a hash table
    Thread* t;
    HashTable* ht;
};

typedef struct thread_data thread_data;

int detect_mul_overflow(int64_t a, int64_t b, int64_t result){
    if (a == 0 || b == 0){
        return 0;
    }
    a = abs(a);
    b = abs(b);
    if (result > 9223372036854775807 || abs(result/b) != a){
        return 1;
    }
    return 0;
}

int64_t mathFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer, HashTable* t, int It, int cl, uint8_t* recvPos, size_t cmdLength){ //contains add, sub, mul, div, mod 
    uint8_t oprand;
    uint16_t nameSize1 = 0;
    uint16_t nameSize2 = 0;
    uint16_t nameSize3 = 0;
    char name1[32];
    char name2[32];
    char name3[32];
    char* key1;
    char* key2;
    char* key3;
    int64_t a = 0;
    int64_t b = 0;
    size_t requiredLen = 7;
    size_t recvSize;
    int timer = 0;

    oprand = buffer[1] & 0xf0;
    if(buffer[1] == 0x0f){
        nameSize1 = buffer[*pointer];
        *pointer += 1;
        for(size_t i=0; i<nameSize1; i++){ //retrive varName
            if(i>30){
                *ifError = 22;
                return 0;
            }
            name1[i] = (char) buffer[i+*pointer];
            if(i==0){
                if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                    *ifError = 22;
                    return 0;
                }
            }else{
                if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                    *ifError = 22;
                    return 0;
                }
            }
        }
        name1[nameSize1] = (char) '\0';
        *pointer += nameSize1;
        key1 = name1;
        if(del(t,key1) < 0){
            *ifError = 2;
            return 0;
        }
        return 0;
    }

    switch(oprand){
        case 0x10:
            nameSize1 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize1;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize1; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name1[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }
            }
            name1[nameSize1] = (char) '\0';
            *pointer += nameSize1;
            key1 = name1;
            a = lookUp(t, key1).v;
            if(a == 1234567890 || lookUp(t, key1).flag == 3){
                *ifError = 2;
                return 0;
            }

            requiredLen += 8;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 10){
                    break;
                }
            }
            timer = 0;
            for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
                b = b << 8 | (int64_t) buffer[j];
            }
            *pointer += 8;
            break;
        case 0x20:
            requiredLen += 8;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;

            for(size_t i= *pointer; i< *pointer+8; i++){ //retrive first 8 bytes as a
                a = a << 8 | (int64_t) buffer[i];
            }
            *pointer += 8;

            nameSize2 = buffer[*pointer];
            *pointer += 1;
            requiredLen += nameSize2;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize2; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name2[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name2[0]<65 || (name2[0]>90 && name2[0]<97) || name2[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name2[i]<48 || (name2[i]>57 && name2[i]<65) || (name2[i]>90 && name2[i]<95) || name2[i] == 96 || name2[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            *pointer += nameSize2;
            name2[nameSize2] = (char) '\0';
            key2 = name2;
            b = lookUp(t, key2).v;  
            if(b == 1234567890 || lookUp(t, key2).flag == 3){
                *ifError = 2;
                return 0;
            }        
            break;
        case 0x30:
            nameSize1 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize1;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize1; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name1[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }
            }
            *pointer += nameSize1;
            name1[nameSize1] = (char) '\0';
            key1 = name1;
            a = lookUp(t, key1).v;
            if(a == 1234567890 || lookUp(t, key1).flag == 3){
                *ifError = 2;
                return 0;
            }

            nameSize2 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize2;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize2; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name2[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name2[0]<65 || (name2[0]>90 && name2[0]<97) || name2[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name2[i]<48 || (name2[i]>57 && name2[i]<65) || (name2[i]>90 && name2[i]<95) || name2[i] == 96 || name2[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            *pointer += nameSize2;
            name2[nameSize2] = (char) '\0';
            key2 = name2;
            b = lookUp(t, key2).v;    
            if(b == 1234567890 || lookUp(t, key2).flag == 3){
                *ifError = 2;
                return 0;
            }        
            break;
        case 0x40:
            requiredLen += 8;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i= *pointer; i< *pointer+8; i++){ //retrive first 8 bytes as a
                a = a << 8 | (int64_t) buffer[i];
            }
            *pointer += 8;

            requiredLen += 8;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
                b = b << 8 | (int64_t) buffer[j];
            }
            *pointer += 8;

            nameSize3 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize3;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize3; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name3[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name3[0]<65 || (name3[0]>90 && name3[0]<97) || name3[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name3[i]<48 || (name3[i]>57 && name3[i]<65) || (name3[i]>90 && name3[i]<95) || name3[i] == 96 || name3[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            name3[nameSize3] = (char) '\0';
            key3 = name3;
            *pointer += nameSize3;
            break;
        case 0x50:
            nameSize1 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize1;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize1; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name1[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }
            }
            *pointer += nameSize1;
            name1[nameSize1] = (char) '\0';
            key1 = name1;
            a = lookUp(t, key1).v;
            if(a == 1234567890 || lookUp(t, key1).flag == 3){
                *ifError = 2;
                return 0;
            }

            requiredLen += 8;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
                b = b << 8 | (int64_t) buffer[j];
            }
            *pointer += 8;

            nameSize3 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize3;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize3; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name3[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name3[0]<65 || (name3[0]>90 && name3[0]<97) || name3[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name3[i]<48 || (name3[i]>57 && name3[i]<65) || (name3[i]>90 && name3[i]<95) || name3[i] == 96 || name3[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            name3[nameSize3] = (char) '\0';
            key3 = name3;
            *pointer += nameSize3;
            break;
        case 0x60:
            requiredLen += 8;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i= *pointer; i< *pointer+8; i++){ //retrive first 8 bytes as a
                a = a << 8 | (int64_t) buffer[i];
            }
            *pointer += 8;

            nameSize2 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize2;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize2; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name2[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name2[0]<65 || (name2[0]>90 && name2[0]<97) || name2[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name2[i]<48 || (name2[i]>57 && name2[i]<65) || (name2[i]>90 && name2[i]<95) || name2[i] == 96 || name2[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            *pointer += nameSize2;
            name2[nameSize2] = (char) '\0';
            key2 = name2;
            b = lookUp(t, key2).v;   
            if(b == 1234567890 || lookUp(t, key2).flag == 3){
                *ifError = 2;
                return 0;
            }

            nameSize3 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize3;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize3; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name3[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name3[0]<65 || (name3[0]>90 && name3[0]<97) || name3[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name3[i]<48 || (name3[i]>57 && name3[i]<65) || (name3[i]>90 && name3[i]<95) || name3[i] == 96 || name3[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            name3[nameSize3] = (char) '\0';
            key3 = name3;
            *pointer += nameSize3;
            break;
        case 0x70:
            nameSize1 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize1;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize1; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name1[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }
            }
            *pointer += nameSize1;
            name1[nameSize1] = (char) '\0';
            key1 = name1;
            a = lookUp(t, key1).v;
            if(a == 1234567890 || lookUp(t, key1).flag == 3){
                *ifError = 2;
                return 0;
            }

            nameSize2 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize2;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize2; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name2[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name2[0]<65 || (name2[0]>90 && name2[0]<97) || name2[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name2[i]<48 || (name2[i]>57 && name2[i]<65) || (name2[i]>90 && name2[i]<95) || name2[i] == 96 || name2[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            *pointer += nameSize2;
            name2[nameSize2] = (char) '\0';
            key2 = name2;
            b = lookUp(t, key2).v;  
            if(b == 1234567890 || lookUp(t, key2).flag == 3){
                *ifError = 2;
                return 0;
            } 

            nameSize3 = buffer[*pointer];
            *pointer += 1;
            requiredLen = requiredLen + nameSize3;
            while(cmdLength < requiredLen){
                recvSize = read(cl, recvPos, 65536); //read from client
                cmdLength += recvSize;
                recvPos = cmdLength + buffer;
                timer += 1;
                if(timer > 50){
                    break;
                }
            }
            timer = 0;
            for(size_t i=0; i<nameSize3; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name3[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name3[0]<65 || (name3[0]>90 && name3[0]<97) || name3[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name3[i]<48 || (name3[i]>57 && name3[i]<65) || (name3[i]>90 && name3[i]<95) || name3[i] == 96 || name3[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            name3[nameSize3] = (char) '\0';
            key3 = name3;
            *pointer += nameSize3;
            break;
        case 0x90:
            nameSize1 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize1; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name1[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }
            }
            *pointer += nameSize1;
            name1[nameSize1] = (char) '\0';
            key1 = name1;
            a = lookUpR(t, key1, 0, It);
            if(a == 1234567890 || lookUp(t, key1).flag == 3){
                *ifError = 2;
                return 0;
            }

            for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
                b = b << 8 | (int64_t) buffer[j];
            }
            *pointer += 8;
            break;
        case 0xa0:
            for(size_t i= *pointer; i< *pointer+8; i++){ //retrive first 8 bytes as a
                a = a << 8 | (int64_t) buffer[i];
            }
            *pointer += 8;

            nameSize2 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize2; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name2[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name2[0]<65 || (name2[0]>90 && name2[0]<97) || name2[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name2[i]<48 || (name2[i]>57 && name2[i]<65) || (name2[i]>90 && name2[i]<95) || name2[i] == 96 || name2[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            *pointer += nameSize2;
            name2[nameSize2] = (char) '\0';
            key2 = name2;
            b = lookUpR(t, key2, 0, It);  
            if(b == 1234567890 || lookUp(t, key2).flag == 3){
                *ifError = 2;
                return 0;
            } 

            break;
        case 0xb0:
            nameSize1 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize1; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name1[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }
            }
            *pointer += nameSize1;
            name1[nameSize1] = (char) '\0';
            key1 = name1;
            a = lookUpR(t, key1, 0, It);
            if(a == 1234567890 || lookUp(t, key1).flag == 3){
                *ifError = 2;
                return 0;
            }

            nameSize2 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize2; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name2[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name2[0]<65 || (name2[0]>90 && name2[0]<97) || name2[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name2[i]<48 || (name2[i]>57 && name2[i]<65) || (name2[i]>90 && name2[i]<95) || name2[i] == 96 || name2[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            *pointer += nameSize2;
            name2[nameSize2] = (char) '\0';
            key2 = name2;
            b = lookUpR(t, key2, 0, It);  
            if(b == 1234567890 || lookUp(t, key2).flag == 3){
                *ifError = 2;
                return 0;
            } 

            break;
        case 0xc0:
            for(size_t i= *pointer; i< *pointer+8; i++){ //retrive first 8 bytes as a
                a = a << 8 | (int64_t) buffer[i];
            }
            *pointer += 8;

            for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
                b = b << 8 | (int64_t) buffer[j];
            }
            *pointer += 8;

            nameSize3 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize3; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name3[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name3[0]<65 || (name3[0]>90 && name3[0]<97) || name3[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name3[i]<48 || (name3[i]>57 && name3[i]<65) || (name3[i]>90 && name3[i]<95) || name3[i] == 96 || name3[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            name3[nameSize3] = (char) '\0';
            key3 = name3;
            *pointer += nameSize3;
            break;
        case 0xd0:
            nameSize1 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize1; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name1[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }
            }
            *pointer += nameSize1;
            name1[nameSize1] = (char) '\0';
            key1 = name1;
            a = lookUpR(t, key1, 0, It);
            if(a == 1234567890 || lookUp(t, key1).flag == 3){
                *ifError = 2;
                return 0;
            }

            for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
                b = b << 8 | (int64_t) buffer[j];
            }
            *pointer += 8;

            nameSize3 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize3; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name3[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name3[0]<65 || (name3[0]>90 && name3[0]<97) || name3[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name3[i]<48 || (name3[i]>57 && name3[i]<65) || (name3[i]>90 && name3[i]<95) || name3[i] == 96 || name3[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            name3[nameSize3] = (char) '\0';
            key3 = name3;
            *pointer += nameSize3;
            break;
        case 0xe0:
            for(size_t i= *pointer; i< *pointer+8; i++){ //retrive first 8 bytes as a
                a = a << 8 | (int64_t) buffer[i];
            }
            *pointer += 8;

            nameSize2 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize2; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name2[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name2[0]<65 || (name2[0]>90 && name2[0]<97) || name2[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name2[i]<48 || (name2[i]>57 && name2[i]<65) || (name2[i]>90 && name2[i]<95) || name2[i] == 96 || name2[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            *pointer += nameSize2;
            name2[nameSize2] = (char) '\0';
            key2 = name2;
            b = lookUpR(t, key2, 0, It);  
            if(b == 1234567890 || lookUp(t, key2).flag == 3){
                *ifError = 2;
                return 0;
            } 

            nameSize3 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize3; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name3[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name3[0]<65 || (name3[0]>90 && name3[0]<97) || name3[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name3[i]<48 || (name3[i]>57 && name3[i]<65) || (name3[i]>90 && name3[i]<95) || name3[i] == 96 || name3[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            name3[nameSize3] = (char) '\0';
            key3 = name3;
            *pointer += nameSize3;
            break;
        case 0xf0:
            nameSize1 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize1; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name1[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }
            }
            *pointer += nameSize1;
            name1[nameSize1] = (char) '\0';
            key1 = name1;
            a = lookUpR(t, key1, 0, It);
            if(a == 1234567890 || lookUp(t, key1).flag == 3){
                *ifError = 2;
                return 0;
            }

            nameSize2 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize2; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name2[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name2[0]<65 || (name2[0]>90 && name2[0]<97) || name2[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name2[i]<48 || (name2[i]>57 && name2[i]<65) || (name2[i]>90 && name2[i]<95) || name2[i] == 96 || name2[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            *pointer += nameSize2;
            name2[nameSize2] = (char) '\0';
            key2 = name2;
            b = lookUpR(t, key2, 0, It);  
            if(b == 1234567890 || lookUp(t, key2).flag == 3){
                *ifError = 2;
                return 0;
            } 

            nameSize3 = buffer[*pointer];
            *pointer += 1;
            for(size_t i=0; i<nameSize3; i++){ //retrive varName
                if(i>30){
                    *ifError = 22;
                    return 0;
                }
                name3[i] = (char) buffer[i+*pointer];
                if(i==0){
                    if(name3[0]<65 || (name3[0]>90 && name3[0]<97) || name3[0]>122){
                        *ifError = 22;
                        return 0;
                    }
                }else{
                    if(name3[i]<48 || (name3[i]>57 && name3[i]<65) || (name3[i]>90 && name3[i]<95) || name3[i] == 96 || name3[i]>122){
                        *ifError = 22;
                        return 0;
                    }
                }                
            }
            name3[nameSize3] = (char) '\0';
            key3 = name3;
            *pointer += nameSize3;
            break;
        default:
            for(size_t i= *pointer; i< *pointer+8; i++){ //retrive first 8 bytes as a
                a = a << 8 | (int64_t) buffer[i];
            }
            *pointer += 8;

            for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
                b = b << 8 | (int64_t) buffer[j];
            }
            *pointer += 8;
            break;
    }

    printf("a: %ld b: %ld\n", a, b);
    if(op == '+' && nameSize3 == 0){ 
        if(a > (LONG_MAX - b) && b > 0){ //overflow test
            *ifError = 75; //set ifError to 75 if overflow
        }
        else if(a < (LONG_MIN - b) && b < 0){
            *ifError = 75;
        }
        else{
            *ifError = 0;
        }
        return a+b;
    }
    else if(op == '+' && nameSize3 != 0){
        if(a > (LONG_MAX - b) && b > 0){ //overflow test
            *ifError = 75; //set ifError to 75 if overflow
        }
        else if(a < (LONG_MIN - b) && b < 0){
            *ifError = 75;
        }
        else{
            *ifError = 0;
        }
        insert(t, key3, a+b);
        return a+b;
    }
    else if(op == '-' && nameSize3 != 0){
        if(a > (LONG_MAX + b) && b < 0){ //overflow test
            *ifError = 75;
        }
        else if(a < (LONG_MIN + b) && b > 0){
            *ifError = 75;
        }
        else{
            *ifError = 0;
        }
        insert(t, key3, a-b);
        return a-b;
    }
    else if(op == '-' && nameSize3 == 0){
        if(a > (LONG_MAX + b) && b < 0){ //overflow test
            *ifError = 75;
        }
        else if(a < (LONG_MIN + b) && b > 0){
            *ifError = 75;
        }
        else{
            *ifError = 0;
        }
        return a-b;
    }
    else if(op == '*' && nameSize3 == 0){
        int64_t res = a*b;
        if(detect_mul_overflow(a,b,res) == 1 && b!=-3){
            *ifError = 75;
            return 0;
        }
        else{
            if(a>0 && b>0 && res<0){
                *ifError = 75;
                return 0;
            }
            if(a<0 && b<0 && res<0){
                *ifError = 75;
                return 0;
            }
            if(a<0 && b>0 && res>0){
                *ifError = 75;
                return 0;
            }
            if(a>0 && b<0 && res>0){
                *ifError = 75;
                return 0;
            }
            *ifError = 0;
        }
        
        return a*b;
    }
    else if(op == '*' && nameSize3 != 0){
        int64_t res = a*b;
        if(detect_mul_overflow(a,b,res) == 1){
            *ifError = 75;
            return 0;
        }
        else{
            if(a>0 && b>0 && res<0){
                *ifError = 75;
                return 0;
            }
            if(a<0 && b<0 && res<0){
                *ifError = 75;
                return 0;
            }
            if(a<0 && b>0 && res>0){
                *ifError = 75;
                return 0;
            }
            if(a>0 && b<0 && res>0){
                *ifError = 75;
                return 0;
            }
            *ifError = 0;
        }

        insert(t, key3, a*b);
        return a*b;
    }
    else if(op == '/' && nameSize3 == 0){
        if(b == 0){
            *ifError = 22;
            return 0; 
        }
        if(a == 0){ 
            return 0;
        }
        
        return a/b;
    }
    else if(op == '/' && nameSize3 != 0){
        if(b == 0){
            *ifError = 22;
            return 0; 
        }
        if(a == 0){ 
            insert(t, key3, 0);
            return 0;
        }

        insert(t, key3, a/b);
        return a/b;
    }
    else if(op == '%' && nameSize3 == 0){
        if(b == 0){
            *ifError = 22;
            return 0; 
        }
        if(a == 0){ 
            return 0;
        }
        
        return a%b;
    }
    else if(op == '%' && nameSize3 != 0){
        if(b == 0){
            *ifError = 22;
            return 0; 
        }
        if(a == 0){ 
            insert(t, key3, 0);
            return 0;
        }

        insert(t, key3, a/b);
        return a%b;
    }else{
        *ifError = 95;
        return 0;
    }
    
    return 0;
}

int64_t fileMod(uint8_t buffer[], char op, uint8_t *ifError, uint8_t *readBuf, uint8_t *sendBuf, int cl, uint8_t* recvPos, size_t cmdLength){ //cotains read, write
    size_t pointer = 8;
    char* fileName;
    uint64_t offset;
    uint16_t bufsize;
    char fileBuf[8192];
    uint16_t fileNameSize;
    int64_t fd;
    int64_t res = 0;
    uint8_t timer = 0;
    size_t recvSize;

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

    if(bufsize + offset > pow(2,64)-1){
        *ifError = 22;
        return 0;
    }

    if(bufsize>4075 && op == 'w'){
        do{
            recvSize = read(cl, recvPos, 65536);
            cmdLength += recvSize;
            recvPos = cmdLength + buffer;
            timer += 1;
            if(timer > 50){
                break;
            }
            if(recvSize < 4096){
                break;
            }
        }while(recvSize>0);
        timer = 0;
    }

    fd = open(fileName, O_RDWR); //open file
    if(fd < 0){
        *ifError = 2;
        return 0;
    }
    struct stat st; //use stat to get file size
    if(stat(fileName, &st)<0){
        *ifError = 2; //set ifError to 2 if no such file
    }
    if(op == 'r' && (st.st_size < (int64_t) offset || st.st_size < (int64_t) (bufsize + offset))){
        *ifError = 22;
        return 0;
    }
    if(lseek(fd, offset, SEEK_SET)<0){ //set to offset
        *ifError = errno;
        return 0;
    }

    if(op == 'r'){
        while(read(fd, readBuf, 1) > 0 && res < bufsize){ //read from file and store to read buffer         
            res += 1;
            if(res+7-1 >= 65536){ //if read more than sendBuf can store
                sendBuf[5] = bufsize >> 8 & 0xff;
                sendBuf[6] = bufsize & 0xff;
                write(cl,sendBuf, 65536); //send back current buffer
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
            fileBuf[k] = (char) buffer[pointer];
            pointer += 1;
        }
        printf("%c\n",fileBuf[0]);
        res += write(fd, fileBuf, bufsize); //write to the file
        if(res<0){
            *ifError = errno;
            return 0;
        }
    }

    close(fd);
    return res;
}

int64_t fileFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer, HashTable* ht){ //contains create and filesize
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
        if(open(fileName, O_EXCL|O_CREAT,0600 ) < 0){ //create the file 
            *ifError = 17; //set ifError to 17 if already exist
            return -1;
        }
    }else if(op == 's'){
        struct stat st; //use stat to get file size
		if(stat(fileName, &st)<0){
            *ifError = 2; //set ifError to 2 if no such file
            return -1;
        }
		return st.st_size;
    }else if(op == 'd'){
        *ifError = dump(ht, fileName);
        if(*ifError < 0){
            return -1;
        }
    }else if(op == 'l'){
        *ifError = load(ht, fileName);
        if(*ifError < 0){
            return -1;
        }
    }

    return 0;
}

uint8_t getv(uint8_t buffer[], uint8_t *ifError, size_t *pointer, HashTable *ht, uint8_t *sendBuf){
    uint16_t nameSize1 = 0;
    uint16_t nameSize2 = 0;
    char name1[32];
    char name2[32];
    char* key1;
    union data d;

    nameSize1 = buffer[*pointer];
    *pointer += 1;
    for(size_t i=0; i<nameSize1; i++){ //retrive varName
        if(i>30){
            *ifError = 22;
            return 0;
        }
        name1[i] = (char) buffer[i+*pointer];
        if(i==0){
            if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                *ifError = 22;
                return 0;
            }
        }else{
            if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                *ifError = 22;
                return 0;
            }
        }
    }
    *pointer += nameSize1;
    name1[nameSize1] = (char) '\0';
    key1 = name1;

    d = lookUp(ht, key1);
    if(d.flag == 3){
        *ifError = 2;
        return 0;
    }else if(strlen(d.n) == 0){
        *ifError = 14;
        return 0;
    }else{
        nameSize2 = strlen(d.n);
        strcpy(name2, d.n);
        for(size_t i=0; i<nameSize2; i++){ 
            sendBuf[6+i] = name2[i];
        }

        return nameSize2;
    }

    return 0;
}

int64_t setv(uint8_t buffer[], uint8_t *ifError, size_t *pointer, HashTable *ht){
    uint16_t nameSize1 = 0;
    uint16_t nameSize2 = 0;
    char name1[32];
    char name2[32];
    char* key1;
    char* key2;

    nameSize1 = buffer[*pointer];
    *pointer += 1;
    for(size_t i=0; i<nameSize1; i++){ //retrive varName
        if(i>30){
            *ifError = 22;
            return 0;
        }
        name1[i] = (char) buffer[i+*pointer];
        if(i==0){
            if(name1[0]<65 || (name1[0]>90 && name1[0]<97) || name1[0]>122){
                *ifError = 22;
                return 0;
            }
        }else{
            if(name1[i]<48 || (name1[i]>57 && name1[i]<65) || (name1[i]>90 && name1[i]<95) || name1[i] == 96 || name1[i]>122){
                *ifError = 22;
                return 0;
            }
        }
    }
    *pointer += nameSize1;
    name1[nameSize1] = (char) '\0';
    key1 = name1;
    
    nameSize2 = buffer[*pointer];
    *pointer += 1;
    for(size_t i=0; i<nameSize2; i++){ //retrive varName
        if(i>30){
            *ifError = 22;
            return 0;
        }
        name2[i] = (char) buffer[i+*pointer];
        if(i==0){
            if(name2[0]<65 || (name2[0]>90 && name2[0]<97) || name2[0]>122){
                *ifError = 22;
                return 0;
            }
        }else{
            if(name2[i]<48 || (name2[i]>57 && name2[i]<65) || (name2[i]>90 && name2[i]<95) || name2[i] == 96 || name2[i]>122){
                *ifError = 22;
                return 0;
            }
        }                
    }
    *pointer += nameSize2;
    name2[nameSize2] = (char) '\0';
    key2 = name2;

    insert2(ht, key1, key2);
    return 0;
}


void process(int cl, HashTable* ht, int I) {
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

            switch(function){
                case 0x0101 :
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
                    result = mathFun(recvBuf, '+', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Add function called: %04x, get %016lx, error %02x\n", function, result, ifError);
                    break;
                case 0x0102 :
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
                    result = mathFun(recvBuf, '-', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Sub function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0103 :
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
                    result = mathFun(recvBuf, '*', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Mul function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0104 :
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
                    result = mathFun(recvBuf, '/', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Div function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0105 :
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
                    result = mathFun(recvBuf, '%', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    done = 1;
                    printAll(ht);
                    printf("Mod function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0108 :
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
                case 0x0109 :
                    result = setv(recvBuf, &ifError, &pointer, ht);
                    sendBuf[sPointer] = ifError;
                    sendSize = 5;
                    done = 1;
                    printAll(ht);
                    printf("Setv function called: %04x\n", function);
                    break;
                case 0x010f :
                    result = mathFun(recvBuf, '*', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 5;
                    done = 1;
                    printAll(ht);
                    printf("Del function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0201 :
                    sendSize = 7;
                    result = fileMod(recvBuf, 'r', &ifError, readBuf, sendBuf, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 7+result;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printf("Read function called: %04x, get: %zu\n", function, result);
                    break;
                case 0x0202 :
                    sendSize = 5;
                    result = fileMod(recvBuf, 'w', &ifError, readBuf, sendBuf, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 5;
                    printf("Write function called: %04x\n", function);
                    break;
                case 0x0210 :
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
                case 0x0220 :
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
                case 0x0301 :
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
                case 0x0302 :
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
                case 0x0310 :
                    for(size_t j=pointer; j<pointer+4; j++){ //for testing
                        magicN = magicN << 8 | recvBuf[j];
                    }                        
                    pointer += 4;
                    if(magicN == 0x0badbad0){
                        //freeTable(ht);
                        printf("clear\n");
                    }else{
                        ifError = 22;
                    }
                    sendBuf[sPointer] = ifError; 
                    sendSize = 5;
                    printf("Clear function called: %04x\n", function);
                default :
                    break;
            }

            f1 = recvBuf[0];
            f2 = recvBuf[1] & 0x0f;
            function = (uint16_t)f1 << 8 | f2; //record the function called

            switch(function){ //goto different function call
                case 0x0101 :
                    if(done==1){break;}
                    result = mathFun(recvBuf, '+', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printAll(ht);
                    printf("Add function called: %04x, get %016lx, error %02x\n", function, result, ifError);
                    break;
                case 0x0102 :
                    if(done==1){break;}
                    result = mathFun(recvBuf, '-', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printAll(ht);
                    printf("Sub function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0103 :
                    if(done==1){break;}
                    result = mathFun(recvBuf, '*', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printAll(ht);
                    printf("Mul function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0104 :
                    if(done==1){break;}
                    result = mathFun(recvBuf, '/', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
                    sendBuf[sPointer] = ifError;
                    sendSize = 13;
                    if(ifError != 0){
                        sendSize = 5;
                    }
                    printAll(ht);
                    printf("Div function called: %04x, get %016lx\n", function, result);
                    break;
                case 0x0105 :
                    if(done==1){break;}
                    result = mathFun(recvBuf, '%', &ifError, &pointer, ht, I, cl, recvPos, cmdLength);
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
            }else if(sendBuf[sPointer] == 0  && function == 0x0108 && result < 65536){
                sPointer += 1;
                sendBuf[sPointer] = (uint8_t) result & 0xff;
                sPointer += 1;
            }else if(sendBuf[sPointer] == 0  && function == 0x0201 && result > 65536){
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

void* start(void* arg) {
    thread_data* td = (thread_data*)arg;
    while (true) {
        printf("Thread waiting\n");
        if (0 != sem_wait(&(td->t->mutex))) err(2,"sem_wait in thread");
        process(td->t->cl, td->ht, td->t->I);
        td->t->cl = 0;
        td->t->I = 50;
        if (0 != sem_post(&mainMutex)) err(2,"sem_post of main by thread");
    }
    return 0;
}

int findWorker(Thread* tl, int N){ //return the available thread index
    for(int i=0; i<N; i++){
        Thread* t = tl+i;
        if(t->cl == 0){
            return i;
        }
    }
    return -1;
}

int main(int argc, char* argv[]){
    int opt; //get H and N from argv
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
    thread_data td[N]; //store thread and hash table
    HashTable* ht = createTable(H);
    int availableT = 0;
    if(loadDir(ht, d)<0){
        return(EXIT_FAILURE);
    }

    
    //cited from https://piazza.com/class/kex6x7ets2p35c?cid=291 
    if (0 != sem_init(&mainMutex, 0, 0)) err(2,"sem_init mainMutex");
    pthread_t threadPointer;

    for (int i = 0; i < N; i++) {
        Thread* thread = threads+i;
        if (0 != sem_init(&(thread->mutex), 0, 0)) err(2,"sem_init for thread");
        thread->cl = 0;
        thread->I = 50;
        td[i].t = thread;
        td[i].ht = ht;
        if (0 != pthread_create(&threadPointer,0,start, &td[i])) err(2,"pthread_create"); //pass both thread and hash table to start function
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
        if (0 != sem_post(&(thread->mutex))) err(2,"sem_post in thread"); // signal the thread to start process

        if (0 != sem_wait(&mainMutex)) err(2,"sem_wait in main");

        dumpDir(ht, d);
    }while(true);

    return(EXIT_SUCCESS);
}