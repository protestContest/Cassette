#pragma once

extern char EmptyIV[12];

void ChaCha(void *data, u32 len, void *buf, char key[32], char iv[12], u32 blockCounter, u32 rounds);
#define Encrypt(data, len, key) ChaCha(data, len, data, key, EmptyIV, 0, 8)
#define Decrypt(data, len, key) ChaCha(data, len, data, key, EmptyIV, 0, 8)
