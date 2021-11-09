/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define MAXLIST 10

static const int DEBUG = 0;
static void* segList[MAXLIST];  // point to size block
static const int MinimalBlockSize = 16; // 4 words

static int min(int a, int b) {
  return a < b ? a : b;
}

static int max(int a, int b) {
  return a > b ? a : b;
}

static int getSegIndex(int size) {
  if(size < MinimalBlockSize) return -1;
  for(int i = 0; i < MAXLIST; i++) {
    if(size < (1 << (i + 4))) return i;
  }
  return 9;
}

static int getTopSize(void *block) {
  return *(size_t *)block & ~0x7;
}

static int isLastAllocated(void *block) {
  return !(*(size_t *)block & 2);
}

static int isThisAllocated(void *block) {
  return *(size_t *)block & 1;
}

static int isNextAllocated(void *block) {
  return *(size_t *)(block + getTopSize(block)) & 1;
}

static void *getBottomPtr(void *block) {
  size_t size = getTopSize(block);
  return (size_t *)(block + size) - 1;
}

static void *getNextPtr(void *block) {
  return (size_t *)block + 1;
}

static void *getNext(void *block) {
  return *(size_t **)getNextPtr(block);
}

static void *getPrePtr(void *block) {
  return (size_t *)block + 2;
}

static void *getPre(void *block) {
  return *(size_t **)getPrePtr(block);
}

/**
 * Allocated block only
 */
static void *getAreaStartPtr(void *block) {
  return (size_t *)block + 1;
}

static void *getBlockPtrFromSegList(int size) {
  int index = getSegIndex(size);
  void *cur = segList[index];
  while(cur != NULL) {
    size_t _size = getTopSize(cur);
    if(_size == size) break;
    if(_size >= size + MinimalBlockSize) break;
    cur = getNext(cur);
  }
  if(cur == NULL) {
    for(int i = MAXLIST - 1; i > index; i--) {
      cur = segList[i];
      while(cur != NULL) {
        size_t _size = getTopSize(cur);
        if(_size == size) break;
        if(_size >= size + MinimalBlockSize) break;
        cur = getNext(cur);
      }
      if(cur != NULL) break;
    }
  }
  return cur;
}

static void addBlockToList(void *block) {
  size_t blockSize = getTopSize(block);
  int blockIndex = getSegIndex(blockSize);
  *(size_t **)getPrePtr(block) = NULL;
  if(segList[blockIndex] == block) return;
  *(size_t **)getNextPtr(block) = segList[blockIndex];
  if(segList[blockIndex] != NULL)
    *(size_t **)getPrePtr(segList[blockIndex]) = block;
  segList[blockIndex] = block;
}

static void removeBlockFromList(void *block) {
  size_t blockSize = getTopSize(block);
  int blockIndex = getSegIndex(blockSize);
  void *pre = getPre(block);
  void *next = getNext(block);
  if(pre == NULL) { // first elem
    segList[blockIndex] = next;
    if(next != NULL)
      *(size_t **)getPrePtr(next) = NULL;
  } else if(next == NULL) { // last elem
    *(size_t **)getNextPtr(pre) = NULL;
  } else {
    *(size_t **)getNextPtr(pre) = next;
    *(size_t **)getPrePtr(next) = pre;
  }
  *(size_t **)getNextPtr(block) = NULL;
  *(size_t **)getPrePtr(block) = NULL;
}

static void setBlockToFree(void *block, size_t size) {
  *(size_t *)block = size;
  *(size_t **)getPrePtr(block) = NULL;
  *(size_t **)getNextPtr(block) = NULL;
  *(size_t *)getBottomPtr(block) = size;
}

static void setBlockPreFreed(void *block) {
  *(size_t *)block |= 2;
}

static void setBlockPreAllocated(void *block) {
  *(size_t *)block &= ~2;
}

static void setBlockToAllocated(void *block, size_t size, int isLastFreed) {
  *(size_t *)block = size | 1 | ((!!isLastFreed) << 1);
}

static void *allocMoreMem(size_t size) {
  void *top = mem_sbrk(size);
  *((size_t *)(top + size) - 1) = 1;
  size_t oldSign = *((size_t *)top - 1);
  *((size_t *)top - 1) = size | (oldSign & 0x6) | 1;
  return top;
}

static void printMemInfo(void) {
  printf("Print memory map:\n");
  size_t *memHi = mem_heap_hi();
  for(size_t *ptr = mem_heap_lo(); ptr <= memHi; ptr++) {
    printf("%p: %x\n", ptr, *ptr);
  }
  printf("Memory map end\n");
}

