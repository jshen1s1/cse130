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

Ht_item* createItem(char* n, int64_t v){
    Ht_item* item = (Ht_item *)malloc(sizeof(Ht_item));
    item->key = (char*) malloc (strlen(n) + 1);
    strcpy(item->key, n);
    item->value = v;
    item->next = NULL;
    item->flag = 0;
    return item;
}

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
    free(i->value2);
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
            if(curr->flag == 1){
                curr->flag = 0;
            }
            curr->value = v;
            return;
        }else{
            while(curr->next != NULL){
                curr = curr->next;
                if(strcmp(curr->key, k) == 0){
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

void insert2(HashTable* t, char* k, char* v){
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
            if(curr->flag == 0){
                curr->flag = 1;
            }
            curr->value2 = (char*) malloc (strlen(v) + 1);
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

data lookUp(HashTable* t, char* k){
    unsigned int index = DJBHash(k, strlen(k)) % t->size;
    union data res;
    res.v = 1234567890;
    res.flag = 0;
    Ht_item* curr = t->items[index];
    while(curr != NULL){
        if(strcmp(curr->key, k) == 0){
            if(curr->flag == 0){
                res.v = curr->value;
                res.flag = 0;
            }else{
                res.flag = 1;
                strcpy(res.n, curr->value2);
            }
            return res;
        }
        if(curr-> next == NULL){
            return res;
        }
        curr = curr->next;
    }
    return res;
}

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

int dump(HashTable* t, char* fileName){
    int64_t fd = open(fileName, O_RDWR | O_CREAT);
    if(fd < 0){
        printf("No such file\n"); 
        return errno;
    }

    for(size_t i=0; i<t->size; i++){
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
