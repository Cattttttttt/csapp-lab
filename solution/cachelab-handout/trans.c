/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
  if(M == 32) { // 23 * 4 + 16 * 12 = 284 miss
    for(int i = 0; i < N; i += 8)
      for(int j = 0; j < M; j += 8) {
        if(i == j) { // 23 * 4 miss
          for(int k = i; k < i + 8 && k < N; k++) { // 23 miss total
            int a1 = A[k][j]; // miss, 8 miss total
            int a2 = A[k][j + 1]; // hit
            int a3 = A[k][j + 2]; // hit
            int a4 = A[k][j + 3]; // hit
            int a5 = A[k][j + 4]; // hit
            int a6 = A[k][j + 5]; // hit
            int a7 = A[k][j + 6]; // hit
            int a8 = A[k][j + 7]; // hit
            B[j][k] = a1; // 8 miss at first, then 1 miss per cycle, 15 miss total
            B[j + 1][k] = a2;
            B[j + 2][k] = a3;
            B[j + 3][k] = a4;
            B[j + 4][k] = a5;
            B[j + 5][k] = a6;
            B[j + 6][k] = a7;
            B[j + 7][k] = a8;
          }
        } else { // 16 * 12 miss
          for(int _i = i; _i < i + 8 && _i < N; _i ++)
            for(int _j = j; _j < j + 8 && _j < M; _j ++) {
              B[_j][_i] = A[_i][_j];
            }
        }
      }
  }
  if(M == 64) { // 568 + 536 = 1104
    for(int i = 0; i < N; i += 8)
      for(int j = 0; j < M; j += 8) {
        for(int _i = i; _i < i + 4 && _i < N; _i ++) { // 11 * 8 + 8 * 56 = 536
          if(i == j) {  // 8 + 3 = 11 miss total
            int a0 = A[_i][j];  // 1 miss per cycle
            int a1 = A[_i][j + 1];
            int a2 = A[_i][j + 2];
            int a3 = A[_i][j + 3];
            int a4 = A[_i][j + 4];
            int a5 = A[_i][j + 5];
            int a6 = A[_i][j + 6];
            int a7 = A[_i][j + 7];
            B[j][_i] = a0;  // 4 miss at first, then 1 miss per cycle
            B[j + 1][_i] = a1;
            B[j + 2][_i] = a2;
            B[j + 3][_i] = a3;
            B[j][_i + 4] = a4;
            B[j + 1][_i + 4] = a5;
            B[j + 2][_i + 4] = a6;
            B[j + 3][_i + 4] = a7;
          } else {  // 8 miss total
            for(int _j = 0; _j < 8 && _j + j < M; _j ++)
              B[j + (_j % 4)][_i + _j / 4 * 4] = A[_i][j + _j];
          }
        }
        /* B[0-3] in cache, A[0-3] in cache
         * only B[0-3] in cache when i == j
         * 8 * 56 + 15 * 8 = 568
         */
        for(int k = j; k < j + 4 && k < M; k ++) { // 8 miss or 15 miss when i == j
          int a0 = A[i + 4][k]; // 4 miss at first
          int a1 = A[i + 5][k]; // then 1 miss per k when i == j
          int a2 = A[i + 6][k];
          int a3 = A[i + 7][k];
          int a4 = A[i + 4][k + 4];
          int a5 = A[i + 5][k + 4];
          int a6 = A[i + 6][k + 4];
          int a7 = A[i + 7][k + 4];
          int tmp;
          tmp = B[k][i + 4]; B[k][i + 4] = a0; a0 = tmp;  // no miss
          tmp = B[k][i + 5]; B[k][i + 5] = a1; a1 = tmp;  // 1 miss per k when i == j
          tmp = B[k][i + 6]; B[k][i + 6] = a2; a2 = tmp;
          tmp = B[k][i + 7]; B[k][i + 7] = a3; a3 = tmp;
          B[k + 4][i] = a0; // 1 miss per k
          B[k + 4][i + 1] = a1;
          B[k + 4][i + 2] = a2;
          B[k + 4][i + 3] = a3;
          B[k + 4][i + 4] = a4;
          B[k + 4][i + 5] = a5;
          B[k + 4][i + 6] = a6;
          B[k + 4][i + 7] = a7;
        }
      }
  }
  /* 8 x 18 -> 1905
   * 8 x 16 -> 1932
   * 8 x 14 -> 1955
   * 8 x 12 -> 1978
   */
  if(M == 61) {
    for(int i = 0; i < N; i += 20)
      for(int j = 0; j < M; j += 8) {
        if(i == j) {
          for(int k = i; k < i + 20 && k < N; k++) {
            int a1 = A[k][j];
            int a2 = A[k][j + 1];
            int a3 = A[k][j + 2];
            int a4 = A[k][j + 3];
            int a5 = A[k][j + 4];
            int a6 = A[k][j + 5];
            int a7 = A[k][j + 6];
            int a8 = A[k][j + 7];
            B[j][k] = a1;
            B[j + 1][k] = a2;
            B[j + 2][k] = a3;
            B[j + 3][k] = a4;
            B[j + 4][k] = a5;
            B[j + 5][k] = a6;
            B[j + 6][k] = a7;
            B[j + 7][k] = a8;
          }
        } else {
          for(int _i = i; _i < i + 20 && _i < N; _i ++)
            for(int _j = j; _j < j + 8 && _j < M; _j ++) {
              B[_j][_i] = A[_i][_j];
            }
        }
      }
  }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

