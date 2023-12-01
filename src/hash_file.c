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

Global_Array array;


HT_ErrorCode HT_Init() {
  //θελω πρακτικα ο πινακας αυτος να μπορει να με οδηγησει 
  //μεσου του χεντερ μπλοκ στις αναγκαιες πληροφοριες πχ το ιδιο το χας τειμπλ καθε αρχειου(σκεψη)
  array.count = 0;
  for (int i = 0; i < MAX_OPEN_FILES; i++) {
    array.file_array[i] = -1;
    array.info->header_block = NULL;  //οχι ακριβως, ετσι θελω να εχω ενα για καθε θεση
  }
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
  CALL_BF(BF_CreateFile(filename));           			
    
  int file;                                   				
  CALL_BF(BF_OpenFile(filename, &file));

  HT_info* info;
  BF_Block_Init(&info->header_block);
  CALL_BF(BF_AllocateBlock(file,info->header_block));
  
  char* data;
  data = BF_Block_GetData(info->header_block);

  HashTable* hash_table;
  hash_table->global_depth = depth;
  hash_table->table[(int)pow(2,depth)];

  memcpy(data,&hash_table,sizeof(HashTable));

  BF_Block_SetDirty(info->header_block);
  CALL_BF(BF_UnpinBlock(info->header_block));
  CALL_BF(BF_Close(file));

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc) {
  int filedesc;
  CALL_BF(BF_OpenFile(fileName, &filedesc));
  for (int i = 0; i < MAX_OPEN_FILES; i++) {
    if (array.file_array[i] == -1 && i == *indexDesc) {
      array.file_array[i] = filedesc;
      array.count += 1;
    } 
  }
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

