#pragma once

/* Heap */

typedef union {
  i32 as_i;
  float as_f;
} Val;

#define nanMask           0x7FC00000
#define typeMask          0xFFF00000

#define intMask           0x7FC00000
#define symMask           0x7FD00000
#define pairMask          0x7FE00000
#define objMask           0x7FF00000
#define tupleMask         0xFFC00000
#define binaryMask        0xFFD00000
#define mapMask           0xFFE00000

#define MakeVal(n, mask)  ((Val){.as_i = ((n) & ~typeMask) | (mask)})
#define FloatVal(n)       ((Val){.as_f = (float)(n)})
#define IntVal(n)         MakeVal((i32)(n), intMask)
#define SymVal(n)         MakeVal(n, symMask)
#define PairVal(n)        MakeVal(n, pairMask)
#define ObjVal(n)         MakeVal(n, objMask)
#define TupleHeader(n)    MakeVal(n, tupleMask)
#define BinaryHeader(n)   MakeVal(n, binaryMask)
#define MapHeader(n)      MakeVal(n, mapMask)

#define IsType(v, mask)   (((v).as_i & typeMask) == (mask))
#define IsFloat(v)        (((v).as_i & nanMask) != nanMask)
#define IsInt(v)          IsType(v, intMask)
#define IsSym(v)          IsType(v, symMask)
#define IsPair(v)         IsType(v, pairMask)
#define IsObj(v)          IsType(v, objMask)
#define IsTupleHeader(v)  IsType(v, tupleMask)
#define IsBinaryHeader(v) IsType(v, binaryMask)
#define IsMapHeader(v)    IsType(v, mapMask)

#define SignExt(n)        ((((n) + 0x00080000) & 0x000FFFFF) - 0x00080000)
#define valBits           20

#define RawFloat(v)       (v).as_f
#define RawInt(v)         SignExt((v).as_i)
#define RawVal(v)         ((v).as_i & ~typeMask)

#define SymbolFrom(s, l)  SymVal(HashBits(s, l, valBits))
#define SymbolFor(str)    SymbolFrom(str, StrLen(str))

#define Eq(v1, v2)        ((v1).as_i == (v2).as_i)
#define nil               PairVal(0)
#define IsNil(v)          Eq(nil, v)
#define BoolVal(v)        ((v) ? True : False)
#define IsTrue(v)         !(IsNil(v) || Eq(v, SymbolFor("false")))
#define IsNum(v)          (IsFloat(v) || IsInt(v))
#define RawNum(v)         (IsFloat(v) ? RawFloat(v) : RawInt(v))
#define IsZero(v)         ((IsFloat(v) && RawNum(v) == 0.0) || (IsInt(v) && RawInt(v) == 0))

typedef struct {
  Val *values;
  char *strings;
  HashMap string_map;
} Heap;

