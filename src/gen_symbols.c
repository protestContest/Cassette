#ifdef GEN_SYMBOLS

#include <stdio.h>
#include "mem/symbols.h"

int main(void)
{
  printf("/* compile/parse.h */\n");
  printf("#define SymBangEq       0x%08X /* != */\n", SymbolFor("!="));
  printf("#define SymHash         0x%08X /* # */\n", SymbolFor("#"));
  printf("#define SymPercent      0x%08X /* %% */\n", SymbolFor("%"));
  printf("#define SymAmpersand    0x%08X /* & */\n", SymbolFor("&"));
  printf("#define SymStar         0x%08X /* * */\n", SymbolFor("*"));
  printf("#define SymPlus         0x%08X /* + */\n", SymbolFor("+"));
  printf("#define SymMinus        0x%08X /* - */\n", SymbolFor("-"));
  printf("#define SymDot          0x%08X /* . */\n", SymbolFor("."));
  printf("#define SymDotDot       0x%08X /* .. */\n", SymbolFor(".."));
  printf("#define SymSlash        0x%08X /* / */\n", SymbolFor("/"));
  printf("#define SymLt           0x%08X /* < */\n", SymbolFor("<"));
  printf("#define SymLtLt         0x%08X /* << */\n", SymbolFor("<<"));
  printf("#define SymLtEq         0x%08X /* <= */\n", SymbolFor("<="));
  printf("#define SymLtGt         0x%08X /* <> */\n", SymbolFor("<>"));
  printf("#define SymEqEq         0x%08X /* == */\n", SymbolFor("=="));
  printf("#define SymGt           0x%08X /* > */\n", SymbolFor(">"));
  printf("#define SymGtEq         0x%08X /* >= */\n", SymbolFor(">="));
  printf("#define SymGtGt         0x%08X /* >> */\n", SymbolFor(">>"));
  printf("#define SymCaret        0x%08X /* ^ */\n", SymbolFor("^"));
  printf("#define SymIn           0x%08X /* in */\n", SymbolFor("in"));
  printf("#define SymNot          0x%08X /* not */\n", SymbolFor("not"));
  printf("#define SymBar          0x%08X /* | */\n", SymbolFor("|"));
  printf("#define SymTilde        0x%08X /* ~ */\n", SymbolFor("~"));
  printf("#define SymSet          0x%08X /* set! */\n", SymbolFor("set!"));
  printf("\n");

  printf("/* device/device.c */\n");
  printf("#define SymConsole      0x%08X /* console */\n", SymbolFor("console"));
  printf("#define SymFile         0x%08X /* file */\n", SymbolFor("file"));
  printf("#define SymDirectory    0x%08X /* directory */\n", SymbolFor("directory"));
  printf("#define SymSerial       0x%08X /* serial */\n", SymbolFor("serial"));
  printf("#define SymSystem       0x%08X /* system */\n", SymbolFor("system"));
  printf("#define SymWindow       0x%08X /* window */\n", SymbolFor("window"));
  printf("\n");

  printf("/* device/directory.c */\n");
  printf("#define SymDevice       0x%08X /* device */\n", SymbolFor("device"));
  printf("#define SymDirectory    0x%08X /* directory */\n", SymbolFor("directory"));
  printf("#define SymPipe         0x%08X /* pipe */\n", SymbolFor("pipe"));
  printf("#define SymLink         0x%08X /* link */\n", SymbolFor("link"));
  printf("#define SymFile         0x%08X /* file */\n", SymbolFor("file"));
  printf("#define SymSocket       0x%08X /* socket */\n", SymbolFor("socket"));
  printf("#define SymUnknown      0x%08X /* unknown */\n", SymbolFor("unknown"));
  printf("#define SymPath         0x%08X /* path */\n", SymbolFor("path"));
  printf("\n");

  printf("/* device/file.c */\n");
  printf("#define SymPosition     0x%08X /* position */\n", SymbolFor("position"));
  printf("\n");

  printf("/* device/system.c */\n");
  printf("#define SymSeed         0x%08X /* seed */\n", SymbolFor("seed"));
  printf("#define SymRandom       0x%08X /* random */\n", SymbolFor("random"));
  printf("#define SymTime         0x%08X /* time */\n", SymbolFor("time"));
  printf("\n");

  printf("/* device/window.c */\n");
  printf("#define SymClear        0x%08X /* clear */\n", SymbolFor("clear"));
  printf("#define SymText         0x%08X /* text */\n", SymbolFor("text"));
  printf("#define SymLine         0x%08X /* line */\n", SymbolFor("line"));
  printf("#define SymBlit         0x%08X /* blit */\n", SymbolFor("blit"));
  printf("#define SymWidth        0x%08X /* width */\n", SymbolFor("width"));
  printf("#define SymHeight       0x%08X /* height */\n", SymbolFor("height"));
  printf("#define SymFont         0x%08X /* font */\n", SymbolFor("font"));
  printf("#define SymFontSize     0x%08X /* font-size */\n", SymbolFor("font-size"));
  printf("#define SymColor        0x%08X /* color */\n", SymbolFor("color"));
  printf("\n");

  printf("/* mem/mem.c */\n");
  printf("#define Moved           0x%08X /* *moved* */\n", SymbolFor("*moved*"));
  printf("\n");

  printf("/* mem/mem.h */\n");
  printf("#define True            0x%08X /* true */\n", SymbolFor("true"));
  printf("#define False           0x%08X /* false */\n", SymbolFor("false"));
  printf("#define Ok              0x%08X /* ok */\n", SymbolFor("ok"));
  printf("#define Error           0x%08X /* error */\n", SymbolFor("error"));
  printf("\n");

  printf("/* mem/symbols.h */\n");
  printf("#define Undefined       0x%08X /* *undefined* */\n", SymbolFor("*undefined*"));
  printf("\n");

  printf("/* runtime/primitives.c */\n");
  printf("#define FloatType       0x%08X /* float */\n", SymbolFor("float"));
  printf("#define NumType         0x%08X /* number */\n", SymbolFor("number"));
  printf("#define AnyType         0x%08X /* any */\n", SymbolFor("any"));
  printf("\n");

  printf("static PrimitiveDef primitives[] = {\n");
  printf("  {\"head\",            0x%08X, &VMHead},\n", SymbolFor("head"));
  printf("  {\"tail\",            0x%08X, &VMTail},\n", SymbolFor("tail"));
  printf("  {\"panic!\",          0x%08X, &VMPanic},\n", SymbolFor("panic!"));
  printf("  {\"unwrap\",          0x%08X, &VMUnwrap},\n", SymbolFor("unwrap"));
  printf("  {\"unwrap!\",         0x%08X, &VMForceUnwrap},\n", SymbolFor("unwrap!"));
  printf("  {\"ok?\",             0x%08X, &VMOk},\n", SymbolFor("ok?"));
  printf("  {\"float?\",          0x%08X, &VMIsFloat},\n", SymbolFor("float?"));
  printf("  {\"integer?\",        0x%08X, &VMIsInt},\n", SymbolFor("integer?"));
  printf("  {\"symbol?\",         0x%08X, &VMIsSym},\n", SymbolFor("symbol?"));
  printf("  {\"pair?\",           0x%08X, &VMIsPair},\n", SymbolFor("pair?"));
  printf("  {\"tuple?\",          0x%08X, &VMIsTuple},\n", SymbolFor("tuple?"));
  printf("  {\"binary?\",         0x%08X, &VMIsBin},\n", SymbolFor("binary?"));
  printf("  {\"map?\",            0x%08X, &VMIsMap},\n", SymbolFor("map?"));
  printf("  {\"function?\",       0x%08X, &VMIsFunc},\n", SymbolFor("function?"));
  printf("  {\"!=\",              0x%08X, &VMNotEqual},\n", SymbolFor("!="));
  printf("  {\"#\",               0x%08X, &VMLen},\n", SymbolFor("#"));
  printf("  {\"%%\",               0x%08X, &VMRem},\n", SymbolFor("%"));
  printf("  {\"&\",               0x%08X, &VMBitAnd},\n", SymbolFor("&"));
  printf("  {\"*\",               0x%08X, &VMMul},\n", SymbolFor("*"));
  printf("  {\"+\",               0x%08X, &VMAdd},\n", SymbolFor("+"));
  printf("  {\"-\",               0x%08X, &VMSub},\n", SymbolFor("-"));
  printf("  {\"..\",              0x%08X, &VMRange},\n", SymbolFor(".."));
  printf("  {\"/\",               0x%08X, &VMDiv},\n", SymbolFor("/"));
  printf("  {\"<\",               0x%08X, &VMLess},\n", SymbolFor("<"));
  printf("  {\"<<\",              0x%08X, &VMShiftLeft},\n", SymbolFor("<<"));
  printf("  {\"<=\",              0x%08X, &VMLessEqual},\n", SymbolFor("<="));
  printf("  {\"<>\",              0x%08X, &VMCat},\n", SymbolFor("<>"));
  printf("  {\"==\",              0x%08X, &VMEqual},\n", SymbolFor("=="));
  printf("  {\">\",               0x%08X, &VMGreater},\n", SymbolFor(">"));
  printf("  {\">=\",              0x%08X, &VMGreaterEqual},\n", SymbolFor(">="));
  printf("  {\">>\",              0x%08X, &VMShiftRight},\n", SymbolFor(">>"));
  printf("  {\"^\",               0x%08X, &VMBitOr},\n", SymbolFor("^"));
  printf("  {\"in\",              0x%08X, &VMIn},\n", SymbolFor("in"));
  printf("  {\"not\",             0x%08X, &VMNot},\n", SymbolFor("not"));
  printf("  {\"|\",               0x%08X, &VMPair},\n", SymbolFor("|"));
  printf("  {\"~\",               0x%08X, &VMBitNot},\n", SymbolFor("~"));
  printf("  {\"open\",            0x%08X, &VMOpen},\n", SymbolFor("open"));
  printf("  {\"close\",           0x%08X, &VMClose},\n", SymbolFor("close"));
  printf("  {\"read\",            0x%08X, &VMRead},\n", SymbolFor("read"));
  printf("  {\"write\",           0x%08X, &VMWrite},\n", SymbolFor("write"));
  printf("  {\"get-param\",       0x%08X, &VMGetParam},\n", SymbolFor("get-param"));
  printf("  {\"set-param\",       0x%08X, &VMSetParam},\n", SymbolFor("set-param"));
  printf("  {\"map-get\",         0x%08X, &VMMapGet},\n", SymbolFor("map-get"));
  printf("  {\"map-set\",         0x%08X, &VMMapSet},\n", SymbolFor("map-set"));
  printf("  {\"map-del\",         0x%08X, &VMMapDelete},\n", SymbolFor("map-del"));
  printf("  {\"map-keys\",        0x%08X, &VMMapKeys},\n", SymbolFor("map-keys"));
  printf("  {\"map-values\",      0x%08X, &VMMapValues},\n", SymbolFor("map-values"));
  printf("  {\"substr\",          0x%08X, &VMSubStr},\n", SymbolFor("substr"));
  printf("  {\"stuff\",           0x%08X, &VMStuff},\n", SymbolFor("stuff"));
  printf("  {\"trunc\",           0x%08X, &VMTrunc},\n", SymbolFor("trunc"));
  printf("  {\"symbol-name\",     0x%08X, &VMSymName},\n", SymbolFor("symbol-name"));
  printf("};\n");

  return 0;
}

#endif
