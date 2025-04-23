#pragma once

extern char EmptyIV[12];

void ChaCha(u8 *data, u32 len, u8 *buf, u8 key[32], u8 iv[12], u32 blockCounter, u32 rounds);
u8 *Encrypt(void *data, u32 len, char *password);
void *Decrypt(u8 *data, u32 len, char *password);
void Sha256(void *data, u32 len, u8 result[32]);
void HMAC(void *data, u32 len, u8 *key, u32 keyLen, u8 result[32]);
void PrintHash(u8 hash[32]);