Val OkResult(Val value, Heap *mem);
Val ErrorResult(char *error, Heap *mem);
void InitMem(Heap *mem, u32 size);
void DestroyMem(Heap *mem);
u32 MemSize(Heap *mem);
void CopyStrings(Heap *src, Heap *dst);
void CollectGarbage(Val **roots, u32 num_roots, Heap *mem);
Val MakeSymbol(char *name, Heap *mem);
Val MakeSymbolFrom(char *name, u32 length, Heap *mem);
char *SymbolName(Val symbol, Heap *mem);
Val HashValue(Val value, Heap *mem);
Val Pair(Val head, Val tail, Heap *mem);
Val Head(Val pair, Heap *mem);
Val Tail(Val pair, Heap *mem);
void SetHead(Val pair, Val head, Heap *mem);
void SetTail(Val pair, Val tail, Heap *mem);
u32 ListLength(Val list, Heap *mem);
bool ListContains(Val list, Val value, Heap *mem);
Val ListAt(Val list, u32 pos, Heap *mem);
bool IsTagged(Val list, char *tag, Heap *mem);
Val TailList(Val list, u32 pos, Heap *mem);
Val TruncList(Val list, u32 pos, Heap *mem);
Val JoinLists(Val a, Val b, Heap *mem);
Val ListConcat(Val a, Val b, Heap *mem);
Val ReverseList(Val list, Heap *mem);
Val MakeTuple(u32 length, Heap *mem);
bool IsTuple(Val tuple, Heap *mem);
u32 TupleLength(Val tuple, Heap *mem);
bool TupleContains(Val tuple, Val value, Heap *mem);
Val TupleGet(Val tuple, u32 index, Heap *mem);
void TupleSet(Val tuple, u32 index, Val value, Heap *mem);
Val TuplePut(Val tuple, u32 index, Val value, Heap *mem);
Val TupleInsert(Val tuple, u32 index, Val value, Heap *mem);
Val JoinTuples(Val tuple1, Val tuple2, Heap *mem);
Val MakeTuple(u32 length, Heap *mem);
bool IsTuple(Val tuple, Heap *mem);
u32 TupleLength(Val tuple, Heap *mem);
bool TupleContains(Val tuple, Val value, Heap *mem);
Val TupleGet(Val tuple, u32 index, Heap *mem);
void TupleSet(Val tuple, u32 index, Val value, Heap *mem);
Val TuplePut(Val tuple, u32 index, Val value, Heap *mem);
Val TupleInsert(Val tuple, u32 index, Val value, Heap *mem);
Val JoinTuples(Val tuple1, Val tuple2, Heap *mem);
Val MakeBinary(u32 length, Heap *mem);
Val BinaryFrom(void *data, u32 length, Heap *mem);
bool IsBinary(Val binary, Heap *mem);
u32 BinaryLength(Val binary, Heap *mem);
void *BinaryData(Val binary, Heap *mem);
u8 BinaryGetByte(Val binary, u32 i, Heap *mem);
void BinarySetByte(Val binary, u32 i, u8 b, Heap *mem);
Val BinaryAfter(Val binary, u32 index, Heap *mem);
Val BinaryTrunc(Val binary, u32 index, Heap *mem);
Val SliceBinary(Val binary, u32 start, u32 end, Heap *mem);
Val JoinBinaries(Val b1, Val b2, Heap *mem);
Val MakeMap(Heap *mem);
Val MapFrom(Val keys, Val vals, Heap *mem);
bool IsMap(Val val, Heap *mem);
u32 MapCount(Val map, Heap *mem);
bool MapContains(Val map, Val key, Heap *mem);
Val MapGet(Val map, Val key, Heap *mem);
Val MapPut(Val map, Val key, Val value, Heap *mem);
Val MapDelete(Val map, Val key, Heap *mem);
Val MapKeys(Val map, Heap *mem);
Val MapValues(Val map, Heap *mem);
u32 Inspect(Val value, Heap *mem);

/* Opts */

typedef struct {
  char *entry;
  char *dir;
  bool compile;
  u32 verbose;
} CassetteOpts;

/* VM */

typedef enum {
  OpHalt,
  OpPop,
  OpDup,
  OpNeg,
  OpNot,
  OpLen,
  OpMul,
  OpDiv,
  OpRem,
  OpAdd,
  OpSub,
  OpIn,
  OpGt,
  OpLt,
  OpEq,
  OpConst,
  OpStr,
  OpPair,
  OpTuple,
  OpSet,
  OpMap,
  OpGet,
  OpExtend,
  OpDefine,
  OpLookup,
  OpExport,
  OpJump,
  OpBranch,
  OpCont,
  OpReturn,
  OpSaveEnv,
  OpRestEnv,
  OpSaveCont,
  OpRestCont,
  OpLambda,
  OpApply,
  OpModule,
  OpLoad,
} OpCode;

typedef struct {
  char *filename;
  u32 offset;
} FileOffset;

typedef struct {
  u8 *data;
  Heap constants;
  HashMap source_map;
  FileOffset *sources;
} Chunk;

typedef enum {
  NoError,
  StackError,
  TypeError,
  ArithmeticError,
  EnvError,
  KeyError,
  ArityError,
  RuntimeError
} VMError;

typedef struct {
  u32 pc;
  u32 cont;
  Val *stack;
  Val *call_stack;
  Val env;
  Val *modules;
  HashMap mod_map;
  Heap mem;
  VMError error;
  CassetteOpts *opts;
} VM;

typedef enum {
  RegCont,
  RegEnv,
} Reg;

typedef Val (*PrimitiveImpl)(VM *vm, Val args);

typedef struct {
  char *module;
  char *name;
  PrimitiveImpl impl;
} PrimitiveDef;

