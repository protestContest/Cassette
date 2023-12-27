#include "cli.h"
#include "univ/str.h"
#include "univ/system.h"
#include "univ/math.h"
#include "compile/lex.h"
#include "runtime/vm.h"
#include <stdio.h>

static void PrintSourceContext(u32 pos, char *source, u32 context);

int Usage(void)
{
  printf("Usage: cassette [-t] entryScript [module1 ... moduleN]\n");
  return 1;
}

/*
Looks for the standard lib in this order:
- value of CASSETTE_STDLIB env variable
- $(HOME)/.local/share/cassette
- /usr/share/cassette
*/
static char *GetStdLibPath(void)
{
  char *home = GetEnv("HOME");
  char *path = GetEnv("CASSETTE_STDLIB");

  if (path && DirExists(path)) return path;

  if (home) {
    path = JoinStr(home, ".local/share/cassette", '/');
    if (DirExists(path)) return path;
  }

  if (DirExists("/usr/share/cassette")) {
    return "/usr/share/cassette";
  }

  return 0;
}

void Version(void)
{
  printf("Cassette v1.0 2023-12\n");
  printf("  Standard library: %s\n", GetStdLibPath());
#ifdef FONT_PATH
  printf("  Font path: %s\n", FONT_PATH);
#endif
}

Options ParseOpts(u32 argc, char *argv[])
{
  u32 i;
  Options opts = {false, 1, 0};
  for (i = 0; i < argc; i++) {
    if (StrEq(argv[i], "-h")) {
      Usage();
      Exit();
    }
    if (StrEq(argv[i], "-v")) {
      Version();
      Exit();
    }
    if (StrEq(argv[i], "-t")) {
      opts.trace = true;
      opts.file_args++;
    }
  }

  opts.stdlib_path = GetStdLibPath();
  return opts;
}

void PrintRuntimeError(Result error, VM *vm)
{
  u32 line, col;
  char *source = 0;

  printf("%s", ANSIRed);
  printf("Error: %s\n", error.error);

  PrintStackTrace(error.value, vm);

  if (error.filename) {
    source = ReadFile(error.filename);
    if (source) {
      line = LineNum(source, error.pos);
      col = ColNum(source, error.pos);
    }
  }

  if (error.filename) {
    printf("    %s", error.filename);
    if (source) {
      printf(":%d:%d", line+1, col+1);
    }
    printf("\n");
  }

  if (source) {
    PrintSourceContext(error.pos, source, 3);
  }
  printf("%s", ANSINormal);
}

void PrintError(Result error)
{
  u32 line, col;
  char *source = 0;

  if (error.filename) {
    source = ReadFile(error.filename);
    if (source) {
      line = LineNum(source, error.pos);
      col = ColNum(source, error.pos);
    }
  }

  printf("%s", ANSIRed);
  if (error.filename) {
    printf("%s", error.filename);
    if (source) {
      printf(":%d:%d", line+1, col+1);
    }
    printf(" ");
  }

  printf("Error: %s\n", error.error);
  if (source) {
    PrintSourceContext(error.pos, source, 3);
  }
  printf("%s", ANSINormal);
}

static void PrintStackTraceLine(char *filename, u32 pos)
{
  char *source = ReadFile(filename);
  u32 line, col;

  line = LineNum(source, pos);
  col = ColNum(source, pos);
  printf("    %s:%d:%d", filename, line + 1, col + 1);
}

void PrintStackTrace(Val stack, VM *vm)
{
  u32 iterations = 0;
  char *prev_filename = 0;
  u32 prev_pos = 0;

  while (stack != Nil) {
    Val item = Head(stack, &vm->mem);
    char *filename = SymbolName(Head(item, &vm->mem), &vm->chunk->symbols);
    u32 pos = RawInt(Tail(item, &vm->mem));
    if (filename == prev_filename && pos == prev_pos) {
      iterations++;
    } else {
      if (iterations > 0) {
        printf(" (%d iterations)", iterations + 1);
        iterations = 0;
      }
      printf("\n");
      PrintStackTraceLine(filename, pos);
    }
    prev_filename = filename;
    prev_pos = pos;
    stack = Tail(stack, &vm->mem);
  }
  if (iterations > 0) {
    printf(" (%d iterations)\n", iterations + 1);
  } else {
    printf("\n");
  }
}

static void PrintSourceContext(u32 pos, char *source, u32 context)
{
  Lexer lex;
  u32 line, col, start_line, end_line, cur_line, i;
  char *cur;

  InitLexer(&lex, source, pos);
  line = LineNum(source, lex.token.lexeme - lex.source);
  col = ColNum(source, lex.token.lexeme - lex.source);

  start_line = Max((i32)line - (i32)context, 0);
  end_line = line + context + 1;

  cur = source;
  cur_line = 0;

  /* skip to first line of context */
  while (cur_line < start_line) {
    cur = LineEnd(cur);
    if (*cur == 0) break;
    cur_line++;
    cur++;
  }

  /* print lines up to and including target line */
  while (cur_line <= line) {
    char *end = LineEnd(cur);
    printf("%3d│ %.*s\n", cur_line+1, (u32)(end - cur), cur);
    cur_line++;
    cur = end;
    if (*end == 0) break;
    cur++;
  }

  /* underline target token */
  printf("   │ ");
  for (i = 0; i < col; i++) printf(" ");
  for (i = 0; i < lex.token.length; i++) printf("^");
  printf("\n");

  /* print remaining context */
  while (*cur && cur_line < end_line) {
    char *end = LineEnd(cur);
    printf("%3d│ %.*s\n", cur_line+1, (u32)(end - cur), cur);
    if (*end == 0) break;
    cur = end + 1;
    cur_line++;
  }
}
