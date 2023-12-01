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
  //θελω πρακτικα ο πινακας αυτος να μπορει να με οδηγησει 
  //μεσου του χεντερ μπλοκ στις αναγκαιες πληροφοριες πχ το ιδιο το χας τειμπλ καθε αρχειου(σκεψη)
  array.count = 0;
  for (int i = 0; i < MAX_OPEN_FILES; i++) {
    array.file_array[i] = -1;
    //array.info->header_block = NULL;  //οχι ακριβως, ετσι θελω να εχω ενα για καθε θεση
  }
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
  CALL_BF(BF_CreateFile(filename));           			
    
  int file;                                   				
  CALL_BF(BF_OpenFile(filename, &file));

  HT_info* info;
  BF_Block* header_block;
  BF_Block_Init(&header_block);
  CALL_BF(BF_AllocateBlock(file,header_block));
  
  char* data;
  info = (HT_info*) BF_Block_GetData(header_block);
  //data = BF_Block_GetData(info->header_block);
  info->hash_table->global_depth = depth;
  info->hash_table->table[(int)pow(2,depth)];
  //HashTable* hash_table;
  //hash_table->global_depth = depth;
  //hash_table->table[(int)pow(2,depth)];

  //memcpy(data,&hash_table,sizeof(HashTable));

  BF_Block_SetDirty(header_block);
  CALL_BF(BF_UnpinBlock(header_block));
  CALL_BF(BF_Close(file));

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc) {
  int filedesc;
  CALL_BF(BF_OpenFile(fileName, &filedesc));
  for (int i = 0; i < MAX_OPEN_FILES; i++) {
    if (i == *indexDesc) {
      array.file_array[i] = filedesc;
      array.count += 1;
    } 
  }
  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
  CALL_BF(BF_Close(array.file_array[indexDesc]));
  array.count -= 1;
  array.file_array[indexDesc] = -1;
  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  int filedesc;
  filedesc = array.file_array[indexDesc];
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}