u32 OpLength(OpCode op);
#define CurrentVersion  1
void InitChunk(Chunk *chunk);
void DestroyChunk(Chunk *chunk);
void FreeChunk(Chunk *chunk);
u32 ChunkSize(Chunk *chunk);
u8 ChunkRef(Chunk *chunk, u32 i);
Val ChunkConst(Chunk *chunk, u32 i);
Val *ChunkBinary(Chunk *chunk, u32 i);
void PushByte(u8 byte, Chunk *chunk);
void PushConst(Val value, Chunk *chunk);
u8 *SerializeChunk(Chunk *chunk);
void DeserializeChunk(u8 *data, Chunk *chunk);
void AddSourceFile(char *filename, u32 offset, Chunk *chunk);
char *SourceFile(u32 pos, Chunk *chunk);
u32 SourcePos(u32 pos, Chunk *chunk);
Chunk *LoadChunk(CassetteOpts *opts);
void InitVM(VM *vm, CassetteOpts *opts);
void DestroyVM(VM *vm);
void RunChunk(VM *vm, Chunk *chunk);
void StackPush(VM *vm, Val value);
Val StackPop(VM *vm);
Val InitialEnv(Heap *mem);
Val CompileEnv(Heap *mem);
Val ExtendEnv(Val env, Val frame, Heap *mem);
Val RestoreEnv(Val env, Heap *mem);
void Define(u32 pos, Val item, Val env, Heap *mem);
Val Lookup(Val env, u32 frame, u32 pos, Heap *mem);
Val FindVar(Val var, Val env, Heap *mem);
bool IsFunc(Val func, Heap *mem);
Val MakeFunc(Val entry, Val env, Heap *mem);
Val FuncEntry(Val func, Heap *mem);
Val FuncEnv(Val func, Heap *mem);
void SeedPrimitives(void);
void AddPrimitive(char *module, char *name, PrimitiveImpl fn);
bool IsPrimitive(Val value, Heap *mem);
Val DoPrimitive(Val prim, VM *vm);
Val PrimitiveMap(Heap *mem);

/* Rec */

typedef struct {
  char *message;
  char *file;
  u32 pos;
} CompileError;

typedef enum {
  TokenEOF,
  TokenID,
  TokenBangEqual,
  TokenString,
  TokenNewline,
  TokenHash,
  TokenPercent,
  TokenLParen,
  TokenRParen,
  TokenStar,
  TokenPlus,
  TokenComma,
  TokenMinus,
  TokenArrow,
  TokenDot,
  TokenSlash,
  TokenNum,
  TokenColon,
  TokenLess,
  TokenLessEqual,
  TokenEqual,
  TokenEqualEqual,
  TokenGreater,
  TokenGreaterEqual,
  TokenLBracket,
  TokenRBracket,
  TokenAnd,
  TokenAs,
  TokenCond,
  TokenDef,
  TokenDo,
  TokenElse,
  TokenEnd,
  TokenFalse,
  TokenIf,
  TokenImport,
  TokenIn,
  TokenLet,
  TokenModule,
  TokenNil,
  TokenNot,
  TokenOr,
  TokenTrue,
  TokenLBrace,
  TokenBar,
  TokenRBrace,
} TokenType;

typedef struct {
  TokenType type;
  char *lexeme;
  u32 length;
  u32 pos;
} Token;

typedef struct {
  char *text;
  u32 pos;
  Token token;
} Lexer;

typedef struct {
  bool ok;
  union {
    Val value;
    CompileError error;
  };
} ParseResult;

typedef struct {
  bool ok;
  u32 needs;
  u32 modifies;
  Val stmts;
} Seq;

typedef struct {
  bool ok;
  union {
    Seq code;
    CompileError error;
  };
} CompileResult;

typedef struct {
  Val name;
  char *file;
  Seq code;
  HashMap imports;
} Module;

typedef struct ModuleResult {
  bool ok;
  union {
    Module module;
    CompileError error;
  };
} ModuleResult;

