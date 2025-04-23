#include "univ/encrypt.h"
#include "univ/str.h"
#include "univ/math.h"
#include "univ/time.h"

char EmptyIV[12] = {0};

static void InitState(u32 state[16], u8 key[32], u32 blockCounter, u8 iv[12])
{
  Copy("expand 32-byte k", state, sizeof(u32)*4);
  Copy(key, state + 4, sizeof(u32)*8);
  state[12] = IsBigEndian() ? ByteSwap(blockCounter) : blockCounter;
  Copy(iv, state + 13, sizeof(u32)*3);
}

static void QuarterRound(u32 *a, u32 *b, u32 *c, u32 *d)
{
  *a += *b;
  *d ^= *a;
  *d = RotL(*d, 16);
  *c += *d;
  *b ^= *c;
  *b = RotL(*b, 12);
  *a += *b;
  *d ^= *a;
  *d = RotL(*d, 8);
  *c += *d;
  *b ^= *c;
  *b = RotL(*b, 7);
}

static void UpdateState(u32 state[16], u32 rounds)
{
  u32 i;
  u32 workingState[16];
  Copy(state, workingState, sizeof(workingState));
  for (i = 0; i < rounds/2; i++) {
    QuarterRound(&workingState[0], &workingState[4], &workingState[8], &workingState[12]);
    QuarterRound(&workingState[1], &workingState[5], &workingState[9], &workingState[13]);
    QuarterRound(&workingState[2], &workingState[6], &workingState[10], &workingState[14]);
    QuarterRound(&workingState[3], &workingState[7], &workingState[11], &workingState[15]);
    QuarterRound(&workingState[0], &workingState[5], &workingState[10], &workingState[15]);
    QuarterRound(&workingState[1], &workingState[6], &workingState[11], &workingState[12]);
    QuarterRound(&workingState[2], &workingState[7], &workingState[8], &workingState[13]);
    QuarterRound(&workingState[3], &workingState[4], &workingState[9], &workingState[14]);
  }

  for (i = 0; i < 16; i++) {
    state[i] += workingState[i];
  }
}

void ChaCha(u8 *data, u32 len, u8 *buf, u8 key[32], u8 iv[12], u32 blockCounter, u32 rounds)
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

u8 *Encrypt(void *data, u32 len, char *password)
{
  u32 iv[3];
  u32 i;
  u8 *encrypted = malloc(len + sizeof(iv));
  u8 key[32];

  SeedRandom(Time());
  for (i = 0; i < ArrayCount(iv); i++) {
    iv[i] = Random();
  }
  Copy(iv, encrypted, sizeof(iv));

  Sha256(password, StrLen(password), key);
  ChaCha(data, len, encrypted+sizeof(iv), key, (u8*)iv, 0, 8);
  return encrypted;
}

void *Decrypt(u8 *data, u32 len, char *password)
{
  u8 *decrypted = malloc(len - 12);
  u8 key[32];
  Sha256(password, StrLen(password), key);
  ChaCha(data+12, len-12, decrypted, key, data, 0, 8);
  return decrypted;
}

static u32 Sha256InitialHash[] = {
  0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};
