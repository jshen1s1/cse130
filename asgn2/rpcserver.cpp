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

struct Thread {
    sem_t mutex;
    int cl;
};
typedef struct Thread Thread;

sem_t mainMutex;

//DJBHash cited from https://www.programmingalgorithms.com/algorithm/djb-hash/c/
unsigned int DJBHash(char* str, unsigned int length) { 
	unsigned int hash = 5381;
	unsigned int i = 0;

	for (i = 0; i < length; str++, i++)
	{
		hash = ((hash << 5) + hash) + (*str);
	}

	return hash;
}

typedef struct Ht_item Ht_item;

struct Ht_item
{
    char* key;
    int64_t value;
    Ht_item* next;
};

typedef struct HashTable HashTable;

struct HashTable
{
    size_t size;
    Ht_item** items;
    size_t count;
};

Ht_item* createItem(char* n, int64_t v){
    Ht_item* item = (Ht_item *)malloc(sizeof(Ht_item));
    item->key = (char*) malloc (strlen(n) + 1);
    strcpy(item->key, n);
    item->value = v;
    item->next = NULL;
    return item;
}

HashTable* createTable(int s){
    HashTable* table = (HashTable *)malloc(sizeof(HashTable));
    table->size = s;
    table->count = 0;
    table->items = (Ht_item**) calloc (table->size, sizeof(Ht_item*));
    for (size_t i=0; i<table->size; i++){
        table->items[i] = NULL;
    }
    return table;
}

void freeItem(Ht_item* i){
    free(i->key);
    free(i->next);
    free(i);
}

void freeTable(HashTable* t){
    for(size_t i=0; i<t->size; i++){
        Ht_item* item = t->items[i];
        if(item != NULL){
            t->items[i] = NULL;
        }
    }
}

void insert(HashTable* t, char* k, int64_t v){
    Ht_item* item = createItem(k, v);
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    printf("%s=%ld at %d\n", item->key, item->value, index);
    Ht_item* curr = t->items[index];
    if(curr == NULL){
        if(t->count > t->size){
            printf("Insert Error: Hash Table is full\n");
            return;
        }
        t->items[index] = item;
        t->count += 1;
        printf("insert %s\n", item->key);
    }else{
        if(strcmp(curr->key, k) == 0){
            curr->value = v;
            return;
        }else{
            while(curr->next != NULL){
                curr = curr->next;
                if(strcmp(curr->key, k) == 0){
                    curr->value = v;
                    return;
                }
            }
            curr->next = item;
        }
    }
    return;
}

void replacement(HashTable* t, char* k, int64_t v){
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    Ht_item* curr = t->items[index];
    if(curr == NULL){
        return;
    }else{
        if(strcmp(curr->key, k) == 0){
            curr->value = v;
            return;
        }else{
            while(curr->next != NULL){
                curr = curr->next;
                if(strcmp(curr->key, k) == 0){
                    curr->value = v;
                    return;
                }
            }
        }
    }
    return;
}

int del(HashTable* t, char* k){
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    Ht_item* curr = t->items[index];
    if(curr == NULL){
        return -1;
    }else{
        if(curr->next == NULL && strcmp(curr->key, k) == 0){
            t->items[index] = NULL;
            t->count -= 1;
            return 0;
        }else{
            if(strcmp(curr->key,k) == 0){
                t->items[index] = curr->next;
                t->count -= 1;
                return 0;
            }
            Ht_item* prev = NULL;
            while(curr->next != NULL){
                prev = curr;
                curr = curr->next;
                if(strcmp(curr->key, k) == 0){
                    prev->next = curr->next;
                    return 0;
                }
            }
        }
    }
    return -1;
}

int64_t lookUp(HashTable* t, char* k){
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    int64_t errorNumber = 1234567890;
    Ht_item* curr = t->items[index];
    while(curr != NULL){
        if(strcmp(curr->key, k) == 0){
            printf("find %s\n", curr->key);
            return curr->value;
        }
        if(curr-> next == NULL){
            return errorNumber;
        }
        curr = curr->next;
    }
    return errorNumber;
}

