// Function to convert a decimal to binary
char* decimalToBinary(int decimal);

// Function to convert a decimal to binary but returning only depth bits
char* int_to_bi(unsigned int k, int depth);

// My hash function that uses FNV hash
char* my_hash_func(unsigned int k, int depth);

// Function to split a bucket when there is no more available space to enter a new record
HT_ErrorCode bucket_split(HashTable* hash_table, BF_Block* bf_block, int indexDesc, Record record);

// Function that doubles the size of hash table
void double_ht(HashTable* hash_table);

// Print a record
void printRecord(Record record);

// Free allocated memory for hash_table and each directory
void free_memory(HashTable* hashtable, int depth);
