#include "hashTable.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

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

//create item start with an int value
Ht_item* createItem(char* n, int64_t v){
    Ht_item* item = (Ht_item *)malloc(sizeof(Ht_item));
    item->key = (char*) malloc (strlen(n) + 1);
    strcpy(item->key, n);
    item->value = v;
    item->next = NULL;
    item->flag = 0;
    return item;
}

//create item start with variable name value
Ht_item* createItem2(char* n, char* v){
    Ht_item* item = (Ht_item *)malloc(sizeof(Ht_item));
    item->key = (char*) malloc (strlen(n) + 1);
    strcpy(item->key, n);
    item->value2 = (char*) malloc (strlen(v) + 1);
    strcpy(item->value2, v);
    item->next = NULL;
    item->flag = 1;
    return item;
}

//create table with size s
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

//use to free items
void freeItem(Ht_item* i){
    free(i->key);
    free(i->next);
    free(i->value2);
    free(i);
}

//remove all items from table
void freeTable(HashTable* t){
    for(size_t i=0; i<t->size; i++){
        Ht_item* item = t->items[i];
        if(item != NULL){
            t->items[i] = NULL;
        }
    }
}

//insert a new number item to hashtable
void insert(HashTable* t, char* k, int64_t v){
    Ht_item* item = createItem(k, v);
    unsigned int index = DJBHash(k, strlen(k)) % t->size; //get the index
    Ht_item* curr = t->items[index];
    if(curr == NULL){ //if no item in current index
        if(t->count >= t->size){
            printf("Insert Error: Hash Table is full\n");
            return;
        }
        t->items[index] = item;
        t->count += 1;
    }else{
        if(strcmp(curr->key, k) == 0){ //if the current item has the same key as input
            if(curr->flag == 1){ //turn the flag is it was a variable name
                curr->flag = 0;
            }
            curr->value = v; //update the value
            return;
        }else{
            while(curr->next != NULL){ //go through the linked list
                curr = curr->next;
                if(strcmp(curr->key, k) == 0){ //if the current item has the same key as input do an update
                    if(curr->flag == 1){ 
                        curr->flag = 0;
                    }
                    curr->value = v;
                    return;
                }
            }
            curr->next = item;
        }
    }
    return;
}

//insert a new variable name item to hashtable
void insert2(HashTable* t, char* k, char* v){ //do the same process as insert1
    Ht_item* item = createItem2(k, v); 
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    Ht_item* curr = t->items[index];
    if(curr == NULL){
        if(t->count >= t->size){
            printf("Insert Error: Hash Table is full\n");
            return;
        }
        t->items[index] = item;
        t->count += 1;
    }else{
        if(strcmp(curr->key, k) == 0){
            if(curr->flag == 0){ //if it was a number, turn the flag
                curr->flag = 1;
            }
            curr->value2 = (char*) malloc (strlen(v) + 1); //malloc size to the variable name
            strcpy(curr->value2, v);
            return;
        }else{
            while(curr->next != NULL){
                curr = curr->next;
                if(strcmp(curr->key, k) == 0){
                    if(curr->flag == 0){
                        curr->flag = 1;
                    }
                    curr->value2 = (char*) malloc (strlen(v) + 1);
                    strcpy(curr->value2, v);
                    return;
                }
            }
            curr->next = item;
        }
    }
    return;
}

