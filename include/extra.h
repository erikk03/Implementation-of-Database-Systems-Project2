// Function to convert a decimal to binary
char* decimalToBinary(int decimal);

// Function to convert a decimal to binary but returning only depth bits
char* int_to_bi(unsigned int k, int depth);

// My hash function that uses FNV hash
char* my_hash_func(unsigned int k, int depth);

// Function to split a bucket when there is no more available space to enter a new record
// case_bs == 0 when only bucket_split is required
// case_bs == 1 when we wand bucket_split and hash table duplication
HT_ErrorCode bucket_split(HashTable* hash_table, Bucket* bucket, int indexDesc, Record record, int case_bs);


HT_ErrorCode split_and_double(HashTable* hash_table, Bucket* bucket, int indexDesc, Record record, int case_bs);

// Print a record
void printRecord(Record record);

// Free allocated memory for hash_table and each directory
void free_memory(HashTable* hashtable, int depth);
