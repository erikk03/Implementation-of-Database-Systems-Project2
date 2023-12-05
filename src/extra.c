#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include "bf.h"
#include "hash_file.h"

#define HT_ERROR -1
#define CALL_BF(call)       	\
{                           	\
	BF_ErrorCode code = call; 	\
	if (code != BF_OK) {      	\
		BF_PrintError(code);  	\
		printf("error_code:%d\n", code);\
		return HT_ERROR;      	\
	}                         	\
}

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
// https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
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

    //printf("k:%d\nbinary:%s\n", k, decimalToBinary(k));
    int capacity = pow(2,depth);
    uint32_t hash = hash_key(decimalToBinary(k), 32);
    unsigned int index = (unsigned int)(hash & (uint32_t)(capacity -1));
    //printf("new_binary:%s\nreturn:%s\n", decimalToBinary(index), int_to_bi(index, depth));
    return int_to_bi(index, depth);
}

// Bucket split
HT_ErrorCode bucket_split(HashTable* hash_table, Bucket* bucket, int indexDesc, Record record_to_insert, int double_ht){

    // Take block and it's info that is going to be split
    BF_Block* block_to_split = NULL;
    BF_Block_Init(&block_to_split);
    block_to_split = bucket->block;

    HT_block_info* block_info;
    char* data;
	data = (char*)(block_to_split);
	block_info = (HT_block_info*)(data + BF_BLOCK_SIZE - sizeof(block_info));

    // Allocate the 2 new blocks to file
    BF_Block* new_block = NULL;
    BF_Block_Init(&new_block);
    CALL_BF(BF_AllocateBlock(indexDesc, new_block));
    
    BF_Block *new_block2 = NULL;
    BF_Block_Init(&new_block2);
    CALL_BF(BF_AllocateBlock(indexDesc, new_block2));

    // Take info for each block and initialize them
    HT_block_info* new_block_info;
    char* data1;
    data1 = BF_Block_GetData(new_block);
    new_block_info = (HT_block_info*)(data1 + BF_BLOCK_SIZE - sizeof(new_block_info));
    new_block_info->available_space = BF_BLOCK_SIZE - sizeof(new_block_info);
    new_block_info->number_of_records = 0;
    new_block_info->local_depth = block_info->local_depth + 1;
    
    HT_block_info* new_block_info2;
    char* data2;
    data2 = BF_Block_GetData(new_block2);
    new_block_info2 = (HT_block_info*)(data2 + BF_BLOCK_SIZE - sizeof(new_block_info));
    new_block_info2->available_space = BF_BLOCK_SIZE - sizeof(new_block_info);
    new_block_info2->number_of_records = 0;
    new_block_info2->local_depth = block_info->local_depth + 1;
    
    // Find buddies of a bucket
    int number_of_dir = (int)pow(2,hash_table->global_depth);
    char temp_array[number_of_dir][32];          // Temporary array to save id's of buddies
    int number_of_buddies = 0;
    
    for(int i=0; i<number_of_dir; i++){
        if(hash_table->table[i]->pointer == bucket){
            strcpy(temp_array[i], hash_table->table[i]->id);
            number_of_buddies ++;
        }else{
            strcpy(temp_array[i], "EMPTY");
        }
    }
    
    // If we don't want to double our hash table
    //if(number_of_buddies > 1){
        // Loop to adjust half the directories that pointed to block_to_split, now make them point to new_block
        int changed_pointer = 0;                                                                                        // Number of pointers that have been changed
        for(int i=0; i<number_of_dir; i++){
            for(int j=0; j<number_of_dir; j++){
                if((strcmp(hash_table->table[i]->id, temp_array[j]) == 0) && (changed_pointer < number_of_buddies/2)){  // If less than half of directories that point to a bucket have been changed
                    hash_table->table[i]->pointer = (Bucket*)new_block;                                                 // Make directory point to new_block
                    hash_table->table[i]->pointer->block = (BF_Block*)data1;
                    strcpy(temp_array[j], "DONE");                                                                      // Check dir in array with buddies as DONE
                    changed_pointer ++;
                }
            }
        }
        // Now we just want to make the other half directories to point to new_block2
        for(int i=0; i<number_of_dir; i++){
            for(int j=0; j<number_of_dir; j++) {
                if(strcmp(hash_table->table[i]->id, temp_array[j]) == 0) {
                    hash_table->table[i]->pointer = (Bucket*)new_block2;                                               // Make directory point to new_block
                    hash_table->table[i]->pointer->block = (BF_Block*)data2;
                    strcpy(temp_array[j], "DONE");                                                                     // Check dir in array with buddies as DONE
                    changed_pointer ++;
                }
            }
        }
    //}

    // Divide records of block_to_split to two new blocks that we created
    for(int i=0; i<number_of_dir; i++){
        Record* record_to_move = (Record*)data;
        char temp[32];
        
        for(int j=0; j<block_info->number_of_records; j++){
            strcpy(temp, my_hash_func(record_to_move[j].id, block_info->local_depth + 1));

            if(strcmp(hash_table->table[i]->id, temp) == 0 && (hash_table->table[i]->pointer == (Bucket*)new_block)){
                memcpy(data1 + (new_block_info->number_of_records * sizeof(Record)), &record_to_move[j], sizeof(Record));
                new_block_info->number_of_records++;
                new_block_info->available_space = new_block_info->available_space - sizeof(Record);
                BF_Block_SetDirty(new_block);
                
            }
            else if(strcmp(hash_table->table[i]->id, temp) == 0 && (hash_table->table[i]->pointer == (Bucket*)new_block2)){
                memcpy(data2 + (new_block_info2->number_of_records * sizeof(Record)), &record_to_move[j], sizeof(Record));
                new_block_info2->number_of_records++;
                new_block_info2->available_space = new_block_info2->available_space - sizeof(Record);
                BF_Block_SetDirty(new_block2);

            }
        }
    }
    
    // Copy the record_to_insert where it belongs better, if there is available space in that block
    for(int i=0; i<number_of_dir; i++){
        char temp[32];
        strcpy(temp, my_hash_func(record_to_insert.id, block_info->local_depth + 1));

        if(strcmp(hash_table->table[i]->id, temp) == 0 && (hash_table->table[i]->pointer == (Bucket*)new_block)){
            
            // If there is available space in new_block
            if(new_block_info->available_space >= sizeof(record_to_insert)){
                // Copy to new_block
                memcpy(data1 + (new_block_info->number_of_records * sizeof(Record)), &record_to_insert, sizeof(Record));
                new_block_info->number_of_records++;
                new_block_info->available_space = new_block_info->available_space - sizeof(record_to_insert);
                BF_Block_SetDirty(new_block);
            }else{
                // Copy to new_block2
                memcpy(data2 + (new_block_info2->number_of_records * sizeof(Record)), &record_to_insert, sizeof(Record));
                new_block_info2->number_of_records++;
                new_block_info2->available_space = new_block_info2->available_space - sizeof(record_to_insert);
                BF_Block_SetDirty(new_block2);
            }
        }
        else if(strcmp(hash_table->table[i]->id, temp) == 0 && (hash_table->table[i]->pointer == (Bucket*)new_block2)){
            
            //If there is available space in new_block2
            if(new_block_info2->available_space >= sizeof(record_to_insert)){
                // Copy to new_block2
                memcpy(data2 + (new_block_info2->number_of_records * sizeof(Record)), &record_to_insert, sizeof(Record));
                new_block_info2->number_of_records++;
                new_block_info2->available_space = new_block_info2->available_space - sizeof(record_to_insert);
                BF_Block_SetDirty(new_block2);
            }else{
                // Copy to new_block
                memcpy(data1 + (new_block_info->number_of_records * sizeof(Record)), &record_to_insert, sizeof(Record));
                new_block_info->number_of_records++;
                new_block_info->available_space = new_block_info->available_space - sizeof(record_to_insert);
                BF_Block_SetDirty(new_block);
            }
        }
    }

    //MIN KSEXASOUME UNPIN DESTROY BLA BLA TOY PALIOU BLOCK POY ESPASE
    BF_UnpinBlock(bucket->block);
	BF_Block_Destroy(&bucket->block);

    return HT_OK;
}