//update an item in hashtable
void replacement(HashTable* t, char* k, int64_t v){
    unsigned int index = DJBHash(k, strlen(k)) % t->size; //goto the index
    Ht_item* curr = t->items[index]; 
    if(curr == NULL){
        return;
    }else{
        if(strcmp(curr->key, k) == 0){ //if the current item has the same key as input do an update
            curr->value = v;
            return;
        }else{
            while(curr->next != NULL){ //go through linked list
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

//remove an item from hashtable
int del(HashTable* t, char* k){
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    Ht_item* curr = t->items[index];
    if(curr == NULL){ //find the item in hashtable
        return -1;
    }else{
        if(curr->next == NULL && strcmp(curr->key, k) == 0){ //remove it if found
            //freeItem(curr);
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
            while(curr->next != NULL){ //repeat the process if there's a linked list
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

//find a number item from hashtable
int64_t lookUp(HashTable* t, char* k){
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    int64_t res = 1234567890;
    Ht_item* curr = t->items[index];
    while(curr != NULL){
        if(strcmp(curr->key, k) == 0){
            if(curr->flag == 0){ //if the item is a number item 
                res = (int64_t) curr->value; //return value
                return res;
            }else{ //if the item is a variable name item
                res = 9999; //return number indicates wrong type 
                return res;
            }  
        }
        if(curr-> next == NULL){
            return res;
        }
        curr = curr->next;
    }
    return res;
}

//find a variable name item from hashtable
char* lookUpVN(HashTable* t, char* k){
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    char* res = (char*) malloc (31);
    strcpy(res, "NULL"); //set the return value to NULL
    Ht_item* curr = t->items[index];
    while(curr != NULL){
        if(strcmp(curr->key, k) == 0){
            if(curr->flag == 0){ //if the item is a number item 
                strcpy(res, "NUMBER"); //return string indicates wrong type 
                return res;
            }else{ //if the item is a variable name item
                strcpy(res, curr->value2); //return variable name
                return res;
            }
        }
        if(curr-> next == NULL){
            return res;
        }
        curr = curr->next;
    }
    return res;
}

//find a number item from hashtable recursively
int64_t lookUpR(HashTable* t, char* k, int times, int end){
    if(times>=end){ //if reached the maximum return error
        return 9876543210;
    }
    times += 1; //every iteration +1
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    Ht_item* curr = t->items[index];
    while(curr != NULL){
        if(strcmp(curr->key, k) == 0){
            if(curr->flag == 0){ //if value found return value
                return curr->value;
            }else{
                return lookUpR(t,curr->value2, times, end); //if variable name found, do a recursion with the variable name
            }
        }
        if(curr-> next == NULL){
            return 1234567890; //return if nothing found
        }
        curr = curr->next;
    }
    return 1234567890;
}

//print out all items in hashtable. For testing.
void printAll(HashTable* t){
    for(size_t i=0; i<t->size; i++){
        Ht_item* curr = t->items[i];
        if(curr != NULL){
            if(curr->flag == 0){
                printf("%s=%ld\n", curr->key, curr->value);
            }else{
                printf("%s=%s\n", curr->key, curr->value2);
            }
            if(curr->next != NULL){
                curr = curr->next;
                while(curr != NULL){
                    if(curr->flag == 0){
                        printf("%s=%ld\n", curr->key, curr->value);
                    }else{
                        printf("%s=%s\n", curr->key, curr->value2);
                    }
                    curr = curr->next;
                }
            }
        }
    }
    return;
}

//print out all items in hashtable to the destination file.
int dump(HashTable* t, char* fileName){
    int64_t fd = open(fileName, O_RDWR | O_CREAT); //open or create the file to dump
    if(fd < 0){
        printf("No such file\n"); 
        return errno;
    }

    for(size_t i=0; i<t->size; i++){
        Ht_item* curr = t->items[i];
        if(curr != NULL){
            if(curr->flag == 0){ //if the item is a number item print as k=v
                dprintf(fd, "%s=%ld\n", curr->key, curr->value);
            }else{ //if the item is a variable name item print as k=k
                dprintf(fd, "%s=%s\n", curr->key, curr->value2);
            }
            if(curr->next != NULL){
                curr = curr->next;
                while(curr != NULL){
                    if(curr->flag == 0){
                        dprintf(fd, "%s=%ld\n", curr->key, curr->value);
                    }else{
                        dprintf(fd, "%s=%s\n", curr->key, curr->value2);
                    }
                    curr = curr->next;
                }
            }
        }
    }
    close(fd);
    return 0;
}

//print out all items in hashtable to the destination file in assigned directory. 
int dumpDir(HashTable* t, char* d){
    int64_t dir_fd = open (d, O_DIRECTORY | O_PATH); //open the directory
    if(dir_fd < 0){
        printf("No such file directory\n"); 
        return errno;
    }
    int64_t fd = openat (dir_fd, "log", O_CREAT | O_RDWR, 0644); //open or create the file to dump
    if(fd < 0){
        printf("No such file\n"); 
        return errno;
    }

    for(size_t i=0; i<t->size; i++){ //same process as dump
        Ht_item* curr = t->items[i];
        if(curr != NULL){
            if(curr->flag == 0){
                dprintf(fd, "%s=%ld\n", curr->key, curr->value);
            }else{
                dprintf(fd, "%s=%s\n", curr->key, curr->value2);
            }
            if(curr->next != NULL){
                curr = curr->next;
                while(curr != NULL){
                    if(curr->flag == 0){
                        dprintf(fd, "%s=%ld\n", curr->key, curr->value);
                    }else{
                        dprintf(fd, "%s=%s\n", curr->key, curr->value2);
                    }
                    curr = curr->next;
                }
            }
        }
    }
    close(fd);
    return 0;
}

//store keys and values from a file to hashtable
int load(HashTable* t, char* fileName){
    char str[5000]; //store what's read
    char* k; //key
    char* k2; //variable name 
    char varName[32]; //checker
    int64_t v; //value
    int64_t fd = open(fileName, O_RDWR);
    const char* s = "=";

    if(fd < 0){
        printf("No such file\n");   
        return errno;
    }
    FILE* fp = fdopen(fd, "a+");
    if(fp){
        while(fscanf(fp, "%s", str) != EOF){
            k = strtok(str, s); //use strtok to get char* before '='
            k2 = strtok(NULL, s); //use strtok to get char* after '='
            strcpy(varName, k); 
            for(size_t i=0; i<strlen(k); i++){ //check if malformed
                if(i>30){
                    return -1;
                }
                if(i==0){
                    if(varName[0]<65 || (varName[0]>90 && varName[0]<97) || varName[0]>122){
                        return -1;
                    }
                }else{
                    if(varName[i]<48 || (varName[i]>57 && varName[i]<65) || (varName[i]>90 && varName[i]<95) || varName[i] == 96 || varName[i]>122){
                        return -1;
                    }
                }
            }
            if(atoi(k2) != 0){ //check if a number or variable name
                v = (int64_t) atoi(k2); //get the value
                insert(t, k, v);
            }else{
                insert2(t, k, k2);
            }
        }
    }else{
        printf("Error\n");   
        return errno;
    }

    close(fd);
    fclose(fp);
    return 0;
}

//store keys and values from a file in assigned directory to hashtable
int loadDir(HashTable* t, char* d){ // same process as load
    char str[5000]; 
    char* k;
    char* k2;
    char varName[32];
    int64_t v;
    int64_t dir_fd = open (d, O_DIRECTORY | O_PATH); //goto the directory
    const char* s = "=";

    if(dir_fd < 0){
        printf("No such file directory\n");   
        return -1;
    }
    int log_fd = openat (dir_fd, "log", O_CREAT | O_RDWR, 0644); //create the file if not found
    if(log_fd < 0){
        printf("No such file \n");   
        return -1;
    }
    FILE* fp = fdopen(log_fd, "a+");
    if(fp){
        while(fscanf(fp, "%s", str) != EOF){
            k = strtok(str, s);
            k2 = strtok(NULL, s);
            strcpy(varName, k);
            for(size_t i=0; i<strlen(k); i++){
                if(i>30){
                    return -1;
                }
                if(i==0){
                    if(varName[0]<65 || (varName[0]>90 && varName[0]<97) || varName[0]>122){
                        return -1;
                    }
                }else{
                    if(varName[i]<48 || (varName[i]>57 && varName[i]<65) || (varName[i]>90 && varName[i]<95) || varName[i] == 96 || varName[i]>122){
                        return -1;
                    }
                }
            }
            if(atoi(k2) != 0){
                v = (int64_t) atoi(k2);
                insert(t, k, v);
            }else{
                insert2(t, k, k2);
            }
        }
    }else{
        printf("Error\n");   
        return -1;
    }

    close(log_fd);
    fclose(fp);
    return 0;
}

