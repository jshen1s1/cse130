#ifndef functions_h
#define functions_h
#include "hashTable.h"

int detect_mul_overflow(int64_t a, int64_t b, int64_t result);

int64_t mathFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer, HashTable* t, int It, int cl, uint8_t* recvPos, size_t cmdLength, char* d);

int64_t fileMod(uint8_t buffer[], char op, uint8_t *ifError, uint8_t *readBuf, uint8_t *sendBuf, int cl, uint8_t* recvPos, size_t cmdLength);

int64_t fileFun(uint8_t buffer[], char op, uint8_t *ifError, size_t *pointer, HashTable* ht);

uint8_t getv(uint8_t buffer[], uint8_t *ifError, size_t *pointer, HashTable *ht, uint8_t *sendBuf);

int64_t setv(uint8_t buffer[], uint8_t *ifError, size_t *pointer, HashTable *ht, char* d);

#endif