void PrintSourceContext(char *src, u32 pos);
void PrintCompileError(CompileError *error, char *filename);
#define AtEnd(lex)  (PeekToken(lex).type == TokenEOF)
void InitLexer(Lexer *lex, char *text, u32 start);
Token NextToken(Lexer *lex);
Token PeekToken(Lexer *lex);
Val TokenPos(Lexer *lex);
ParseResult Parse(char *source, Heap *mem);
#define REnv  (1 << RegEnv)
#define RCont (1 << RegCont)
#define Needs(seq, reg)     ((seq).needs & (reg))
#define Modifies(seq, reg)  ((seq).modifies & (reg))
typedef Val Linkage;
#define LinkReturn  SymbolFor("return")
#define LinkExport  SymbolFor("export")
#define LinkNext    SymbolFor("next")
#define MakeSeq(needs, modifies, stmts)   (Seq){true, needs, modifies, stmts}
#define EmptySeq(mem)   MakeSeq(0, 0, nil)
#define IsEmptySeq(seq) (IsNil((seq).stmts))
Val MakeLabel(void);
char *GetLabel(Val label);
Val Label(Val label, Heap *mem);
Val LabelRef(Val label, Heap *mem);
Seq LabelSeq(Val label, Heap *mem);
Val ModuleRef(Val name, Heap *mem);
Seq ModuleSeq(char *file, Heap *mem);
Val SourceRef(u32 pos, Heap *mem);
Seq SourceSeq(u32 pos, Heap *mem);
Seq AppendSeq(Seq seq1, Seq seq2, Heap *mem);
Seq TackOnSeq(Seq seq1, Seq seq2, Heap *mem);
Seq ParallelSeq(Seq seq1, Seq seq2, Heap *mem);
Seq Preserving(u32 regs, Seq seq1, Seq seq2, Heap *mem);
Seq EndWithLinkage(Linkage linkage, Seq seq, Heap *mem);
struct ModuleResult Compile(Val ast, CassetteOpts *opts, Heap *mem);
void Assemble(Seq seq, Chunk *chunk, Heap *mem);
ModuleResult LoadModule(char *path, Heap *mem, CassetteOpts *opts);
CompileResult LoadModules(CassetteOpts *opts, Heap *mem);

/* Lib */

PrimitiveDef *StdLib(void);
u32 StdLibSize(void);

Val StdHead(VM *vm, Val args);
Val StdTail(VM *vm, Val args);
Val StdIsNil(VM *vm, Val args);
Val StdIsNum(VM *vm, Val args);
Val StdIsInt(VM *vm, Val args);
Val StdIsSym(VM *vm, Val args);
Val StdIsPair(VM *vm, Val args);
Val StdIsTuple(VM *vm, Val args);
Val StdIsBinary(VM *vm, Val args);
Val StdIsMap(VM *vm, Val args);
Val IOPrint(VM *vm, Val args);
Val IOInspect(VM *vm, Val args);
Val IOOpen(VM *vm, Val args);
Val IORead(VM *vm, Val args);
Val IOWrite(VM *vm, Val args);
Val IOReadFile(VM *vm, Val args);
Val IOWriteFile(VM *vm, Val args);
Val ListValid(VM *vm, Val args);
Val ListToBin(VM *vm, Val args);
Val ListToTuple(VM *vm, Val args);
Val ListReverse(VM *vm, Val args);
Val ListTrunc(VM *vm, Val args);
Val ListAfter(VM *vm, Val args);
Val ListJoin(VM *vm, Val args);
Val TupleToList(VM *vm, Val args);
Val BinToList(VM *vm, Val args);
Val BinTrunc(VM *vm, Val args);
Val BinAfter(VM *vm, Val args);
Val BinSlice(VM *vm, Val args);
Val BinJoin(VM *vm, Val args);
Val StdMapNew(VM *vm, Val args);
Val StdMapKeys(VM *vm, Val args);
Val StdMapValues(VM *vm, Val args);
Val StdMapPut(VM *vm, Val args);
Val StdMapGet(VM *vm, Val args);
Val StdMapDelete(VM *vm, Val args);
Val StdMapToList(VM *vm, Val args);
Val MathRandom(VM *vm, Val args);
Val MathRandInt(VM *vm, Val args);
Val MathCeil(VM *vm, Val args);
Val MathFloor(VM *vm, Val args);
Val MathRound(VM *vm, Val args);
Val MathAbs(VM *vm, Val args);
Val MathMax(VM *vm, Val args);
Val MathMin(VM *vm, Val args);
Val MathSin(VM *vm, Val args);
Val MathCos(VM *vm, Val args);
Val MathTan(VM *vm, Val args);
Val MathLn(VM *vm, Val args);
Val MathExp(VM *vm, Val args);
Val MathPow(VM *vm, Val args);
Val MathSqrt(VM *vm, Val args);
Val MathPi(VM *vm, Val args);
Val MathE(VM *vm, Val args);
Val MathInfinity(VM *vm, Val args);
Val BitAnd(VM *vm, Val args);
Val BitOr(VM *vm, Val args);
Val BitNot(VM *vm, Val args);
Val BitXOr(VM *vm, Val args);
Val BitShift(VM *vm, Val args);
Val BitCount(VM *vm, Val args);
Val StringValid(VM *vm, Val args);
Val StringLength(VM *vm, Val args);
Val StringAt(VM *vm, Val args);
Val StringSlice(VM *vm, Val args);
Val SysTime(VM *vm, Val args);
Val SysExit(VM *vm, Val args);

