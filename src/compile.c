#include "compile.h"
#include "reader.h"

CompileResult CompileOk(Chunk *chunk)
{
  CompileResult result;
  result.status = Ok;
  result.chunk = chunk;
  return result;
}

CompileResult CompileError(char *msg)
{
  CompileResult result;
  result.status = Error;
  result.error = msg;
  return result;
}

CompileResult Compile(char *src)
{
  Reader *reader = NewReader(src);

  u32 line = 0;
  while (true) {
    Token token = ScanToken(reader);

    if (token.line != line) {
      printf("%3u│ ", token.line);
      line = token.line;
    } else {
      printf("   │ ");
    }

    printf("%2d \"%*.s\"\n", token.type, token.length, token.start);

    if (token.type == TOKEN_EOF) break;
  }

  return CompileOk(NewChunk());
}
