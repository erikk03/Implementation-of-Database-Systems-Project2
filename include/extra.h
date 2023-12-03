// Function to convert a decimal to binary
char* int_to_bi(unsigned int k, int depth);

// Hash function
unsigned int hash_function(const char *k);

// Free allocated memory for hash_table and each directory
void free_memory(HashTable* hashtable, int depth);