// Bucket split
HT_ErrorCode split_and_double(HashTable* hash_table, Bucket* bucket, int indexDesc, Record record_to_insert, int double_ht){

    // Take block and it's info that is going to be split
    BF_Block* block_to_split = NULL;
    BF_Block_Init(&block_to_split);
    block_to_split = bucket->block;

    HT_block_info* block_info;
    char* data;
	data = (char*)(block_to_split);
	block_info = (HT_block_info*)(data + BF_BLOCK_SIZE - sizeof(block_info));

    // Allocate the 2 new blocks to file
    BF_Block* new_block = NULL;
    BF_Block_Init(&new_block);
    CALL_BF(BF_AllocateBlock(indexDesc, new_block));
    
    BF_Block *new_block2 = NULL;
    BF_Block_Init(&new_block2);
    CALL_BF(BF_AllocateBlock(indexDesc, new_block2));

    // Take info for each block and initialize them
    HT_block_info* new_block_info;
    char* data1;
    data1 = BF_Block_GetData(new_block);
    new_block_info = (HT_block_info*)(data1 + BF_BLOCK_SIZE - sizeof(new_block_info));
    new_block_info->available_space = BF_BLOCK_SIZE - sizeof(new_block_info);
    new_block_info->number_of_records = 0;
    new_block_info->local_depth = block_info->local_depth + 1;
    
    HT_block_info* new_block_info2;
    char* data2;
    data2 = BF_Block_GetData(new_block2);
    new_block_info2 = (HT_block_info*)(data2 + BF_BLOCK_SIZE - sizeof(new_block_info));
    new_block_info2->available_space = BF_BLOCK_SIZE - sizeof(new_block_info);
    new_block_info2->number_of_records = 0;
    new_block_info2->local_depth = block_info->local_depth + 1;
    
    int number_of_dir = (int)pow(2,hash_table->global_depth);
    
    char foo_id[32];
    // Connect new directories to buckets/blocks that existed before doubling the hash table
    for(int i=0; i<number_of_dir/2; i++){
        strcpy(foo_id, hash_table->table[i]->id + 1);
        for(int j=number_of_dir/2; j<number_of_dir; j++){
            
            if(strcmp(foo_id, hash_table->table[j]->id + 1) == 0){
                hash_table->table[j]->pointer = hash_table->table[i]->pointer;
                hash_table->table[j]->pointer->block = hash_table->table[i]->pointer->block;
            }   
        }
    }

    // Find buddies of a bucket
    char temp_array[number_of_dir][32];          // Temporary array to save id's of buddies
    int number_of_buddies = 0;
    
    for(int i=0; i<number_of_dir; i++){
        if(hash_table->table[i]->pointer == bucket){
            strcpy(temp_array[i], hash_table->table[i]->id);
            number_of_buddies ++;
        }else{
            strcpy(temp_array[i], "EMPTY");
        }
    }
    

    //if(number_of_buddies > 1){
        // Loop to adjust half the directories that pointed to block_to_split, now make them point to new_block
        int changed_pointer = 0;                                                                                        // Number of pointers that have been changed
        for(int i=0; i<number_of_dir; i++){
            for(int j=0; j<number_of_dir; j++){
                if((strcmp(hash_table->table[i]->id, temp_array[j]) == 0) && (changed_pointer < number_of_buddies/2)){  // If less than half of directories that point to a bucket have been changed
                    hash_table->table[i]->pointer = (Bucket*)new_block;                                                 // Make directory point to new_block
                    hash_table->table[i]->pointer->block = (BF_Block*)data1;
                    strcpy(temp_array[j], "DONE");                                                                      // Check dir in array with buddies as DONE
                    changed_pointer ++;
                }
            } 
        }
        for(int i=0; i<number_of_dir; i++){
            printf("%d:%s:%s\n", i, temp_array[i], hash_table->table[i]->id);
        }
        // Now we just want to make the other half directories to point to new_block2
        for(int i=0; i<number_of_dir; i++){
            for(int j=0; j<number_of_dir; j++) {
                if(strcmp(hash_table->table[i]->id, temp_array[j]) == 0 ) {
                    hash_table->table[i]->pointer = (Bucket*)new_block2;                                               // Make directory point to new_block
                    hash_table->table[i]->pointer->block = (BF_Block*)data2;
                    strcpy(temp_array[j], "DONE");                                                                     // Check dir in array with buddies as DONE
                    changed_pointer ++;
                }
            }
        }
        for(int i=0; i<number_of_dir; i++){
            printf("%d:%s:%s\n", i, temp_array[i], hash_table->table[i]->id);
        }
    //}

    // Divide records of block_to_split to two new blocks that we created
    for(int i=0; i<number_of_dir; i++){
        Record* record_to_move = (Record*)data;
        char temp[32];
        
        for(int j=0; j<block_info->number_of_records; j++){                                     // global_depth = block_info->local_depth + 1 at this point
            strcpy(temp, my_hash_func(record_to_move[j].id, block_info->local_depth + 1));      // block_info->local_depth +1 beacause we hash based on local depth of new blocks

            if(strcmp(hash_table->table[i]->id, temp) == 0 && (hash_table->table[i]->pointer == (Bucket*)new_block)){
                memcpy(data1 + (new_block_info->number_of_records * sizeof(Record)), &record_to_move[j], sizeof(Record));
                new_block_info->number_of_records++;
                new_block_info->available_space = new_block_info->available_space - sizeof(Record);
                BF_Block_SetDirty(new_block);
                
            }
            else if(strcmp(hash_table->table[i]->id, temp) == 0 && (hash_table->table[i]->pointer == (Bucket*)new_block2)){
                memcpy(data2 + (new_block_info2->number_of_records * sizeof(Record)), &record_to_move[j], sizeof(Record));
                new_block_info2->number_of_records++;
                new_block_info2->available_space = new_block_info2->available_space - sizeof(Record);
                BF_Block_SetDirty(new_block2);

            }
        }
    }
    
    // Copy the record_to_insert where it belongs better, if there is available space in that block
    for(int i=0; i<number_of_dir; i++){
        char temp[32];
        strcpy(temp, my_hash_func(record_to_insert.id, block_info->local_depth + 1));

        if(strcmp(hash_table->table[i]->id, temp) == 0 && (hash_table->table[i]->pointer == (Bucket*)new_block)){
            
            // If there is available space in new_block
            if(new_block_info->available_space >= sizeof(record_to_insert)){
                // Copy to new_block
                memcpy(data1 + (new_block_info->number_of_records * sizeof(Record)), &record_to_insert, sizeof(Record));
                new_block_info->number_of_records++;
                new_block_info->available_space = new_block_info->available_space - sizeof(record_to_insert);
                BF_Block_SetDirty(new_block);
            }else{
                // Copy to new_block2
                memcpy(data2 + (new_block_info2->number_of_records * sizeof(Record)), &record_to_insert, sizeof(Record));
                new_block_info2->number_of_records++;
                new_block_info2->available_space = new_block_info2->available_space - sizeof(record_to_insert);
                BF_Block_SetDirty(new_block2);
            }
        }
        else if(strcmp(hash_table->table[i]->id, temp) == 0 && (hash_table->table[i]->pointer == (Bucket*)new_block2)){
            
            //If there is available space in new_block2
            if(new_block_info2->available_space >= sizeof(record_to_insert)){
                // Copy to new_block2
                memcpy(data2 + (new_block_info2->number_of_records * sizeof(Record)), &record_to_insert, sizeof(Record));
                new_block_info2->number_of_records++;
                new_block_info2->available_space = new_block_info2->available_space - sizeof(record_to_insert);
                BF_Block_SetDirty(new_block2);
            }else{
                // Copy to new_block
                memcpy(data1 + (new_block_info->number_of_records * sizeof(Record)), &record_to_insert, sizeof(Record));
                new_block_info->number_of_records++;
                new_block_info->available_space = new_block_info->available_space - sizeof(record_to_insert);
                BF_Block_SetDirty(new_block);
            }
        }
    }

    //MIN KSEXASOUME UNPIN DESTROY BLA BLA TOY PALIOU BLOCK POY ESPASE
    BF_UnpinBlock(bucket->block);
	BF_Block_Destroy(&bucket->block);

    return HT_OK;
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
