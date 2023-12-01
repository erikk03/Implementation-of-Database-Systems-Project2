#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20

#define HT_ERROR -1
#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HT_ERROR;        \
  }                         \
}

// Global struct type Global_Array
Global_Array global_array;

// Function to convert an integer to binary
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

HT_ErrorCode HT_Init() {
  
  global_array.active_files_num = 0;                                    // Active files are zero in the beginning
  for(int i = 0; i < MAX_OPEN_FILES; i++){                              // Init file_array to NULL because no file has been opened yet
    global_array.file_array[i] = NULL;
  }

  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
  //possible changes: hash_table -> dynamic , might need to change global_array
  CALL_BF(BF_CreateFile(filename));           			
    
  int file;                                   				
  CALL_BF(BF_OpenFile(filename, &file));

  BF_Block* header_block = NULL;
  BF_Block_Init(&header_block);
  CALL_BF(BF_AllocateBlock(file,header_block));

  HashTable* hash_table;
  hash_table->global_depth = depth;
  for(int i=0; i < (int)pow(2,depth); i++){                               // Hash table has 2^depth Directories
    hash_table->table[i]->pointer = NULL;                                 // NULL because no Buckets created yet
    hash_table->table[i]->id = int_to_bi(i);                              // Directory id in binary. e.x 00,01,10,11 for depth=2
  }
  
  HT_info* info;
  info = (HT_info*) BF_Block_GetData(header_block);                       // Write HT_info to header_block of filename
  info->id = file;                                                        // Id of file
  info->hash_table = hash_table;                                          // Connect HT_info with Hashtable
  

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
  CALL_BF(BF_CloseFile(indexDesc));
  global_array.active_files_num--;
  global_array.file_array[indexDesc] = NULL;

  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  // int filedesc;
  // filedesc = array.file_array[indexDesc];
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}

