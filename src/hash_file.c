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

HT_ErrorCode HT_Init() {
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
  CALL_BF(BF_CreateFile(filename));           			
    
  int file;                                   				
  CALL_BF(BF_OpenFile(filename, &file));

  BF_Block* header_block;
  BF_Block_Init(&header_block);
  CALL_BF(BF_AllocateBlock(file,header_block));
  
  char* data;
  data = BF_Block_GetData(header_block);

  HashTable* hash_table;
  hash_table->global_depth = depth;
  hash_table->table[(int)pow(2,depth)];

  memcpy(data,&hash_table,sizeof(HashTable));

  BF_Block_SetDirty(header_block);
  CALL_BF(BF_UnpinBlock(header_block));
  CALL_BF(BF_Close(file));

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}

