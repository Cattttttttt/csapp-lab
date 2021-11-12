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

#define MAXLIST 20

static const int DEBUG = 0;
static void* segList[MAXLIST];  // point to size block
static const int MinimalBlockSize = 20; // 5 words

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

static void *getLeftSonPtr(void *block) {
  return (size_t *)block + 1;
}

static void *getRightSonPtr(void *block) {
  return (size_t *)block + 2;
}

static void *getChildPtr(void *block, int index) {
  if(index < 0 || index > 1) return NULL;
  return index == 0 ? getLeftSonPtr(block) : getRightSonPtr(block);
}

static void *getChild(void *block, int index) {
  return *(size_t **)getChildPtr(block, index);
}

static void *getFatherPtr(void *block) {
  return (size_t *)block + 3;
}

static void *getFather(void *block) {
  return *(size_t **)getFatherPtr(block);
}

static int compare(void *blockA, void *blockB) {
  int sizeA = getTopSize(blockA);
  int sizeB = getTopSize(blockB);
  return sizeA == sizeB ? blockA < blockB : sizeA < sizeB;
}

static int compareInt(void *block, int inter) {
  return getTopSize(block) <= inter;
}

static int id(void *node) {
  void *father = getFather(node);
  if(father == NULL) return -1;
  if(getChild(father, 0) == node) return 0;
  if(getChild(father, 1) == node) return 1;
  return -1;
}

static void rotate(void *node, int rootIndex) {
  if(node == NULL) return;
  int res = id(node);
  if(res == -1) return;
  void *father = getFather(node);
  if(getChild(node, !res) != NULL) {
    *(size_t **)getFatherPtr(getChild(node, !res)) = father;
  }
  *(size_t **)getChildPtr(father, res) = getChild(node, !res);
  *(size_t **)getFatherPtr(node) = getFather(father);
  *(size_t **)getChildPtr(node, !res) = father;
  if(getFather(father) != NULL) {
    int _res = id(father);
    *(size_t **)getChildPtr(getFather(father), _res) = node;
  } else {
    segList[rootIndex] = node;
  }
  *(size_t **)getFatherPtr(father) = node;
}

static void splay(void *node, int rootIndex) {
  if(node == NULL) return;
  while(getFather(node) != NULL) {
    void *father = getFather(node);
    if(getFather(father) == NULL) {
      rotate(node, rootIndex);
      continue;
    }
    if(id(node) == id(father)) {
      rotate(father, rootIndex);
      rotate(node, rootIndex);
      continue;
    }
    if(id(node) != id(father)) {
      rotate(node, rootIndex);
      rotate(node, rootIndex);
      continue;
    }
    break;
  }
}

static void insert(void *node, int rootIndex) {
  if(segList[rootIndex] == NULL) {
    segList[rootIndex] = node;
    return;
  }
  void *cur = segList[rootIndex];
  while(cur != NULL) {
    int res = compare(cur, node);
    if(getChild(cur, res) == NULL) {
      *(size_t **)getFatherPtr(node) = cur;
      *(size_t **)getChildPtr(cur, res) = node;
      splay(node, rootIndex);
      break;
    }
    cur = getChild(cur, res);
  }
}

static void *find(int length, int rootIndex) {
  void *cur = segList[rootIndex];
  while(cur != NULL) {
    if(getTopSize(cur) == length) {
      splay(cur, rootIndex);
      return cur;
    }
    cur = getChild(cur, compareInt(cur, length));
  }
  return NULL;
}

static void *findLarger(int length, int rootIndex) {
  length += MinimalBlockSize;
  void *cur = segList[rootIndex];
  while(cur != NULL) {
    void *left = getChild(cur, 0);
    if(getTopSize(cur) == length - MinimalBlockSize) {
      splay(cur, rootIndex);
      return cur;
    }
    if(getTopSize(cur) < length) {
      cur = getChild(cur, 1);
    } else {
      if(left != NULL && getTopSize(left) >= length) {
        cur = left;
      } else {
        splay(cur, rootIndex);
        return cur;
      }
    }
  }
  return NULL;
}

