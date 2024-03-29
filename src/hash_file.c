#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bf.h"
#include "hash_file.h"
#include "extra.h"
#define MAX_OPEN_FILES 20

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
#define CALL_OR_DIE(call)     			\
	{                           		\
		HT_ErrorCode code = call; 		\
		if (code != HT_OK) {      		\
			printf("Error\n"); \
			exit(code);             	\
		}                         		\
	}

// Global struct type Global_Array
Global_Array global_array;

int* pos_array;

HT_ErrorCode HT_Init() {
	
	global_array.active_files_num = 0;                                    // Active files are zero in the beginning
	for(int i = 0; i < MAX_OPEN_FILES; i++){                              // Init file_array to NULL because no file has been opened yet
		global_array.file_array[i] = NULL;
	}

	return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
	
	CALL_BF(BF_CreateFile(filename));           			
		
	int file;                                   				
	CALL_BF(BF_OpenFile(filename, &file));

	BF_Block* header_block = NULL;
	BF_Block_Init(&header_block);
	CALL_BF(BF_AllocateBlock(file,header_block));

	HT_info* info;											
	info = (HT_info*) BF_Block_GetData(header_block);						// Write HT_info to header_block of filename
	info->id = file; 														// Id of file

	HashTable* hash_table = (HashTable*)(info + sizeof(info));				// Write HashTable next to HT_info to header block of filename
	hash_table->global_depth = depth;
	hash_table->table = (Directory **)malloc(pow(2,hash_table->global_depth) * sizeof(Directory*));
	
	if (hash_table->table == NULL) {
			fprintf(stderr, "Memory allocation failed\n");
			return 1; 														// Exit with an error code
	}

	for(int i=0; i < (int)pow(2,depth); i++){                              	// Hash table has 2^depth Directories
		Directory *directory = (Directory*)malloc(sizeof(Directory));		// Allocate each Directory that HashTable has
		if (directory == NULL) {
			fprintf(stderr, "Memory allocation failed\n");
			return 1; 														// Exit with an error code
		}
		hash_table->table[i] = directory;									// Make table[i] to point to the Directory allocated
		hash_table->table[i]->pointer = NULL;                               // NULL because no Buckets created yet
		strcpy(hash_table->table[i]->id, int_to_bi(i, depth));				// Directory id in binary. e.x 00,01,10,11 for depth=2
	}
																			
	info->hash_table = hash_table;          								// Connect HT_Info with HashTable via pointer

	pos_array = (int*)malloc(pow(2,hash_table->global_depth)*sizeof(int));	// Allocate pos_array of size 2^global_depth 
	
	for(int i=0; i<pow(2,hash_table->global_depth); i++){					// Init values of pos_array
		pos_array[i] = -1;
	}

	BF_Block_SetDirty(header_block);
	
	CALL_BF(BF_UnpinBlock(header_block));
	BF_Block_Destroy(&header_block);

	CALL_BF(BF_CloseFile(file));

	return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc) {
	int filedesc;
	CALL_BF(BF_OpenFile(fileName, &filedesc));
	
	BF_Block *block = NULL;
	BF_Block_Init(&block);

	CALL_BF(BF_GetBlock(filedesc, 0, block));

	HT_info* ht_info;
	ht_info = (HT_info*)BF_Block_GetData(block);

	for(int i=0; i<MAX_OPEN_FILES; i++){
		if(i == filedesc){                                                      // For file with filedesc == i
			global_array.active_files_num++;
			global_array.file_array[i] = ht_info;                               // file_array[i] points to HT_info with filedesc == i
			*indexDesc = i;                                                     // Returns indexDesc as the position of opened file in file_array
			break;
		}
	}

	BF_Block_Destroy(&block);

	return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
	BF_Block *block = NULL;
	BF_Block_Init(&block);

	CALL_BF(BF_GetBlock(indexDesc, 0, block));							         // Get the first block(file header)
	CALL_BF(BF_UnpinBlock(block));												 // Unpin the file header
		
	BF_Block_Destroy(&block);

	CALL_BF(BF_CloseFile(indexDesc));
	
	// free memory that was allocated dynamically
	free_memory(global_array.file_array[indexDesc]->hash_table, global_array.file_array[indexDesc]->hash_table->global_depth);
	
	global_array.active_files_num--;
	global_array.file_array[indexDesc] = NULL;

	

	return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
	HashTable* hash_table = global_array.file_array[indexDesc]->hash_table;

	char string[32] = {};
	int global_depth = hash_table->global_depth;
	strcpy(string, int_to_bi(record.id, global_depth));
	
	// For each directory
	for(int i = 0; i < (int)pow(2,global_depth); i++){
		BF_Block* bf_block = NULL;
		BF_Block_Init(&bf_block);
		
		HT_block_info* block_info;
		char* data;

		// If directory does not point to a block
		if(strcmp(string, hash_table->table[i]->id) == 0 && hash_table->table[i]->pointer == NULL) {
			CALL_BF(BF_AllocateBlock(indexDesc, bf_block));
			hash_table->table[i]->pointer = bf_block;

			int blocks_num;
			CALL_BF(BF_GetBlockCounter(indexDesc, &blocks_num));
			pos_array[i] = blocks_num -1;

			data = BF_Block_GetData(bf_block);

			block_info = (HT_block_info*)(data + BF_BLOCK_SIZE - sizeof(block_info));
			block_info->available_space = BF_BLOCK_SIZE - sizeof(block_info);
			block_info->number_of_records = 0;
			memcpy(data, &record, sizeof(record));
			block_info->available_space = block_info->available_space - sizeof(record);
			block_info->number_of_records++;
			block_info->local_depth = hash_table->global_depth;

			BF_Block_SetDirty(bf_block);

			// BF_UnpinBlock(bf_block);
			// BF_Block_Destroy(&bf_block);

		}
		else if(strcmp(string, hash_table->table[i]->id) == 0 && hash_table->table[i]->pointer != NULL){	// If dir already points to a block
			// block_info just to take available space

			CALL_BF(BF_GetBlock(indexDesc, pos_array[i], bf_block));

			data = BF_Block_GetData(bf_block);
			block_info = (HT_block_info*)(data + BF_BLOCK_SIZE - sizeof(block_info));

			// If there is space in the block
			if(sizeof(record) <= block_info->available_space){
				memcpy(data + (block_info->number_of_records * sizeof(record)), &record, sizeof(record));
				block_info->available_space = block_info->available_space - sizeof(record);
				block_info->number_of_records++;
				
				BF_Block_SetDirty(bf_block);

			}
			else if((sizeof(record) > block_info->available_space) && (hash_table->global_depth == block_info->local_depth)){
				// If global depth == local depth => Bucket split AND resize hash table

				double_ht(hash_table);

				CALL_OR_DIE(bucket_split(hash_table, bf_block, indexDesc, record));
					
			}	
			else if((sizeof(record) > block_info->available_space) &&(hash_table->global_depth > block_info->local_depth)) {

				CALL_OR_DIE(bucket_split(hash_table, bf_block, indexDesc, record));

			}
			
		CALL_BF(BF_UnpinBlock(bf_block));

		}

		// CALL_BF(BF_UnpinBlock(bf_block));
		// BF_Block_Destroy(&bf_block);

	}
	return HT_OK;

}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
    HashTable* hash_table = global_array.file_array[indexDesc]->hash_table;
	
	BF_Block* block = NULL;
	BF_Block_Init(&block);
    
	HT_block_info* block_info;
    
	Record* record;

    for(int i=0; i<pow(2,hash_table->global_depth); i++){
		BF_GetBlock(indexDesc, pos_array[i], block);								// Get block that dir with id=i points to
		char* data = BF_Block_GetData(block);

        block_info = (HT_block_info*)(data + BF_BLOCK_SIZE -sizeof(block_info));
        record =(Record*)(data);
        for(int j = 0; j < block_info->number_of_records; j++) {                    // For each record inside the block
			if(id==NULL) {
                printRecord(record[j]);
                continue;
            }else if(record[j].id == *id){
                printRecord(record[j]);
            }    
        }
	}

    return HT_OK;
}


