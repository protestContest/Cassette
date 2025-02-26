#pragma once

/* Generates a product key based on the given data. Data should be 8 bytes in length. The key is
 * composed of 100 bits (13 bytes):
 *
 * - 8 bytes of original data
 * - 4 byte hash of data (see Hash)
 * - 4 bit checksum: each data/hash byte is xor'd together, then the low and high 4 bits of that are
     xor'd
 *
 * The 100 bits are base-32 encoded (Crockford's version) into 20 bytes, then interspersed with
 * hyphens for readability. The result is a 23-character string.
 */
char *GenKey(void *data);

/* Returns whether a product key is valid. */
bool KeyValid(char *key);