static u32 k[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void Sha256Step(u32 w[64], u32 h[8])
{
  u32 j, a[8];

  if (!IsBigEndian()) {
    for (j = 0; j < 16; j++) {
      w[j] = ByteSwap(w[j]);
    }
  }

  for (j = 16; j < 64; j++) {
    u32 s0 = RotR(w[j-15], 7) ^ RotR(w[j-15], 18) ^ (w[j-15] >> 3);
    u32 s1 = RotR(w[j-2], 17) ^ RotR(w[j-2], 19) ^ (w[j-2] >> 10);
    w[j] = w[j-16] + s0 + w[j-7] + s1;
  }

  for (j = 0; j < 8; j++) a[j] = h[j];

  for (j = 0; j < 64; j++) {
    u32 s1 = RotR(a[4], 6) ^ RotR(a[4], 11) ^ RotR(a[4], 25);
    u32 ch = (a[4] & a[5]) ^ ((~a[4]) & a[6]);
    u32 temp1 = a[7] + s1 + ch + k[j] + w[j];
    u32 s0 = RotR(a[0], 2) ^ RotR(a[0], 13) ^ RotR(a[0], 22);
    u32 maj = (a[0] & a[1]) ^ (a[0] & a[2]) ^ (a[1] & a[2]);
    u32 temp2 = s0 + maj;
    a[7] = a[6];
    a[6] = a[5];
    a[5] = a[4];
    a[4] = a[3] + temp1;
    a[3] = a[2];
    a[2] = a[1];
    a[1] = a[0];
    a[0] = temp1 + temp2;
  }

  for (j = 0; j < 8; j++) {
    h[j] += a[j];
  }
}

static void Sha256Rest(void *data, u32 totalLen, u32 startChunk, u32 hash[8])
{
  u8 *src = (u8*)data;
  bool bigEndian = IsBigEndian();
  u32 h[8];

#define chunkBits   512
#define chunkBytes  64
#define lengthBits  64
  u8 pad[72] = {0};
  u8 *pp = pad;
  u64 L = totalLen*8;
  u32 padBits = chunkBits - (L + 1 + lengthBits) % chunkBits;
  u32 padBytes = (padBits + 1) / 8;
  u32 numChunks = (L + 1 + lengthBits + padBits) / chunkBits - startChunk;
  u32 dataChunks = Align(L, chunkBits) / chunkBits - startChunk;
  u32 extraBytes = (L % chunkBits) / 8;
  u32 i;

  Copy(hash, h, sizeof(h));

  pad[0] = 0x80;
  pad[padBytes+0] = (L >> 56) & 0xFF;
  pad[padBytes+1] = (L >> 48) & 0xFF;
  pad[padBytes+2] = (L >> 40) & 0xFF;
  pad[padBytes+3] = (L >> 32) & 0xFF;
  pad[padBytes+4] = (L >> 24) & 0xFF;
  pad[padBytes+5] = (L >> 16) & 0xFF;
  pad[padBytes+6] = (L >>  8) & 0xFF;
  pad[padBytes+7] = (L >>  0) & 0xFF;
  padBytes += 8;

  for (i = 0; i < numChunks; i++) {
    u32 w[64] = {0};

    if (i == dataChunks - 1) {
      Copy(src+i*chunkBytes, w, extraBytes);
      Copy(pp, ((u8*)w)+extraBytes, chunkBytes-extraBytes);
      pp += chunkBytes-extraBytes;
      padBytes -= chunkBytes-extraBytes;
    } else if (i == numChunks - 1) {
      Copy(pp, w, padBytes);
    } else {
      Copy(src + i*chunkBytes, w, 16*sizeof(u32));
    }

    Sha256Step(w, h);
  }

  if (!bigEndian) {
    for (i = 0; i < 8; i++) {
      h[i] = ByteSwap(h[i]);
    }
  }

  Copy(h, hash, sizeof(h));
}

void Sha256(void *data, u32 len, u8 result[32])
{
  Copy(Sha256InitialHash, result, sizeof(Sha256InitialHash));
  Sha256Rest(data, len, 0, (u32*)result);
}

void HMAC(void *data, u32 len, u8 *key, u32 keyLen, u8 result[32])
{
  u8 blockKey[64] = {0};
  u8 outer[96];
  u8 inner[256];
  u8 *innerHash = outer+64;
  u32 i;

  if (keyLen > sizeof(blockKey)) {
    Sha256(key, keyLen, blockKey);
  } else {
    Copy(key, blockKey, keyLen);
  }

  for (i = 0; i < sizeof(blockKey); i++) {
    outer[i] = blockKey[i] ^ 0x5C;
    inner[i] = blockKey[i] ^ 0x36;
  }

  Copy(Sha256InitialHash, innerHash, sizeof(Sha256InitialHash));
  Sha256Step((u32*)inner, (u32*)innerHash);
  Sha256Rest(data, len+64, 1, (u32*)innerHash);
  Sha256(outer, 96, result);
}

void PrintHash(u8 hash[32])
{
  u32 i;
  for (i = 0; i < 32; i++) {
    printf("%02X", hash[i]);
  }
  printf("\n");
}
