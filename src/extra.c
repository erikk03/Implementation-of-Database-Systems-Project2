#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Function to convert a decimal to binary
char* int_to_bi(unsigned int k, int depth) {
    //return (k == 0 || k == 1 ? k : ((k % 2) + 10 * int_to_bi(k / 2)));
    if (depth <= 0) {
        printf("Invalid depth.\n");
        return NULL;
    }

    char* string = (char*)malloc((depth + 1) * sizeof(char)); // +1 for null terminator
    
    if (string == NULL) {
        printf("Memory allocation failed.\n");
        return NULL;
    }

    string[depth] = '\0'; // Null-terminate the string

    for (int i = depth - 1; i >= 0; i--) {
        int bit = (k >> i) & 1; // Extract bit at position i
        string[depth - 1 - i] = bit + '0'; // Convert bit to character
    }

    return string;
}

// Hash function
unsigned int hash_function(const char *k){   
    unsigned int current = 0;   
    unsigned int rot = 0;  
    int i = 0;  
    for (i = 0; i < strlen(k); i++){  
        rot = ((rot & (7 << 29)) >> 29) | (rot << 3);  
        rot += k[i];  
        current ^= rot;  
        rot = current;  
    }  
    return current;  
}