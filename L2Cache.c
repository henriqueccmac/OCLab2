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

void initCache() {    
  for (int i = 0; i < L1_LINES; i++){
      cache.linesL1[i].Valid = 0;
      cache.linesL1[i].Dirty = 0;
      cache.linesL1[i].Tag = 0;
      cache.init = 1;
      for (int j=0; j<BLOCK_SIZE; j+=WORD_SIZE)
        cache.line[i].Data[j] = 0;
    }

    for (int i = 0; i < L2_LINES; i++){
      cache.linesL2[i].Valid = 0;
      cache.linesL2[i].Dirty = 0;
      cache.linesL2[i].Tag = 0;
      cache.init = 1;
      for (int j=0; j<BLOCK_SIZE; j+=WORD_SIZE)
        cache.line[i].Data[j] = 0;
    }
  }

void accessL2(int address, unsigned char *data, int mode) {

  unsigned int tag, index_n, offset;
  
  tag = address / ((L2_SIZE / (BLOCK_SIZE * ASSOCIATIVITY_L2)) * BLOCK_SIZE);
  index_n = (address / BLOCK_SIZE) % (L2_SIZE / (BLOCK_SIZE * ASSOCIATIVITY_L2));
  offset = address % BLOCK_SIZE;

  int found = 0;
  int i = 0;
  while (i < ASSOCIATIVITY_L2 && !found) {
    if (cache.L2.Lines[index_n][i].Valid && cache.L2.Lines[index_n][i].Tag == tag) {
      found = 1;
    } else {
      i++;
    }
  }

  if (!found) {
    i = 0;
    while (i < ASSOCIATIVITY_L2 && cache.L2.Lines[index_n][i].Valid) {
      i++;
    }
    if (i == ASSOCIATIVITY_L2) {
      i = 0;
      unsigned int min = cache.L2.Lines[index_n][0].Time;
      for (int j = 1; j < ASSOCIATIVITY_L2; j++) {
        if (cache.L2.Lines[index_n][j].Time < min) {
          min = cache.L2.Lines[index_n][j].Time;
          i = j;
        }
      }
    }
    if (cache.L2.Lines[index_n][i].Dirty) {
      accessDRAM(cache.L2.Lines[index_n][i].Tag * (L2_SIZE / (BLOCK_SIZE * ASSOCIATIVITY_L2)) * BLOCK_SIZE + index_n * BLOCK_SIZE, cache.L2.Lines[index_n][i].Data, MODE_WRITE);
      cache.L2.Lines[index_n][i].Data[0] = 0;
      cache.L2.Lines[index_n][i].Data[WORD_SIZE] = 0;
    }

    accessDRAM(address - offset, cache.L2.Lines[index_n][i].Data, MODE_READ);

    cache.L2.Lines[index_n][i].Valid = 1;
    cache.L2.Lines[index_n][i].Dirty = 0;
    cache.L2.Lines[index_n][i].Tag = tag;
    cache.L2.Lines[index_n][i].Time = time;

    if (mode == MODE_READ) {
      memcpy(data, &(cache.L2.Lines[index_n][i].Data), BLOCK_SIZE);
      time += L2_READ_TIME;
    }
  } else {
    if (mode == MODE_READ) {
      memcpy(data, &(cache.L2.Lines[index_n][i].Data), BLOCK_SIZE);
      time += L1_READ_TIME;
      cache.L2.Lines[index_n][i].Time = time;
    }
    if (mode == MODE_WRITE) {
      memcpy(&(cache.L2.Lines[index_n][i].Data), data, BLOCK_SIZE);
      time += L1_WRITE_TIME;
      cache.L2.Lines[index_n][i].Dirty = 1;
      cache.L2.Lines[index_n][i].Time = time;
    }
  }
  
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
