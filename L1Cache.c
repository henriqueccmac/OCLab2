#include "L1Cache.h"
#include "Cache.h"


uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache cache;

/**************** Auxiliar Functions **************/
uint32_t indexExtractor(uint32_t address, int indexSize, int offsetSize){
  uint32_t index;
  address = address >> offsetSize;
  uint32_t mask = (1 << indexSize) - 1;
  index = address & mask; 
  return index;
}
uint32_t offsetExtractor(uint32_t address, int offsetSize){
  uint32_t offset;
  uint32_t mask = (1 << offsetSize) - 1;
  offset = address & mask; 
  return offset;
}
/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

  if (address >= DRAM_SIZE - WORD_SIZE + 1)
    exit(-1);

  if (mode == MODE_READ) {
    memcpy(data, &(DRAM[address]), BLOCK_SIZE);
    time += DRAM_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    memcpy(&(DRAM[address]), data, BLOCK_SIZE);
    time += DRAM_WRITE_TIME;
  }
}

/*********************** L1 cache *************************/

void initCache() { cache.init = 0; }

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, Tag, MemAddress, offset;
  uint8_t TempBlock[BLOCK_SIZE];

  /* init cache */
  if (cache.init == 0) {
    for (int i = 0; i < L1_LINES; i++){
      cache.line[i].Valid = 0;
      cache.line[i].Dirty = 0;
      cache.line[i].Tag = 0;
      cache.init = 1;
      for (int j=0; j<BLOCK_SIZE; j+=WORD_SIZE)
        cache.line[i].Data[j] = 0;
    }
  }  

  Tag = address >> (OFFSET_SIZE + L1_INDEX_SIZE); // Why do I do this?
  index = indexExtractor(address, L1_INDEX_SIZE, OFFSET_SIZE);
  offset = offsetExtractor(address, OFFSET_SIZE);
  
  MemAddress = address >> (OFFSET_SIZE); // again this....!
  MemAddress = MemAddress << (OFFSET_SIZE); // address of the block in memory (address - offset)

  /* access Cache*/

  if (cache.line[index].Valid && cache.line[index].Tag == Tag) {    
    if (mode == MODE_READ) {    // read data from cache line
      memcpy(data, &(cache.line[index].Data[offset]), WORD_SIZE);
      time += L1_READ_TIME;
    }

    if (mode == MODE_WRITE) { // write data from cache line
      memcpy(&(cache.line[index].Data[offset]), data, WORD_SIZE);
      time += L1_WRITE_TIME;
      cache.line[index].Dirty = 1;
    }
  }

  else{ 
    if (cache.line[index].Dirty) { // line has dirty block
      accessDRAM(MemAddress, cache.line[index].Data, MODE_WRITE); // then write back old block
    }

    accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM 
    memcpy(&(cache.line[index].Data[offset]), TempBlock,BLOCK_SIZE); // copy new block to cache line
    cache.line[index].Valid = 1;
    cache.line[index].Tag = Tag;
    cache.line[index].Dirty = 0;
  } // if miss, then replaced with the correct block
}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
