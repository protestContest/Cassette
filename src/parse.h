#pragma once
#include "reader.h"

#define IsSpace(c)        ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsEnd(c)          ((c) == '\0')

Val ParseBlock(Reader *r);
