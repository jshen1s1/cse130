#include "hashTable.h"
#include "functions.h"
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


//check if multiply overflows
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

int64_t mathFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer, HashTable* t, int It, int cl, uint8_t* recvPos, size_t cmdLength, char* d){ //contains add, sub, mul, div, mod 
    uint8_t oprand; //number of oprands
    uint16_t nameSize1 = 0; //varible a
    uint16_t nameSize2 = 0; //varible b
    uint16_t nameSize3 = 0; //varible c
    char name1[32];
    char name2[32];
    char name3[32];
    char* key1;
    char* key2;
    char* key3;
    int64_t a = 0; //finial a
    int64_t b = 0; //finial b
    size_t requiredLen = 7; //dynamic length checker
    size_t recvSize; //size read each time
    int timer = 0;

    oprand = buffer[1] & 0xf0;
    if(buffer[1] == 0x0f){ //if del is called
        nameSize1 = buffer[*pointer];
        *pointer += 1;
        for(size_t i=0; i<nameSize1; i++){ //retrive varName
            if(i>30){ //check length
                *ifError = 22;
                return 0;
            }
            name1[i] = (char) buffer[i+*pointer];
            if(i==0){ //if name vaild
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
        if(del(t,key1) < 0){ //remove from hashtable
            dumpDir(t, d);
            *ifError = 2;
            return 0;
        }
        dumpDir(t, d);
        return 0;
    }

    switch(oprand){
        case 0x10: //a is oprand
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
            a = lookUp(t, key1); //get a value from hashtable 
            if(a == 1234567890){ //no such varible
                *ifError = 2;
                return 0;
            }
            if(a == 9999){ //value is a variable name
                *ifError = 14;
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
        case 0x20: //b is oprand
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
            b = lookUp(t, key2);
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            }        
            if(b == 9999){
                *ifError = 14;
                return 0;
            }
            break;
        case 0x30: //a b both oprand
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
            a = lookUp(t, key1);
            if(a == 1234567890){
                *ifError = 2;
                return 0;
            }
            if(a == 9999){
                *ifError = 14;
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
            b = lookUp(t, key2);
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            }       
            if(b == 9999){
                *ifError = 14;
                return 0;
            }
            break;
        case 0x40: //result is oprand
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
        case 0x50: //a result oprand
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
            a = lookUp(t, key1);
            if(a == 1234567890){
                *ifError = 2;
                return 0;
            }
            if(a == 9999){
                *ifError = 14;
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
        case 0x60: //b result oprand
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
            b = lookUp(t, key2);
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            }
            if(b == 9999){
                *ifError = 14;
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
        case 0x70: //a b result oprand
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
            a = lookUp(t, key1);
            if(a == 1234567890){
                *ifError = 2;
                return 0;
            }
            if(a == 9999){
                *ifError = 14;
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
            b = lookUp(t, key2);
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            } 
            if(b == 9999){
                *ifError = 14;
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
        case 0x90: //with flag 0x80, oprand a
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
            if(a == 1234567890){
                *ifError = 2;
                return 0;
            }
            if(a == 9876543210){
                *ifError = 40;
                return 0;
            }

            for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
                b = b << 8 | (int64_t) buffer[j];
            }
            *pointer += 8;
            break;
        case 0xa0: //with flag 0x80, oprand b
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
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            } 
            if(b == 9876543210){
                *ifError = 40;
                return 0;
            }

            break;
        case 0xb0: //with flag 0x80, oprand a b
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
            if(a == 1234567890){
                *ifError = 2;
                return 0;
            }
            if(a == 9876543210){
                *ifError = 40;
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
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            } 
            if(b == 9876543210){
                *ifError = 40;
                return 0;
            }
            break;
        case 0xc0: //with flag 0x80, oprand result
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
        case 0xd0: //with flag 0x80, oprand a result
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
            if(a == 1234567890){
                *ifError = 2;
                return 0;
            }
            if(a == 9876543210){
                *ifError = 40;
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
        case 0xe0: //with flag 0x80, oprand b result
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
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            } 
            if(b == 9876543210){
                *ifError = 40;
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
        case 0xf0: //with flag 0x80, oprand a b result
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
            if(a == 1234567890){
                *ifError = 2;
                return 0;
            }
            if(a == 9876543210){
                *ifError = 40;
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
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            } 
            if(b == 9876543210){
                *ifError = 40;
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
        default: //no oprand
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
    if(op == '+' && nameSize3 == 0){ //add opration without insert
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
    else if(op == '+' && nameSize3 != 0){ //add opration with insert
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
        dumpDir(t, d);
        return a+b;
    }
    else if(op == '-' && nameSize3 != 0){ //sub opration with insert
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
        dumpDir(t, d);
        return a-b;
    }
    else if(op == '-' && nameSize3 == 0){ //sub opration without insert
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
    else if(op == '*' && nameSize3 == 0){ //mu; opration without insert
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
        
        return a*b;
    }
    else if(op == '*' && nameSize3 != 0){ //mul opration with insert
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
        dumpDir(t, d);
        return a*b;
    }
    else if(op == '/' && nameSize3 == 0){ //div opration without insert
        if(b == 0){
            *ifError = 22;
            return 0; 
        }
        if(a == 0){ 
            return 0;
        }
        
        return a/b;
    }
    else if(op == '/' && nameSize3 != 0){ //add opration with insert
        if(b == 0){
            *ifError = 22;
            return 0; 
        }
        if(a == 0){ 
            insert(t, key3, 0);
            return 0;
        }

        insert(t, key3, a/b);
        dumpDir(t, d);
        return a/b;
    }
    else if(op == '%' && nameSize3 == 0){ //mod opration without insert
        if(b == 0){
            *ifError = 22;
            return 0; 
        }
        if(a == 0){ 
            return 0;
        }
        
        return a%b;
    }
    else if(op == '%' && nameSize3 != 0){ //mod opration with insert
        if(b == 0){
            *ifError = 22;
            return 0; 
        }
        if(a == 0){ 
            insert(t, key3, 0);
            dumpDir(t, d);
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
    size_t pointer = 8; //pointer to recvBuffer
    char* fileName; //file destination
    uint64_t offset;
    uint16_t bufsize;
    char fileBuf[8192]; //write buffer
    uint16_t fileNameSize;
    int64_t fd;
    int64_t res = 0; //similar as cmdLength
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

    if(bufsize>4075 && op == 'w'){ //read more if write opration and input size is large
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
    if(op == 'r' && (st.st_size < (int64_t) offset || st.st_size < (int64_t) (bufsize + offset))){ //check if read or offset over file size
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
            if(buffer[pointer] == 0x00){
                fileBuf[k] = ' ';
            }else{
                fileBuf[k] = (char) buffer[pointer];
            }
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
        if(open(fileName, O_EXCL|O_CREAT, 0600 ) < 0){ //create the file 
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
        *ifError = dump(ht, fileName); //dump to fild
        return 0;
    }else if(op == 'l'){
        if(load(ht, fileName) < 0){ //load from file
            *ifError = 22;
            return -1;
        }
    }
    return 0;
}

uint8_t getv(uint8_t buffer[], uint8_t *ifError, size_t *pointer, HashTable *ht, uint8_t *sendBuf){
    uint16_t nameSize1 = 0; //variable a
    uint16_t nameSize2 = 0; //variable b
    char name1[32];
    char name2[32];
    char* key1;
    char* d;

    nameSize1 = buffer[*pointer];
    *pointer += 1;
    for(size_t i=0; i<nameSize1; i++){ //retrive varName
        if(i>30){ //check key length
            *ifError = 22;
            return 0;
        }
        name1[i] = (char) buffer[i+*pointer];
        if(i==0){ //if key valid
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

    d = lookUpVN(ht, key1); //search for key-key
    if(strcmp(d, "NULL") == 0){ //key not found 
        *ifError = 2;
        return 0;
    }else if(strcmp(d, "NUMBER") == 0){ //number found
        *ifError = 14;
        return 0;
    }else{
        nameSize2 = strlen(d);
        strcpy(name2, d);
        for(size_t i=0; i<nameSize2; i++){  //store to send buffer
            sendBuf[6+i] = name2[i];
        }

        return nameSize2;
    }

    return 0;
}

int64_t setv(uint8_t buffer[], uint8_t *ifError, size_t *pointer, HashTable *ht, char* d){
    uint16_t nameSize1 = 0; //variable a
    uint16_t nameSize2 = 0; //variable b
    char name1[32];
    char name2[32];
    char* key1;
    char* key2;

    nameSize1 = buffer[*pointer];
    *pointer += 1;
    for(size_t i=0; i<nameSize1; i++){ //retrive varName a
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
    for(size_t i=0; i<nameSize2; i++){ //retrive varName b
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

    insert2(ht, key1, key2); //insert key-key
    dumpDir(ht, d);
    return 0;
}

