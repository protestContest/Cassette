#pragma once

/* Generates a product key based on the given data. Data should be 8 bytes in
 * length. */
char *GenKey(void *data);

/* Returns whether a product key is valid. */
bool KeyValid(char *key);
