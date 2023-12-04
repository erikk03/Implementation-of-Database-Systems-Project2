#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include "bf.h"
#include "hash_file.h"

// Function to convert a decimal to char
char* decimalToBinary(int decimal) {
    // Size of an integer in bits
    int size = sizeof(int) * 8;

    char* binary = (char*)malloc(size + 1);     // +1 for null terminator
    
    if (binary == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }

    binary[size] = '\0';                        // Null-terminate the string

    // Loop to convert each bit
    for (int i = size - 1; i >= 0; i--) {
        int bit = (decimal >> i) & 1;
        binary[size - 1 - i] = bit + '0';
    }

    return binary;
}

// Function to convert a decimal to binary but returning only depth bits
char* int_to_bi(unsigned int k, int depth) {

    if (depth <= 0) {
        printf("Invalid depth.\n");
        return NULL;
    }

    char* string = (char*)malloc((depth + 1) * sizeof(char));   // +1 for null terminator
    
    if (string == NULL) {
        printf("Memory allocation failed.\n");
        return NULL;
    }

    string[depth] = '\0';                                       // Null-terminate the string

    for (int i = depth - 1; i >= 0; i--) {
        int bit = (k >> i) & 1;                                 // Extract bit at position i
        string[depth - 1 - i] = bit + '0';                      // Convert bit to character
    }

    return string;
}


// Return 32-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
#define OFFSET_BASIS 2166136261u
#define FNV_PRIME 16777619u

uint32_t hash_key(char* data, size_t bytes){
   uint32_t h = OFFSET_BASIS;
   for (size_t i = 0; i < bytes; ++i){
      h = (h ^ data[i]) * FNV_PRIME;
   }

   return h;
}

// My hash function
char* my_hash_func(unsigned int k, int depth){

    printf("k:%d\nbinary:%s\n", k, decimalToBinary(k));
    int capacity = pow(2,depth);
    uint32_t hash = hash_key(decimalToBinary(k), 32);
    unsigned int index = (unsigned int)(hash & (uint32_t)(capacity -1));
    printf("new_binary:%s\nreturn:%s\n", decimalToBinary(index), int_to_bi(index, depth));
    return int_to_bi(index, depth);;
}

// Print record
void printRecord(Record record){
    printf("(%d,%s,%s,%s)\n",record.id,record.name,record.surname,record.city);

}

// Free allocated memory for hash_table and each directory
void free_memory(HashTable* hashtable, int depth){
    for(int i=0; i<(int)pow(2,depth); i++){
        free(hashtable->table[i]);
    }
    free(hashtable->table);
}