static void delete(void *block, int rootIndex) {
  splay(block, rootIndex);
  if(getChild(block, 0) == NULL) {
    segList[rootIndex] = getChild(block, 1);
    if(segList[rootIndex] != NULL)
      *(size_t **)getFatherPtr(segList[rootIndex]) = NULL;
    return;
  }
  if(getChild(block, 1) == NULL) {
    segList[rootIndex] = getChild(block, 0);
    *(size_t **)getFatherPtr(segList[rootIndex]) = NULL;
    return;
  }
  void *cur = getChild(block, 0);
  void *right = getChild(block, 1);
  while(getChild(cur, 1) != NULL) cur = getChild(cur, 1);
  splay(cur, rootIndex);
  *(size_t **)getChildPtr(cur, 1) = right;
  *(size_t **)getFatherPtr(right) = cur;
}

/**
 * Allocated block only
 */
static void *getAreaStartPtr(void *block) {
  return (size_t *)block + 1;
}

static void *getBlockPtrFromSegList(int size) {
  int index = getSegIndex(size);
  void *cur = find(size, index);
  if(cur == NULL) {
    for(int i = MAXLIST - 1; i >= index; i--) {
      cur = findLarger(size, i);
      if(cur != NULL) break;
    }
  }
  return cur;
}

static void addBlockToList(void *block) {
  size_t blockSize = getTopSize(block);
  int blockIndex = getSegIndex(blockSize);
  insert(block, blockIndex);
}

static void removeBlockFromList(void *block) {
  size_t blockSize = getTopSize(block);
  int blockIndex = getSegIndex(blockSize);
  delete(block, blockIndex);
}

static void setBlockToFree(void *block, size_t size) {
  *(size_t *)block = size;
  *(size_t **)getChildPtr(block, 0) = NULL;
  *(size_t **)getChildPtr(block, 1) = NULL;
  *(size_t **)getFatherPtr(block) = NULL;
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

static void printTree(void *block) {
  if(block == NULL) {
    for(int i = 0; i < MAXLIST; i++) {
      if(segList[i] == NULL) continue;
      printTree(segList[i]);
      fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
    return;
  }
  if(getChild(block, 0) != NULL) printTree(getChild(block, 0));
  printBlockInfo(block, 0, 0);
  if(getChild(block, 1) != NULL) printTree(getChild(block, 1));
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
void *mm_malloc(size_t size) {
  int newSize = ALIGN(max(size + SIZE_T_SIZE, MinimalBlockSize));
  void *cur = getBlockPtrFromSegList(newSize);
  if(cur == NULL) {
    void *new = allocMoreMem(newSize);
    return new;
  }

  removeBlockFromList(cur);
  int blockSize = getTopSize(cur);

  if(blockSize == newSize) {

    setBlockToAllocated(cur, blockSize, 0);
    setBlockPreAllocated(cur + blockSize);
    return getAreaStartPtr(cur);

  } else if (blockSize < newSize) {

    return NULL;

  } else {

    setBlockToAllocated(cur, newSize, 0);

    void *new = cur + newSize;

    setBlockToFree(new, blockSize - newSize);
    addBlockToList(new);
    return getAreaStartPtr(cur);

  }
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
  size_t oldSize = getTopSize(ptr);
  if(oldSize == size) return ptr;
  if(oldSize > size + MinimalBlockSize) {
    int oldSign = *(size_t *)ptr & 0x7;
    *(size_t *)ptr = size | oldSign | 1;
    setBlockToFree(ptr + size, oldSign - size);
    addBlockToList(ptr);
    return ptr;
  }
  int isNextFreed = !isNextAllocated(ptr);
  if(isNextFreed) {
    void *next = ptr + oldSize;
    int oldSign = *(size_t *)ptr & 0x7;
    int newBlockSize = oldSize + getTopSize(next);
    if(size == newBlockSize) {
      removeBlockFromList(next);
      *(size_t *)ptr = size | oldSign | 1;
      setBlockPreAllocated(ptr + size);
      return ptr;
    }
    if(size + MinimalBlockSize <= newBlockSize) {
      removeBlockFromList(next);
      *(size_t *)ptr = size | oldSign | 1;
      setBlockToFree(ptr + size, newBlockSize - size);
      addBlockToList(ptr + size);
      return ptr;
    }
  }
  void *newPtr = mm_malloc(size);
  size_t minSize = min(size, getTopSize((size_t *)ptr - 1) - 4);
  memcpy(newPtr, ptr, minSize);
  mm_free(ptr);
  return newPtr;
}












