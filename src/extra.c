#include <string.h>

// Function to convert a decimal to binary
unsigned int int_to_bi(unsigned int k) {
    return (k == 0 || k == 1 ? k : ((k % 2) + 10 * int_to_bi(k / 2)));
    
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