#include "univ/keygen.h"
#include "univ/math.h"
#include "univ/str.h"

static char *FormatKey(char *key)
{
  key = realloc(key, 24);
  key[23] = 0;
  key[22] = key[19];
  key[21] = key[18];
  key[20] = key[17];
  key[19] = key[16];
  key[18] = key[15];
  key[17] = '-';
  key[16] = key[14];
  key[15] = key[13];
  key[14] = key[12];
  key[13] = key[11];
  key[12] = key[10];
  key[11] = '-';
  key[10] = key[9];
  key[9] = key[8];
  key[8] = key[7];
  key[7] = key[6];
  key[6] = key[5];
  key[5] = '-';
  return key;
}

static char *UnformatKey(char *key)
{
  char *newkey = malloc(StrLen(key) + 1);
  char *cur = newkey;
  while (*key) {
    if (*key != '-' && *key != ' ') {
      *cur = *key;
      cur++;
    }
    key++;
  }
  *cur = 0;
  return newkey;
}

char *GenKey(void *data)
{
  u8 bytes[13] = {0};
  u32 hash, i, chk = 0;
  if (data) {
    Copy(data, bytes, 8);
  } else {
    for (i = 0; i < 8; i++) bytes[i] = Random() & 0xFF;
  }
  hash = Hash(bytes, 8);
  WriteBE(hash, bytes+8);
  for (i = 0; i < 12; i++) chk = chk ^ bytes[i];
  bytes[12] = (chk & 0x0F) << 4;
  return FormatKey(Base32Encode(bytes, 13));
}

bool KeyValid(char *key)
{
  char *data;
  u32 i, storedHash, calcHash, storedChk, calcChk = 0;
  key = UnformatKey(key);
  if (StrLen(key) != 20) {
    free(key);
    return false;
  }
  data = Base32Decode(key, 20);
  if (!data) {
    free(key);
    return false;
  }

  storedChk = (data[12] >> 4) & 0xF;
  for (i = 0; i < 12; i++) calcChk = calcChk ^ data[i];
  calcChk &= 0xF;

  storedHash = ReadBE(data+8);
  calcHash = Hash(data, 8);
  free(key);
  free(data);
  return calcHash == storedHash && storedChk == calcChk;
}
