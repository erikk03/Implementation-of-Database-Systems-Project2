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
		return HT_ERROR;      	\
	}                         	\
}

// Global struct type Global_Array
Global_Array global_array;

HT_ErrorCode HT_Init() {
	
	global_array.active_files_num = 0;                                    // Active files are zero in the beginning
	for(int i = 0; i < MAX_OPEN_FILES; i++){                              // Init file_array to NULL because no file has been opened yet
		global_array.file_array[i] = NULL;
	}

	return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
	//possible changes: , might need to change global_array
	
	CALL_BF(BF_CreateFile(filename));           			
		
	int file;                                   				
	CALL_BF(BF_OpenFile(filename, &file));

	BF_Block* header_block = NULL;
	BF_Block_Init(&header_block);
	CALL_BF(BF_AllocateBlock(file,header_block));

	HT_info* info;											
	info = (HT_info*) BF_Block_GetData(header_block);						// Write HT_info to header_block of filename
	info->id = file; 														// Id of file

	HashTable* hash_table = (HashTable*)(info + sizeof(info));
	hash_table->global_depth = depth;
	hash_table->table = (Directory *)malloc(pow(2,hash_table->global_depth) * sizeof(Directory));
	
	if (hash_table->table == NULL) {
			fprintf(stderr, "Memory allocation failed\n");
			return 1; 														// Exit with an error code
	}

	for(int i=0; i < (int)pow(2,depth); i++){                              	// Hash table has 2^depth Directories
		hash_table->table[i].pointer = NULL;                                // NULL because no Buckets created yet
		strcpy(hash_table->table[i].id, int_to_bi(i, depth));				// Directory id in binary. e.x 00,01,10,11 for depth=2
	}
																			
	info->hash_table = hash_table;          				// Connect HT_Info with HashTable via pointer                                // Connect HT_info with Hashtable

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
			global_array.file_array[i] = ht_info;                                 // file_array[i] points to HT_info with filedesc == i
			*indexDesc = i;                                                       // Returns indexDesc as the position of opened file in file_array
			break;
		}
	}

	BF_Block_Destroy(&block);

	return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
	// must unpin blocks that are unpined
	BF_Block *block = NULL;
	BF_Block_Init(&block);

	CALL_BF(BF_GetBlock(indexDesc, 0, block));							           	// Get the first block(file header)
	CALL_BF(BF_UnpinBlock(block));													// Unpin the file header
		
	BF_Block_Destroy(&block);

	CALL_BF(BF_CloseFile(indexDesc));
	
	global_array.active_files_num--;
	global_array.file_array[indexDesc] = NULL;

	return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
	HashTable* hash_table = global_array.file_array[indexDesc]->hash_table;
	char string[32];
	int global_depth = hash_table->global_depth;
	strcpy(string, int_to_bi(record.id, global_depth));
	for(int i = 0; i < (int)pow(2,global_depth); i++){
		printf("%s\n",hash_table->table[i].id);
		if(strcmp(string, hash_table->table[i].id) == 0 && hash_table->table[i].pointer == NULL) {
			
			BF_Block *block = NULL;
			BF_Block_Init(&block);
			CALL_BF(BF_AllocateBlock(indexDesc, block));
			hash_table->table[i].pointer = (Bucket*)(block);
			hash_table->table[i].pointer->block = (BF_Block*)BF_Block_GetData(block);
			hash_table->table[i].pointer->local_depth = hash_table->global_depth;
			
			HT_block_info* block_info;
			char* data;
			data = BF_Block_GetData(block);
			block_info = (HT_block_info*)(data + BF_BLOCK_SIZE - sizeof(block_info));

			block_info->available_space = BF_BLOCK_SIZE - sizeof(block_info);
			block_info->number_of_records = 0;
			memcpy(data, &record, sizeof(record));
			block_info->available_space = block_info->available_space - sizeof(record);
			block_info->number_of_records++;
			BF_Block_SetDirty(block);
		}
		else if(strcmp(string, hash_table->table[i].id) == 0 && hash_table->table[i].pointer != NULL){
			// block_info just to take available space
			HT_block_info* block_info;
			char* data;
			data = (char*)(hash_table->table[i].pointer->block);
			block_info = (HT_block_info*)(data + BF_BLOCK_SIZE - sizeof(block_info));
			
			// If there is space in the block
			if(sizeof(record) <= block_info->available_space){
				memcpy(data + (block_info->number_of_records * sizeof(record)), &record, sizeof(record));
				block_info->available_space = block_info->available_space - sizeof(record);
				block_info->number_of_records++;
				BF_Block_SetDirty(hash_table->table[i].pointer->block);
			}
			else{

				// Create 2 new block with block_info for the bucket split
				BF_Block *new_block = NULL;
				BF_Block_Init(&new_block);
				CALL_BF(BF_AllocateBlock(indexDesc, new_block));

				BF_Block *new_block2 = NULL;
				BF_Block_Init(&new_block2);
				CALL_BF(BF_AllocateBlock(indexDesc, new_block2));
				
				HT_block_info* new_block_info;
				char* data1;
				data1 = BF_Block_GetData(new_block);
				new_block_info = (HT_block_info*)(data1 + BF_BLOCK_SIZE - sizeof(new_block_info));
				new_block_info->available_space = BF_BLOCK_SIZE - sizeof(new_block_info);
				new_block_info->number_of_records = 0;

				HT_block_info* new_block_info2;
				char* data2;
				data2 = BF_Block_GetData(new_block2);
				new_block_info2 = (HT_block_info*)(data2 + BF_BLOCK_SIZE - sizeof(new_block_info2));
				new_block_info2->available_space = BF_BLOCK_SIZE - sizeof(new_block_info2);
				new_block_info2->number_of_records = 0;


				printf("local depth: %d\n\n", hash_table->table[i].pointer->local_depth );
				printf("global depth: %d\n\n", hash_table->global_depth );

				// If global depth == local depth => Bucket split AND resize hash table
				if(hash_table->global_depth == hash_table->table[i].pointer->local_depth ) {
					int previous_depth = hash_table->global_depth;
					char previous_dir_id[32];
					strcpy(previous_dir_id, hash_table->table[i].id);
					
					hash_table->global_depth++;
					hash_table->table = (Directory *)realloc(hash_table->table, pow(2,hash_table->global_depth) * sizeof(int));
					if (hash_table->table == NULL) {
						fprintf(stderr, "Memory allocation failed\n");
						return 1; 																				// Exit with an error code
					}
					
					// Update directory ids for every directory
					for(int j=0; i < (int)pow(2,hash_table->global_depth); j++){                              	// Hash table has 2^depth Directories
						strcpy(hash_table->table[j].id, int_to_bi(j, hash_table->global_depth));				// Directory id in binary. e.x 00,01,10,11 for depth=2
					
					}

					// Init values for the new directories created
					for(int j=(int)pow(2,previous_depth); j<(int)pow(2,hash_table->global_depth); j++){
						if(strcmp(hash_table->table[j].id + 1, previous_dir_id) == 0){
							hash_table->table[j].pointer = (Bucket*)new_block2;
							hash_table->table[j].pointer->block = (BF_Block*)BF_Block_GetData(new_block2);
							hash_table->table[j].pointer->local_depth++;

						}else{
							hash_table->table[j].pointer = NULL;
						}
						
					}

					char* data = (char* )(hash_table->table[i].pointer->block);
					Record* temp_record =(Record*)(data);
					for(int j = 0; j < block_info->number_of_records; j++) {
						char temp[32];
						strcpy(temp, int_to_bi(temp_record[j].id, global_depth));
						
						if(strcmp(temp,hash_table->table[i].id) == 0) {
							//vale sto block1
							//data = BF_Block_GetData(new_block);
							memcpy(data1 + (new_block_info->number_of_records * sizeof(Record)), &temp_record[j], sizeof(Record));
							new_block_info->number_of_records;
							new_block_info->available_space = new_block_info->available_space - sizeof(Record);
						}else{
							//vale sto block2
							//data = BF_Block_GetData(new_block2);
							memcpy(data2 + (new_block_info2->number_of_records * sizeof(Record)), &temp_record[j], sizeof(Record));
							new_block_info2->number_of_records;
							new_block_info2->available_space = new_block_info2->available_space - sizeof(Record);
						}
					}
					hash_table->table[i].pointer = (Bucket*)new_block;
					hash_table->table[i].pointer->block = (BF_Block*)BF_Block_GetData(new_block);
					hash_table->table[i].pointer->local_depth++;

					//vale neo record 
					//ftiakse ta palia me vash to neo id
				}
				else if(hash_table->global_depth > hash_table->table[i].pointer->local_depth) {
					//bucket split
					//local ++
				}
			}
		
		}	
	}
		
	return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
	//insert code here
	return HT_OK;
}