void printAll(HashTable* t){
    for(size_t i=0; i<t->size; i++){
        Ht_item* curr = t->items[i];
        if(curr != NULL){
            printf("%s=%ld\n", curr->key, curr->value);
            if(curr->next != NULL){
                curr = curr->next;
                while(curr != NULL){
                    printf("%s=%ld\n", curr->key, curr->value);
                    curr = curr->next;
                }
            }
        }
    }
    return;
}

int dump(HashTable* t, char* fileName){
    int64_t fd = open(fileName, O_RDWR);
    if(fd < 0){
        printf("No such file\n"); 
        return errno;
    }

    for(size_t i=0; i<t->size; i++){
        Ht_item* curr = t->items[i];
        if(curr != NULL){
            dprintf(fd,"%s=%ld\n", curr->key, curr->value);
            if(curr->next != NULL){
                curr = curr->next;
                while(curr != NULL){
                    dprintf(fd,"%s=%ld\n", curr->key, curr->value);
                    curr = curr->next;
                }
            }
        }
    }
    close(fd);
    return 0;
}

int load(HashTable* t, char* fileName){
    char str[5000]; 
    char* k;
    int64_t v;
    int64_t fd = open(fileName, O_RDWR);
    const char* s = "=";

    if(fd < 0){
        printf("No such file\n");   
        return errno;
    }
    FILE* fp = fdopen(fd, "r+");
    if(fp){
        while(fscanf(fp, "%s", str) != EOF){
            printf("%s\n", str);
            k = strtok(str, s);
            v = (int64_t) atoi(strtok(NULL, s));
            insert(t, k, v);
        }
    }else{
        printf("Error\n");   
        return errno;
    }

    close(fd);
    fclose(fp);
    return 0;
}

