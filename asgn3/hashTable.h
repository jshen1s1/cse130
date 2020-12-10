#ifndef hashTable_h
#define hashTable_h
#include <stdio.h>
#include <stdint.h>

typedef struct Ht_item{ // stores a variable name and a value and next item
    char* key;
    int64_t value;
    char* value2;
    uint8_t flag;
    Ht_item* next;
}Ht_item;

typedef struct HashTable{ //hash table with a hash item array and size of the array in it
    size_t size;
    Ht_item** items;
    size_t count;
}HashTable;

union data{
    int64_t v;
    char n[32];
    uint8_t flag;
};

unsigned int DJBHash(char* str, unsigned int length);

Ht_item* createItem(char* n, int64_t v);

Ht_item* createItem2(char* n, char* v);

HashTable* createTable(int s);

void freeItem(Ht_item* i);

void freeTable(HashTable* t);

void insert(HashTable* t, char* k, int64_t v);

void insert2(HashTable* t, char* k, char* v);

void replacement(HashTable* t, char* k, int64_t v);

int del(HashTable* t, char* k);

data lookUp(HashTable* t, char* k);

void printAll(HashTable* t);

int dump(HashTable* t, char* fileName);

int load(HashTable* t, char* fileName);

#endif