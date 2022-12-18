#include "printer.h"
#include "vm.h"
#include "string.h"

char *TypeAbbr(Value value)
{
  switch (TypeOf(value)) {
  case NUMBER:  return "№";
  case INDEX:   return "#";
  case SYMBOL:  return "★";
  case PAIR:    return "⚭";
  case OBJECT: {
    switch (ObjTypeOf(value)) {
    case BINARY:    return "⨳";
    case FUNCTION:  return "λ";
    case TUPLE:     return "☰";
    case DICT:      return ":";
    }
  }
  default:      return "⍰";
  }
}

/* Value print functions
 */

#define PrintNumber(value) printf("%f", (float)RawVal(value))
#define PrintIndex(value) printf("%u", RawVal(value))

void PrintSymbol(Value value)
{
  Value name = SymbolName(value);
  char *str = BinaryData(name);
  u32 size = BinarySize(name);

  for (u32 i = 0; i < size; i++) {
    printf("%c", str[i]);
  }
}

void PrintListTail(Value value)
{
  if (IsPair(value)) {
    PrintValue(Head(value));

    if (IsNil(Tail(value))) {
      printf(")");
    } else {
      PrintListTail(Tail(value));
    }
  } else {
    PrintValue(value);
    printf(")");
  }
}

void PrintPair(Value value)
{
  printf("%d", RawVal(value));
  // TODO: debug
  // printf("(");
  // PrintValue(Head(value));

  // if (IsPair(Tail(value))) {
  //   PrintListTail(Tail(value));
  // } else {
  //   printf(" . ");
  //   PrintValue(Tail(value));
  //   printf(")");
  // }
}

void PrintBinary(Value value)
{
  printf("\"");
  char *str = BinaryData(value);
  u32 size = BinarySize(value);

  for (u32 i = 0; i < size; i++) {
    printf("%c", str[i]);
  }
  printf("\"");
}

#define PrintFunction(value) printf("f0x%0X", value)
#define PrintTuple(value) printf("t0x%0X", value)
#define PrintDict(value) printf("d0x%0X", value)

void PrintObject(Value value)
{
  switch(ObjTypeOf(value)) {
  case BINARY:    PrintBinary(value); break;
  case FUNCTION:  PrintFunction(value); break;
  case TUPLE:     PrintTuple(value); break;
  case DICT:      PrintDict(value); break;
  }
}

void PrintValue(Value value)
{
  switch (TypeOf(value)) {
  case NUMBER:  PrintNumber(value); break;
  case INDEX:   PrintIndex(value); break;
  case SYMBOL:  PrintSymbol(value); break;
  case PAIR:    PrintPair(value); break;
  case OBJECT:  PrintObject(value); break;
  }
}

void DebugValue(Value value)
{
  printf("%s ", TypeAbbr(value));
  PrintValue(value);
}

/* Table
 */

#define MIN_TABLE_WIDTH 28
#define MAX_TABLE_WIDTH 140

typedef struct Table {
  u32 num_cols;
  u32 *width;
} Table;

void EmptyTable(char *title)
{
  u32 table_pad = MIN_TABLE_WIDTH - CountGraphemes(title, 1, 0);
  printf("  %s%s", UNDERLINE_START, title);
  for (u32 i = 0; i < table_pad; i++) {
    printf(" ");
  }
  printf("%s", UNDERLINE_END);
  printf("\n    (empty)\n\n");
}

void TableTitle(char *title, u32 num_cols, u32 cols_width)
{
  u32 divider_width = (num_cols-1)*3;
  u32 table_width = cols_width + divider_width;
  if (table_width > MAX_TABLE_WIDTH) table_width = MAX_TABLE_WIDTH;
  if (table_width < MIN_TABLE_WIDTH) table_width = MIN_TABLE_WIDTH;
  u32 table_pad = table_width - CountGraphemes(title, 1, 0);
  printf("  %s%s", UNDERLINE_START, title);
  for (u32 i = 0; i < table_pad; i++) {
    printf(" ");
  }
  printf("%s", UNDERLINE_END);
}

Table *BeginTable(char *title, u32 num_cols, ...)
{
  Table *table = malloc(sizeof(Table));
  table->num_cols = num_cols;
  table->width = calloc(num_cols, sizeof(u32));

  u32 cols_width = 0;
  va_list args;
  va_start(args, num_cols);
  for (u32 i = 0; i < num_cols; i++) {
    table->width[i] = va_arg(args, u32);
    cols_width += table->width[i];
  }
  va_end(args);

  TableTitle(title, num_cols, cols_width);

  return table;
}

void PrintFloatItem(Value value, u32 width)
{
  char fmt[8];
  sprintf(fmt, "%% %d.1f", width);
  printf(fmt, RawVal(value));
}

void PrintStringItem(Value value, u32 width)
{
  u32 len = CountGraphemes(BinaryData(value), 0, BinarySize(value));
  if (len > width) len = width;
  for (u32 i = 0; i < width - len; i++) {
    printf(" ");
  }
  for (u32 i = 0; i < len; i++) {
    printf("%c", BinaryData(value)[i]);
  }
}

void PrintIntItem(Value value, u32 width)
{
  char fmt[8];
  sprintf(fmt, "%% %du", width);
  printf(fmt, RawVal(value));
}

void TableItem(Value value, u32 width)
{
  if (width < 2) return;

  switch (TypeOf(value)) {
  case NUMBER: PrintFloatItem(value, width-1); break;
  case SYMBOL: PrintStringItem(SymbolName(value), width-1); break;
  case INDEX:  PrintIntItem(value, width-1); break;
  case PAIR:   PrintIntItem(value, width-1); break;
  case OBJECT:
    switch(ObjTypeOf(value)) {
    case BINARY:  {
      PrintIntItem(value, width-1);
      break;
    }
    default:  PrintIntItem(value, width-1); break;
    }
  }
}

void TableRow(Table *table, ...)
{
  va_list args;
  BeginRow();
  va_start(args, table);
  Value value = va_arg(args, Value);
  TableItem(value, table->width[0]);
  for (u32 i = 1; i < table->num_cols; i++) {
    value = va_arg(args, Value);
    TableSep();
    printf("%s ", TypeAbbr(value));
    TableItem(value, table->width[i] - 1);
  }
  va_end(args);
}

void EndTable(Table *table)
{
  free(table);
  printf("\n\n");
}

/* Hexdump
 */

void HexDump(u8 *data, u32 size, char *title)
{
  u32 addr_size = (size < 0x10000) ? 4 : 8;
  u32 row_bytes = 32;

  char addr_fmt[7];
  sprintf(addr_fmt, "  %%0%dX", addr_size);

  if (size == 0) {
    EmptyTable(title);
    return;
  }
  TableTitle(title, 3, addr_size + 80 + 32);

  u32 ptr = 0;
  while (ptr < size) {
    BeginRow();
    printf(addr_fmt, ptr);

    TableSep();
    for (u32 i = 0; i < row_bytes; i++) {
      if (ptr + i < size) {
        u8 val = data[ptr+i];
        printf("%02X", val);
      } else {
        printf("  ");
      }
      if (i % 2 != 0 && i != (row_bytes-1) != 0) printf(" ");
    }

    TableSep();
    char *row = (char*)&data[ptr];
    for (u32 i = 0; i < row_bytes; i++) {
      if (ptr + i < size) {
        if (IsCtrl(row[i]) || IsUTFContinue(row[i])) printf(".");
        else printf("%c", row[i]);
      } else {
        printf(" ");
      }
      if ((i+1) % 4 == 0) printf(" ");
    }

    ptr += row_bytes;
  }
  TableEnd();
}
