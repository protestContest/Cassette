#include "univ/encrypt.h"
#include "univ/str.h"
#include "univ/math.h"

char EmptyIV[12] = {0};

static void InitState(u32 state[16], char key[32], u32 block_counter, char *iv)
{
  Copy("expand 32-byte k", state, 16);
  Copy(key, state + 4, 32);
  Copy(&block_counter, state + 12, sizeof(block_counter));
  Copy(iv, state + 13, 12);
}

static void QuarterRound(u32 *a, u32 *b, u32 *c, u32 *d)
{
  *a += *b;
  *d ^= *a;
  *d = RotateLeft(*d, 16);
  *c += *d;
  *b ^= *c;
  *b = RotateLeft(*b, 12);
  *a += *b;
  *d ^= *a;
  *d = RotateLeft(*d, 8);
  *c += *d;
  *b ^= *c;
  *b = RotateLeft(*b, 7);
}

static void ColumnRound(u32 state[16])
{
  QuarterRound(&state[0], &state[4], &state[8], &state[12]);
  QuarterRound(&state[1], &state[5], &state[9], &state[13]);
  QuarterRound(&state[2], &state[6], &state[10], &state[14]);
  QuarterRound(&state[3], &state[7], &state[11], &state[15]);
}

static void DiagonalRound(u32 state[16])
{
  QuarterRound(&state[0], &state[5], &state[10], &state[15]);
  QuarterRound(&state[1], &state[6], &state[11], &state[12]);
  QuarterRound(&state[2], &state[7], &state[8], &state[13]);
  QuarterRound(&state[3], &state[4], &state[9], &state[14]);
}

static void UpdateState(u32 state[16], u32 rounds)
{
  u32 i;
  for (i = 0; i < rounds/2; i++) {
    ColumnRound(state);
    DiagonalRound(state);
  }
}

void ChaCha(void *data, u32 len, void *buf, char key[32], char iv[12], u32 blockCounter, u32 rounds)
{
  u32 state[16];
  u32 numBlocks = len / sizeof(state);
  u32 extra = len % sizeof(state);
  u32 i, j;
  InitState(state, key, blockCounter, iv);

  for (i = 0; i < numBlocks; i++) {
    UpdateState(state, rounds);
    for (j = 0; j < 16; j++) {
      u32 index = i*16 + j;
      ((u32*)buf)[index] = ((u32*)data)[index] ^ state[j];
    }
  }

  if (extra > 0) {
    UpdateState(state, rounds);
    for (j = 0; j < extra; j++) {
      u32 index = numBlocks*sizeof(state) + j;
      ((u8*)buf)[index] = ((u8*)data)[index] ^ ((u8*)state)[j];
    }
  }
}
