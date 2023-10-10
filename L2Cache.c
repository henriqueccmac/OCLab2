#include "L2Cache.h"
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

void initCache() {    
  for (int i = 0; i < L1_LINES; i++){
      cache.linesL1[i].Valid = 0;
      cache.linesL1[i].Dirty = 0;
      cache.linesL1[i].Tag = 0;
      cache.init = 1;
      for (int j=0; j<BLOCK_SIZE; j+=WORD_SIZE)
        cache.linesL1[i].Data[j] = 0;
    }

    for (int i = 0; i < L2_LINES; i++){
      cache.linesL2[i].Valid = 0;
      cache.linesL2[i].Dirty = 0;
      cache.linesL2[i].Tag = 0;
      cache.init = 1;
      for (int j=0; j<BLOCK_SIZE; j+=WORD_SIZE)
        cache.linesL2[i].Data[j] = 0;
    }
  }

void accessL2(int address, unsigned char *data, int mode) {

  unsigned int tag, index, offset;
  
  tag = address >> (OFFSET_SIZE + L2_INDEX_SIZE); // Why do I do this?
  index = indexExtractor(address, L2_INDEX_SIZE, OFFSET_SIZE);
  offset = offsetExtractor(address, OFFSET_SIZE);

  if (cache.linesL2[index].Valid && cache.linesL2[index].Tag == tag) {    
    if (mode == MODE_READ) {    // read data from cache line
      memcpy(data, &(cache.linesL2[index].Data[offset]), WORD_SIZE);
      time += L2_READ_TIME;
    }

    if (mode == MODE_WRITE) { // write data from cache line
      memcpy(&(cache.linesL2[index].Data[offset]), data, WORD_SIZE);
      time += L2_WRITE_TIME;
      cache.linesL2[index].Dirty = 1;
    }
  }

  else{ 
    if (cache.linesL2[index].Dirty) { // line has dirty block
      accessDRAM(cache.linesL2[index].Tag * (L2_SIZE / BLOCK_SIZE) * BLOCK_SIZE + index * BLOCK_SIZE, cache.linesL2[index].Data, MODE_WRITE); // then write back old block
      cache.linesL2[index].Data[0] = 0;
      cache.linesL2[index].Data[WORD_SIZE] = 0;
    }

    accessDRAM(address - offset, cache.linesL2[index].Data, MODE_READ); // get new block from DRAM 

    if (mode == MODE_READ) {
      memcpy(data, &(cache.linesL2[index].Data[offset]), WORD_SIZE);
      time += L2_READ_TIME;
      cache.linesL2[index].Dirty = 0; 
      cache.linesL2[index].Valid = 1;
      cache.linesL2[index].Tag = tag;
    }
    if (mode == MODE_WRITE) {
      memcpy(&(cache.linesL2[index].Data[offset]), data, WORD_SIZE);
      time += L2_WRITE_TIME;
      cache.linesL2[index].Dirty = 1;
      cache.linesL2[index].Valid = 1;
      cache.linesL2[index].Tag = tag;
    }
  } // if miss, then replaced with the correct block
}

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, Tag, offset;
  //uint8_t TempBlock[BLOCK_SIZE];

  Tag = address >> (OFFSET_SIZE + L1_INDEX_SIZE); // Why do I do this?
  index = indexExtractor(address, L1_INDEX_SIZE, OFFSET_SIZE);
  offset = offsetExtractor(address, OFFSET_SIZE);

  //uint32_t MemAddress;
  //MemAddress = address >> (OFFSET_SIZE); // again this....!
  //MemAddress = MemAddress << (OFFSET_SIZE); // address of the block in memory (address - offset)

  /* access Cache*/

  if (cache.linesL1[index].Valid && cache.linesL1[index].Tag == Tag) {    
    if (mode == MODE_READ) {    // read data from cache line
      memcpy(data, &(cache.linesL1[index].Data[offset]), WORD_SIZE);
      time += L1_READ_TIME;
    }

    if (mode == MODE_WRITE) { // write data from cache line
      memcpy(&(cache.linesL1[index].Data[offset]), data, WORD_SIZE);
      time += L1_WRITE_TIME;
      cache.linesL1[index].Dirty = 1;
    }
  }

  else{ 
    if (cache.linesL1[index].Dirty) { // line has dirty block
      accessL2(cache.linesL1[index].Tag * (L1_SIZE / BLOCK_SIZE) * BLOCK_SIZE + index * BLOCK_SIZE, cache.linesL1[index].Data, MODE_WRITE); // then write back old block
      cache.linesL1[index].Data[0] = 0;
      cache.linesL1[index].Data[WORD_SIZE] = 0;
    }

    accessL2(address - offset, cache.linesL1[index].Data, MODE_READ); // get new block from DRAM 

    if (mode == MODE_READ) {
      memcpy(data, &(cache.linesL1[index].Data[offset]), WORD_SIZE);
      time += L1_READ_TIME;
      cache.linesL1[index].Dirty = 0; 
      cache.linesL1[index].Valid = 1;
      cache.linesL1[index].Tag = Tag;
    }
    if (mode == MODE_WRITE) {
      memcpy(&(cache.linesL1[index].Data[offset]), data, WORD_SIZE);
      time += L1_WRITE_TIME;
      cache.linesL1[index].Dirty = 1;
      cache.linesL1[index].Valid = 1;
      cache.linesL1[index].Tag = Tag;
    }
  } // if miss, then replaced with the correct block
}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