static void printBlockInfo(void *block, int source, int print) {
  if(block >= mem_heap_hi()) {
    printf("From line %d: ", source);
    printf("block overflow\n");
    exit(1);
  }
  int allocated = isThisAllocated(block);
  size_t size = getTopSize(block);
  if(allocated) {
    if(print) {
      printf("From line %d: ", source);
      printf("%p:\n", block);
      printf("\tAllocated: yes\n\tSize: %u\n", size);
    }
    void *bottom = getBottomPtr(block);
    if(bottom >= mem_heap_hi()) {
      if(!print) {
        printf("From line %d: ", source);
        printf("%p:\n", block);
        printf("\tAllocated: yes\n\tSize: %u\n", size);
      }
      printf("\tBlock overflow\nExit\n");
      printMemInfo();
      exit(1);
    }
    if(*((size_t *)bottom + 1) & 2) {
      if(!print) {
        printf("From line %d: ", source);
        printf("%p:\n", block);
        printf("\tAllocated: yes\n\tSize: %u\n", size);
      }
      printf("\tNext block free tag error\nExit\n");
      printMemInfo();
      exit(1);
    }

  } else {
    if(print) {
      printf("From line %d: ", source);
      printf("%p:\n", block);
      printf("\tAllocated: no\n\tSize: %u\n", size);
    }
    void *bottom = getBottomPtr(block);
    void *memHi = mem_heap_hi();
    if(bottom >= (void *)((size_t *)memHi - 1)) {
      if(!print) {
        printf("From line %d: ", source);
        printf("%p:\n", block);
        printf("\tAllocated: no\n\tSize: %u\n", size);
      }
      printf("\tBlock overflow\nExit\n");
      printMemInfo();
      exit(1);
    }
    if(*(size_t *)bottom != *(size_t *)block) {
      if(!print) {
        printf("From line %d: ", source);
        printf("%p:\n", block);
        printf("\tAllocated: no\n\tSize: %u\n", size);
      }
      printf("\tBlock format error\nExit\n");
      printMemInfo();
      exit(1);
    }
    if((*((size_t *)bottom + 1) & 2) == 0) {
      if(!print) {
        printf("From line %d: ", source);
        printf("%p:\n", block);
        printf("\tAllocated: no\n\tSize: %u\n", size);
      }
      printf("\tNext block free tag error\nExit\n");
      printMemInfo();
      exit(1);
    }

  }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  for(int i = 0; i < MAXLIST; i++) segList[i] = NULL;
  mem_sbrk(ALIGN(8));
  *(size_t *)mem_heap_lo() = 1;
  *((size_t *)mem_heap_lo() + 1) = 1;
  return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
  int newSize = ALIGN(max(size + SIZE_T_SIZE, MinimalBlockSize));
  void *cur = getBlockPtrFromSegList(newSize);
  if(cur == NULL) {
    void *new = allocMoreMem(newSize);
    //printBlockInfo((size_t *)new - 1, 273, DEBUG);
    return new;
  }

  removeBlockFromList(cur);
  int blockSize = getTopSize(cur);

  if(blockSize == newSize) {

    setBlockToAllocated(cur, blockSize, 0);
    setBlockPreAllocated(cur + blockSize);
    //printBlockInfo(cur, 294, DEBUG);
    return getAreaStartPtr(cur);

  } else if(blockSize < newSize) {

    return NULL;

  } else {

    setBlockToAllocated(cur, newSize, 0);

    void *new = cur + newSize;

    setBlockToFree(new, blockSize - newSize);
    addBlockToList(new);
    //printBlockInfo(cur, 299, DEBUG);
    return getAreaStartPtr(cur);
  }
  return NULL;
}

/**
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  ptr = (size_t *)ptr - 1;
  //printBlockInfo(ptr, 313, DEBUG);
  int isPre = isLastAllocated(ptr);
  int isThis = isThisAllocated(ptr);
  int isNext = isNextAllocated(ptr);
  int blockSize = getTopSize(ptr);
  if(!isThis) return;
  if(isPre && isNext) {

    setBlockPreFreed(ptr + blockSize);
    setBlockToFree(ptr, blockSize);
    addBlockToList(ptr);
    //printBlockInfo(ptr, 324, DEBUG);

  } else if(isPre && !isNext) {

    void *next = ptr + blockSize;
    blockSize += getTopSize(next);

    setBlockPreFreed(ptr + blockSize);
    removeBlockFromList(next);
    setBlockToFree(ptr, blockSize);
    addBlockToList(ptr);
    //printBlockInfo(ptr, 335, DEBUG);

  } else if(!isPre && isNext) {

    void *pre = ptr - (*((size_t *)ptr - 1) & ~0x7);
    blockSize += getTopSize(pre);

    setBlockPreFreed(pre + blockSize);
    removeBlockFromList(pre);
    setBlockToFree(pre, blockSize);
    addBlockToList(pre);
    //printBlockInfo(pre, 346, DEBUG);

  } else {

    void *pre = ptr - (*((size_t *)ptr - 1) & ~0x7);
    void *next = ptr + blockSize;
    blockSize += getTopSize(pre) + getTopSize(next);

    setBlockPreFreed(pre + blockSize);
    removeBlockFromList(pre);
    removeBlockFromList(next);
    setBlockToFree(pre, blockSize);
    addBlockToList(pre);
    //printBlockInfo(pre, 359, DEBUG);

  }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
  if(ptr == NULL) return mm_malloc(size);
  if(size == 0) {
    mm_free(ptr);
    return NULL;
  }
  void *newPtr = mm_malloc(size);
  size_t minSize = min(size, getTopSize((size_t *)ptr - 1) - 4);
  memcpy(newPtr, ptr, minSize);
  mm_free(ptr);
  return newPtr;
}












