#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

const char helpInfo[] = "Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\nOptions:\n  -h         Prnit this help message.\n  -v         Optional verbose flag.\n  -s <num>   Number of set index bits.\n  -E <num>   Number of lines per set.\n  -b <num>   Number of block offset bits.\n  -t <file>  Trace file.\n\nExamples:\n  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";

int verboseTag = 0;
int s = 0, E = 0, b = 0, timestamp = 1;
int bitS = 0, bitB = 0;
int hitCnt = 0, missCnt = 0, evicCnt = 0;
char *t = NULL;
FILE *fp = NULL;

typedef struct CacheLine {
  int isValid;
  int tag;
} CacheLine;

CacheLine **cache = NULL;

typedef struct CacheLineNode {
  CacheLine* node;
  struct CacheLineNode* next;
  struct CacheLineNode* pre;
  int time;
} CacheLineNode;

CacheLineNode **cacheNode = NULL;

int initial(int argc, char **argv) {
  int _tmp;
  while((_tmp = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
    switch(_tmp) {
      case 'h':
        fprintf(stdout, helpInfo, argv[0]);
        return 0;
      case 'v':
        verboseTag = 1;
        break;
      case 's':
        s = atoi(optarg);
        break;
      case 'E':
        E = atoi(optarg);
        break;
      case 'b':
        b = atoi(optarg);
        break;
      case 't':
        t = optarg;
        break;
      case '?':
        fprintf(stdout, helpInfo, argv[0]);
        return 1;
      default:
        return 1;
    }
  }
  if(t == NULL || s == 0 || E == 0 || b == 0) {
    fprintf(stdout, "%s: Missing required command line argument\n", argv[0]);
    fprintf(stdout, helpInfo, argv[0]);
    return 1;
  }
  bitS = s;
  bitB = b;
  s = 1 << s;
  b = 1 << b;
  cache = calloc(s, sizeof(CacheLine *));
  cacheNode = calloc(s, sizeof(CacheLineNode *));
  if(cache == NULL) return 1;
  for(int i = 0; i < s; i ++) {
    cache[i] = calloc(E, sizeof(CacheLine));
    cacheNode[i] = calloc(1, sizeof(CacheLineNode));
    CacheLineNode *cur = cacheNode[i];
    CacheLineNode *pre = NULL;
    for(int j = 0; j < E; j ++) {
      cur -> node = &cache[i][j];
      cur -> time = 0;
      cur -> pre = pre;
      if(j < E - 1)
        cur -> next = calloc(1, sizeof(CacheLineNode));
      else
        cur -> next = NULL;
      pre = cur;
      cur = cur -> next;
    }
    if(cacheNode[i] == NULL) {
      return 1;
    }
    if(cache[i] == NULL) {
      return 1;
    }
  }
  return 0;
}

int openFile() {
  fp = fopen(t, "r");
  if(fp == NULL) return 1;
  return 0;
}

int cacheUpd(int _s, int tag) {
  if(_s < 0 || _s >= s) return 1;
  CacheLineNode *target = cacheNode[_s];
  while(target != NULL) {
    if(target -> node -> tag == tag && target -> node -> isValid) break;
    target = target -> next;
  }
  CacheLineNode *end = cacheNode[_s];
  while(end -> next != NULL) end = end -> next;
  if(target) {
    if(verboseTag) fprintf(stdout, " hit");
    hitCnt ++;
    target -> time = timestamp;
    timestamp ++;
    if(end != cacheNode[_s] && target -> next != NULL) {
      if(target -> pre == NULL) {
        cacheNode[_s] = target -> next;
      } else {
        target -> pre -> next = target -> next;
      }
      target -> next -> pre = target -> pre;
      end -> next = target;
      target -> next = NULL;
      target -> pre = end;
    }
  } else {
    if(verboseTag) fprintf(stdout, " miss");
    missCnt ++;
    cacheNode[_s] -> time = timestamp;
    timestamp ++;
    evicCnt += cacheNode[_s] -> node -> isValid == 1;
    cacheNode[_s] -> node -> isValid = 1;
    cacheNode[_s] -> node -> tag = tag;
    if(cacheNode[_s] != end) {
      CacheLineNode *tmp = cacheNode[_s];
      cacheNode[_s] = tmp -> next;
      cacheNode[_s] -> pre = NULL;
      tmp -> pre = end;
      tmp -> pre -> next = tmp;
      tmp -> next = NULL;
    }
  }
  return 0;
}

void trim(char *str) {
  int len = strlen(str);
  char *begin = str, *end = str + len - 1;
  while(isspace(*begin) || isspace(*end)) {
    if(begin >= end) {
      *(str + 1) = '\0';
      return;
    }
    if(isspace(*begin)) begin += 1;
    if(isspace(*end)) {
      *end = '\0';
      end -= 1;
    }
  }
  int bias = begin - str;
  len = strlen(str);
  for(int i = 0; i < len - bias; i ++) {
    *(str + i) = *(str + bias + i);
  }
  *(end - bias + 1) = '\0';
}

int readLine(char *str) {
  char line[64];
  while(fgets(line, 64, fp) != NULL) {
    trim(line);
    if(line[0] == 'I') continue;
    if(verboseTag) fprintf(stdout, "%s", line);
    strcpy(str, line);
    return 0;
  }
  return -1;
}

char getOp(char *str) {
  if(str == NULL) return -1;
  return *str;
}

unsigned long long getNumber(char *str) {
  char *start = strchr(str, ' ');
  char *end = strchr(str, ',');
  if(start == NULL || end == NULL) return -1;
  char ans[64];
  strncpy(ans, start, end - start);
  unsigned long long ret = strtoull(ans, NULL, 16);
  return ret;
}

int process() {
  char line[64];
  while(readLine(line) != -1) {
    char op = getOp(line);
    unsigned long long address = getNumber(line);
    if(address == -1) return 1;
    int sMask = (1 << bitS) - 1;
    int setIndex = (address >> bitB) & sMask;
    int tag = address >> (bitB + bitS);
    switch(op) {
      case 'L': case 'S':
        cacheUpd(setIndex, tag);
        if(verboseTag) fprintf(stdout, "\n");
        break;
      case 'M':
        cacheUpd(setIndex, tag);
        cacheUpd(setIndex, tag);
        if(verboseTag) fprintf(stdout, "\n");
        break;
      default:
        return 1;
    }
  }
  return 0;
}

void clean() {
  if(fp) fclose(fp);
  if(cache) {
    for(int i = 0; i < s; i ++)
      if(cache[i]) {
        free(cache[i]);
        cache[i] = NULL;
      }
    free(cache);
    cache = NULL;
  }
  if(cacheNode) {
    for(int i = 0; i < s; i ++) {
      CacheLineNode* cur = cacheNode[i];
      while(cur -> next != NULL) {
        cur = cur -> next;
      }
      while(cur != NULL) {
        CacheLineNode* pre = cur -> pre;
        free(cur);
        cur = pre;
        if(cur != NULL) cur -> next = NULL;
      }
      cacheNode[i] = NULL;
    }
    free(cacheNode);
    cacheNode = NULL;
  }
}

int main(int argc, char **argv) {
  if(initial(argc, argv)) return 1;
  if(openFile()) return 1;
  if(process()) {
    clean();
    return 1;
  }
  printSummary(hitCnt, missCnt, evicCnt);
  clean();
  return 0;
}