HT_ErrorCode HashStatistics(char *fileName) {
	
	int indexDesc;
	CALL_BF(BF_OpenFile(fileName, &indexDesc));

	HashTable* hash_table = global_array.file_array[indexDesc]->hash_table;
	
	BF_Block *block = NULL;
	BF_Block_Init(&block);

	HT_block_info* block_info;
	
	int blocks_number;
    CALL_BF(BF_GetBlockCounter(indexDesc, &blocks_number));

	int min_rec = RECORDS_NUM;
	float average_rec = RECORDS_NUM/blocks_number;
	int max_rec = -1;

	for(int i = 0; i < blocks_number; i++){
		CALL_BF(BF_GetBlock(indexDesc, i, block));								// Get block i in file
	
		char* data = BF_Block_GetData(block);									// Get it's data
		block_info = (HT_block_info*)(data + BF_BLOCK_SIZE - sizeof(block_info));
		// min_rec
		if(block_info->number_of_records < min_rec){
			min_rec = block_info->number_of_records;
		}
		// max_rec
		if(block_info->number_of_records > max_rec){
			max_rec = block_info->number_of_records;
		}

		CALL_BF(BF_UnpinBlock(block));	
	}
	
	BF_Block_Destroy(&block);
	
	printf("\nSTATISTICS OF FILE:\"%s\"\n", fileName);
	printf("Number of blocks:%d\n", blocks_number);
	printf("Minimum number of records in a bucket:%d\n", min_rec);
	printf("Average number of records:%.2f\n", average_rec);
	printf("Maximum number of records in a bucket:%d\n\n", max_rec);
	
	CALL_BF(BF_CloseFile(indexDesc));
	
	return HT_OK;
}