int64_t mathFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer, HashTable* t){ //contains add, sub, mul, div, mod 
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
    int64_t a;
    int64_t b;
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
            a = lookUp(t, key1);
            if(a == 1234567890){
                *ifError = 2;
                return 0;
            }

            for(size_t j= *pointer; j< *pointer+8; j++){ //retrive next 8 bytes as b
                b = b << 8 | (int64_t) buffer[j];
            }
            *pointer += 8;
            break;
        case 0x20:
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
            b = lookUp(t, key2);  
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            }        
            break;
        case 0x30:
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
            a = lookUp(t, key1);
            if(a == 1234567890){
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
            b = lookUp(t, key2);    
            if(b == 1234567890){
                *ifError = 2;
                return 0;
            }        
            break;
        case 0x40:
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
        case 0x50:
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
            a = lookUp(t, key1);
            if(a == 1234567890){
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
        case 0x60:
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
            b = lookUp(t, key2);   
            if(b == 1234567890){
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
        case 0x70:
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
            a = lookUp(t, key1);
            if(a == 1234567890){
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
            b = lookUp(t, key2);  
            if(b == 1234567890){
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
        if(a == 0 | b == 0){ //overflow test
            return 0;
        }

        if(abs(a) > LONG_MAX / abs(b) && a!=0 && b!=0){
            *ifError = 75;
        }
        else if(a == LONG_MIN && b == -1){
            *ifError = 75;
        }
        else if(b == LONG_MIN && a == -1){
            *ifError = 75;
        }
        else{
            *ifError = 0;
        }
        
        return a*b;
    }
    else if(op == '*' && nameSize3 != 0){
        if(a == 0 | b == 0){ //overflow test
            insert(t, key3, 0);
            return 0;
        }

        if(abs(a) > LONG_MAX / abs(b) && a!=0 && b!=0){
            *ifError = 75;
        }
        else if(a == LONG_MIN && b == -1){
            *ifError = 75;
        }
        else if(b == LONG_MIN && a == -1){
            *ifError = 75;
        }
        else{
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

int64_t fileMod(uint8_t buffer[], char op, uint8_t *ifError, uint8_t *readBuf, uint8_t *sendBuf, int cl){
    size_t pointer = 8;
    char* fileName;
    uint64_t offset;
    uint16_t bufsize;
    uint8_t fileBuf[8192];
    uint16_t fileNameSize;
    int64_t fd;
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
        if(open(fileName, O_CREAT | O_EXCL) < 0){ //create the file 
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


int main(int argc, char* argv[]){
    char* port; //get hostname and port number from argv
    char* hostname;
    const char* s = ":";
    hostname = strtok(argv[1], s);
    port = strtok(NULL, s);

    int opt;
    int N = 4;
    int H = 32;
    while((opt = getopt(argc,argv, "H:N: ")) != -1){
        switch(opt){
            case 'N':
                N = atoi(optarg);
                break;
            case 'H':
                H = atoi(optarg);
                break;
        }
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
    
    uint8_t recvBuf[16384];
    uint8_t sendBuf[16384];
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
    size_t pointer; //tracks the recvBuf position
    size_t sPointer; //tracks the sendBuf position
    size_t sendSize; 
    size_t cmdLength = 0; 
    uint8_t timer = 0;
    uint8_t alreadySent = 0;
    uint32_t magicN;
    
    Thread threads[N];    
    HashTable* ht = createTable(H);

    char name[31] = "t";
    char* key = name;
    char name2[31] = "t2";
    char* key2 = name2;
    insert(ht, key, 1);
    insert(ht, key2, 2);

    
    //if (0 != sem_init(&mainMutex, 0, 0)) err(2,"sem_init mainMutex");
    //pthread_t threadPointer;

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

                f1 = recvBuf[0];
                f2 = recvBuf[1] & 0x0f;
                function = (uint16_t)f1 << 8 | f2; //record the function called
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
                        // while(cmdLength < n){ //read to 22 bytes for math functions
                        //     recvSize = read(cl, recvPos, 16384); //read from client
                        //     cmdLength += recvSize;
                        //     recvPos = cmdLength + recvBuf;
                        //     timer += 1;
                        //     if(timer > 50){
                        //         break;
                        //     }
                        // }
                        //timer = 0;
                        //printf("entered\n");
                        result = mathFun(recvBuf, '+', &ifError, &pointer, ht);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        if(ifError != 0){
                            sendSize = 5;
                        }
                        printAll(ht);
                        printf("Add function called: %04x, get %016lx, error %02x\n", function, result, ifError);
                        break;
                    case 0x0102 :
                        // while(cmdLength < 22){ //read to 22 bytes for math functions
                        //     recvSize = read(cl, recvPos, 16384); //read from client
                        //     cmdLength += recvSize;
                        //     recvPos = cmdLength + recvBuf;
                        //     timer += 1;
                        //     if(timer > 50){
                        //         break;
                        //     }
                        // }
                        // timer = 0;
                        result = mathFun(recvBuf, '-', &ifError, &pointer, ht);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        if(ifError != 0){
                            sendSize = 5;
                        }
                        printAll(ht);
                        printf("Sub function called: %04x, get %016lx\n", function, result);
                        break;
                    case 0x0103 :
                        // while(cmdLength < 22){ //read to 22 bytes for math functions
                        //     recvSize = read(cl, recvPos, 16384); //read from client
                        //     cmdLength += recvSize;
                        //     recvPos = cmdLength + recvBuf;
                        //     timer += 1;
                        //     if(timer > 50){
                        //         break;
                        //     }
                        // }
                        // timer = 0;
                        result = mathFun(recvBuf, '*', &ifError, &pointer, ht);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        if(ifError != 0){
                            sendSize = 5;
                        }
                        printAll(ht);
                        printf("Mul function called: %04x, get %016lx\n", function, result);
                        break;
                    case 0x0104 :
                        // while(cmdLength < 22){ //read to 22 bytes for math functions
                        //     recvSize = read(cl, recvPos, 16384); //read from client
                        //     cmdLength += recvSize;
                        //     recvPos = cmdLength + recvBuf;
                        //     timer += 1;
                        //     if(timer > 50){
                        //         break;
                        //     }
                        // }
                        // timer = 0;
                        result = mathFun(recvBuf, '/', &ifError, &pointer, ht);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        if(ifError != 0){
                            sendSize = 5;
                        }
                        printAll(ht);
                        printf("Div function called: %04x, get %016lx\n", function, result);
                        break;
                    case 0x0105 :
                        // while(cmdLength < 22){ //read to 22 bytes for math functions
                        //     recvSize = read(cl, recvPos, 16384); //read from client
                        //     cmdLength += recvSize;
                        //     recvPos = cmdLength + recvBuf;
                        //     timer += 1;
                        //     if(timer > 50){
                        //         break;
                        //     }
                        // }
                        // timer = 0;
                        result = mathFun(recvBuf, '%', &ifError, &pointer, ht);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        if(ifError != 0){
                            sendSize = 5;
                        }
                        printAll(ht);
                        printf("Mod function called: %04x, get %016lx\n", function, result);
                        break;
                    case 0x010f :
                        // while(cmdLength < 22){ //read to 22 bytes for math functions
                        //     recvSize = read(cl, recvPos, 16384); //read from client
                        //     cmdLength += recvSize;
                        //     recvPos = cmdLength + recvBuf;
                        //     timer += 1;
                        //     if(timer > 50){
                        //         break;
                        //     }
                        // }
                        // timer = 0;
                        result = mathFun(recvBuf, '*', &ifError, &pointer, ht);
                        sendBuf[sPointer] = ifError;
                        sendSize = 5;
                        printAll(ht);
                        printf("Del function called: %04x, get %016lx\n", function, result);
                        break;
                    case 0x0201 :
                        sendSize = 7;
                        result = fileMod(recvBuf, 'r', &ifError, readBuf, sendBuf, cl);
                        sendBuf[sPointer] = ifError;
                        sendSize = 7+result;
                        if(ifError != 0){
                            sendSize = 5;
                        }
                        printf("Read function called: %04x, get: %zu\n", function, result);
                        break;
                    case 0x0202 :
                        sendSize = 5;
                        result = fileMod(recvBuf, 'w', &ifError, readBuf, sendBuf, cl);
                        sendBuf[sPointer] = ifError;
                        sendSize = 5;
                        printf("Write function called: %04x\n", function);
                        break;
                    case 0x0301 :
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
                        result = fileFun(recvBuf, 'd', &ifError, &pointer, ht);
                        sendBuf[sPointer] = ifError;
                        sendSize = 5;
                        printf("Dump function called: %04x\n", function);
                        break;
                    case 0x0302 :
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
                        result = fileFun(recvBuf, 'l', &ifError, &pointer, ht);
                        sendBuf[sPointer] = ifError;
                        sendSize = 5;
                        printf("Load function called: %04x\n", function);
                        break;
                    case 0x0300 :
                        for(size_t j=pointer; j<pointer+4; j++){ //for testing
                            magicN = magicN << 8 | recvBuf[j];
                        }                        
                        pointer += 4;
                        if(magicN == 0x0badbad0){
                            freeTable(ht);
                        }else{
                            ifError = 22;
                        }
                        printAll(ht);
                        sendBuf[sPointer] = ifError; 
                        sendSize = 5;
                        printf("Clear function called: %04x\n", function);
                        break;
                    default:
                        break;
                }

                function = (uint16_t)recvBuf[0] << 8 | recvBuf[1];
                switch(function){
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
                        result = fileFun(recvBuf, 'c', &ifError, &pointer, ht);
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
                        result = fileFun(recvBuf, 's', &ifError, &pointer, ht);
                        sendBuf[sPointer] = ifError;
                        sendSize = 13;
                        if(ifError != 0){
                            sendSize = 5;
                        }
                        printf("Filesize function called: %04x, get %016lx\n", function, result);
                        break;
                    default :
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