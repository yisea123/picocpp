/* picoc mini standard C library - provides an optional tiny C standard library 
 * if BUILTIN_MINI_STDLIB is defined */ 
 
#include "picoc.h"
#include "interpreter.h"


/* endian-ness checking */
static const int __ENDIAN_CHECK__ = 1;
static int BigEndian;
static int LittleEndian;


/* global initialisation for libraries */
void Picoc::LibraryInit()
{
	Picoc *pc = this;
    /* define the version number macro */
    pc->VersionString = TableStrRegister( PICOC_VERSION);
	VariableDefinePlatformVarFromPointer("PICOC_VERSION", pc->CharPtrType, (UnionAnyValuePointer)&pc->VersionString, FALSE, 
		strlen(pc->VersionString) + 1);

    /* define endian-ness macros */
    BigEndian = ((*(char*)&__ENDIAN_CHECK__) == 0);
    LittleEndian = ((*(char*)&__ENDIAN_CHECK__) == 1);

    VariableDefinePlatformVar( "BIG_ENDIAN", &pc->IntType, &BigEndian, FALSE);
    VariableDefinePlatformVar( "LITTLE_ENDIAN", &pc->IntType, &LittleEndian, FALSE);
}

/* add a library */
void Picoc::LibraryAdd( struct Table *GlobalTable, const char *LibraryName, struct LibraryFunction *FuncList)
{
	Picoc *pc = this;
    struct ParseState Parser;
    int Count;
    const char *Identifier;
    struct ValueType *ReturnType;
    struct ValueAbs *NewValue;
    void *Tokens;
    const char *IntrinsicName = TableStrRegister( "c library");
    
    /* read all the library definitions */
    for (Count = 0; FuncList[Count].Prototype != NULL; Count++)
    {
        Tokens = LexAnalyse( IntrinsicName, FuncList[Count].Prototype, strlen((char *)FuncList[Count].Prototype), NULL);
		Parser.LexInitParser(pc, FuncList[Count].Prototype, Tokens, IntrinsicName, TRUE, FALSE);
		Parser.TypeParse(&ReturnType, &Identifier, NULL);
		NewValue = Parser.ParseFunctionDefinition( ReturnType, Identifier);
        NewValue->ValFuncDef(pc).Intrinsic = FuncList[Count].Func;
        HeapFreeMem( Tokens);
    }
}

/* print a type to a stream without using printf/sprintf */
void PrintType(struct ValueType *Typ, IOFILE *Stream)
{
    switch (Typ->Base)
    {
        case TypeVoid:          PrintStr("void", Stream); break;
        case TypeInt:           PrintStr("int", Stream); break;
        case TypeShort:         PrintStr("short", Stream); break;
        case TypeChar:          PrintStr("char", Stream); break;
        case TypeLong:          PrintStr("long", Stream); break;
        case TypeUnsignedInt:   PrintStr("unsigned int", Stream); break;
        case TypeUnsignedShort: PrintStr("unsigned short", Stream); break;
        case TypeUnsignedLong:  PrintStr("unsigned long", Stream); break;
        case TypeUnsignedChar:  PrintStr("unsigned char", Stream); break;
#ifndef NO_FP
        case TypeFP:            PrintStr("double", Stream); break;
#endif
        case TypeFunction:      PrintStr("function", Stream); break;
        case TypeMacro:         PrintStr("macro", Stream); break;
        case TypePointer:       if (Typ->FromType) PrintType(Typ->FromType, Stream); PrintCh('*', Stream); break;
        case TypeArray:         PrintType(Typ->FromType, Stream); PrintCh('[', Stream); if (Typ->ArraySize != 0) PrintSimpleInt(Typ->ArraySize, Stream); PrintCh(']', Stream); break;
        case TypeStruct:        PrintStr("struct ", Stream); PrintStr( Typ->IdentifierOfValueType, Stream); break;
        case TypeUnion:         PrintStr("union ", Stream); PrintStr(Typ->IdentifierOfValueType, Stream); break;
        case TypeEnum:          PrintStr("enum ", Stream); PrintStr(Typ->IdentifierOfValueType, Stream); break;
        case TypeGotoLabel:     PrintStr("goto label ", Stream); break;
        case Type_Type:         PrintStr("type ", Stream); break;
    }
}


#ifdef BUILTIN_MINI_STDLIB

/* 
 * This is a simplified standard library for small embedded systems. It doesn't require
 * a system stdio library to operate.
 *
 * A more complete standard library for larger computers is in the library_XXX.c files.
 */
 
static int TRUEValue = 1;
static int ZeroValue = 0;

void BasicIOInit(Picoc *pc)
{
    pc->CStdOutBase.Putch = &PlatformPutc;
    pc->CStdOut = &CStdOutBase;
}

/* initialise the C library */
void CLibraryInit()
{
	Picoc *pc=this;
    /* define some constants */
    VariableDefinePlatformVar( nullptr, "NULL", &IntType, (UnionAnyValuePointer )&ZeroValue, FALSE);
    VariableDefinePlatformVar( nullptr, "TRUE", &IntType, (UnionAnyValuePointer )&TRUEValue, FALSE);
    VariableDefinePlatformVar( nullptr, "FALSE", &IntType, (UnionAnyValuePointer )&ZeroValue, FALSE);
}

/* stream for writing into strings */
void SPutc(unsigned char Ch, union OutputStreamInfo *Stream)
{
    struct StringOutputStream *Out = &Stream->Str;
    *Out->WritePos++ = Ch;
}

/* print a character to a stream without using printf/sprintf */
void PrintCh(char OutCh, struct OutputStream *Stream)
{
    (*Stream->Putch)(OutCh, &Stream->i);
}

/* print a string to a stream without using printf/sprintf */
void PrintStr(const char *Str, struct OutputStream *Stream)
{
    while (*Str != 0)
        PrintCh(*Str++, Stream);
}

/* print a single character a given number of times */
void PrintRepeatedChar(Picoc *pc, char ShowChar, int Length, struct OutputStream *Stream)
{
    while (Length-- > 0)
        PrintCh(ShowChar, Stream);
}

/* print an unsigned integer to a stream without using printf/sprintf */
void PrintUnsigned(unsigned long Num, unsigned int Base, int FieldWidth, int ZeroPad, int LeftJustify, struct OutputStream *Stream)
{
    char Result[33];
    int ResPos = sizeof(Result);

    Result[--ResPos] = '\0';
    if (Num == 0)
        Result[--ResPos] = '0';
            
    while (Num > 0)
    {
        unsigned long NextNum = Num / Base;
        unsigned long Digit = Num - NextNum * Base;
        if (Digit < 10)
            Result[--ResPos] = '0' + Digit;
        else
            Result[--ResPos] = 'a' + Digit - 10;
        
        Num = NextNum;
    }
    
    if (FieldWidth > 0 && !LeftJustify)
        PrintRepeatedChar(ZeroPad ? '0' : ' ', FieldWidth - (sizeof(Result) - 1 - ResPos), Stream);
        
    PrintStr(&Result[ResPos], Stream);

    if (FieldWidth > 0 && LeftJustify)
        PrintRepeatedChar(' ', FieldWidth - (sizeof(Result) - 1 - ResPos), Stream);
}

/* print an integer to a stream without using printf/sprintf */
void PrintSimpleInt(long Num, struct OutputStream *Stream)
{
    PrintInt(Num, -1, FALSE, FALSE, Stream);
}

/* print an integer to a stream without using printf/sprintf */
void PrintInt(long Num, int FieldWidth, int ZeroPad, int LeftJustify, struct OutputStream *Stream)
{
    if (Num < 0)
    {
        PrintCh('-', Stream);
        Num = -Num;
        if (FieldWidth != 0)
            FieldWidth--;
    }
    
    PrintUnsigned((unsigned long)Num, 10, FieldWidth, ZeroPad, LeftJustify, Stream);
}

#ifndef NO_FP
/* print a double to a stream without using printf/sprintf */
void PrintFP(double Num, struct OutputStream *Stream)
{
    int Exponent = 0;
    int MaxDecimal;
    
    if (Num < 0)
    {
        PrintCh('-', Stream);
        Num = -Num;    
    }
    
    if (Num >= 1e7)
        Exponent = log10(Num);
    else if (Num <= 1e-7 && Num != 0.0)
        Exponent = log10(Num) - 0.999999999;
    
    Num /= pow(10.0, Exponent);    
    PrintInt((long)Num, 0, FALSE, FALSE, Stream);
    PrintCh('.', Stream);
    Num = (Num - (long)Num) * 10;
    if (abs(Num) >= 1e-7)
    {
        for (MaxDecimal = 6; MaxDecimal > 0 && abs(Num) >= 1e-7; Num = (Num - (long)(Num + 1e-7)) * 10, MaxDecimal--)
            PrintCh('0' + (long)(Num + 1e-7), Stream);
    }
    else
        PrintCh('0', Stream);
        
    if (Exponent != 0)
    {
        PrintCh('e', Stream);
        PrintInt(Exponent, 0, FALSE, FALSE, Stream);
    }
}
#endif

/* intrinsic functions made available to the language */
void GenericPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs, struct OutputStream *Stream)
{
    char *FPos;
    struct Value *NextArg = Param[0];
    struct ValueType *FormatType;
    int ArgCount = 1;
    int LeftJustify = FALSE;
    int ZeroPad = FALSE;
    int FieldWidth = 0;
    char *Format = Param[0]->getVal<PointerType>(pc);
    
    for (FPos = Format; *FPos != '\0'; FPos++)
    {
        if (*FPos == '%')
        {
            FPos++;
	    FieldWidth = 0;
            if (*FPos == '-')
            {
                /* a leading '-' means left justify */
                LeftJustify = TRUE;
                FPos++;
            }
            
            if (*FPos == '0')
            {
                /* a leading zero means zero pad a decimal number */
                ZeroPad = TRUE;
                FPos++;
            }
            
            /* get any field width in the format */
            while (isdigit((int)*FPos))
                FieldWidth = FieldWidth * 10 + (*FPos++ - '0');
            
            /* now check the format type */
            switch (*FPos)
            {
                case 's': FormatType = CharPtrType; break;
                case 'd': case 'u': case 'x': case 'b': case 'c': FormatType = &IntType; break;
#ifndef NO_FP
                case 'f': FormatType = &FPType; break;
#endif
                case '%': PrintCh('%', Stream); FormatType = NULL; break;
                case '\0': FPos--; FormatType = NULL; break;
                default:  PrintCh(*FPos, Stream); FormatType = NULL; break;
            }
            
            if (FormatType != NULL)
            { 
                /* we have to format something */
                if (ArgCount >= NumArgs)
                    PrintStr("XXX", Stream);   /* not enough parameters for format */
                else
                {
                    NextArg = (struct Value *)((char *)NextArg + MEM_ALIGN(sizeof(struct Value) + TypeStackSizeValue(NextArg)));
                    if (NextArg->TypeOfAnyValue() != FormatType && 
                            !((FormatType == &IntType || *FPos == 'f') && IS_NUMERIC_COERCIBLE(NextArg)) &&
                            !(FormatType == CharPtrType && (NextArg->TypeOfAnyValue()->Base == TypePointer || 
                                                             (NextArg->TypeOfAnyValue()->Base == TypeArray && NextArg->TypeOfAnyValue()->FromType->Base == TypeChar) ) ) )
                        PrintStr("XXX", Stream);   /* bad type for format */
                    else
                    {
                        switch (*FPos)
                        {
                            case 's':
                            {
                                char *Str;
                                
                                if (NextArg->TypeOfAnyValue()->Base == TypePointer)
                                    Str = NextArg->getVal<PointerType>(pc);
                                else
                                    Str = &NextArg->ValAddressOfData(pc);
                                    
                                if (Str == NULL)
                                    PrintStr("NULL", Stream); 
                                else
                                    PrintStr(Str, Stream); 
                                break;
                            }
                            case 'd': PrintInt(ExpressionCoerceInteger(NextArg), FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'u': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 10, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'x': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 16, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'b': PrintUnsigned(ExpressionCoerceUnsignedInteger(NextArg), 2, FieldWidth, ZeroPad, LeftJustify, Stream); break;
                            case 'c': PrintCh(ExpressionCoerceUnsignedInteger(NextArg), Stream); break;
#ifndef NO_FP
                            case 'f': PrintFP(ExpressionCoerceFP(NextArg), Stream); break;
#endif
                        }
                    }
                }
                
                ArgCount++;
            }
        }
        else
            PrintCh(*FPos, Stream);
    }
}

/* printf(): print to console output */
void LibPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream ConsoleStream;
    
    ConsoleStream.Putch = &PlatformPutc;
    GenericPrintf(Parser, ReturnValue, Param, NumArgs, &ConsoleStream);
}

/* sprintf(): print to a string */
void LibSPrintf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    struct OutputStream StrStream;
    
    StrStream.Putch = &SPutc;
    StrStream.i.Str.Parser = Parser;
    StrStream.i.Str.WritePos = Param[0]->getVal<PointerType>(pc);

    GenericPrintf(Parser, ReturnValue, Param+1, NumArgs-1, &StrStream);
    PrintCh(0, &StrStream);
    ReturnValue->setVal<PointerType>(pc,  *Param;
}

/* get a line of input. protected from buffer overrun */
void LibGets(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<PointerType>(pc,  PlatformGetLine(Param[0]->getVal<PointerType>(pc), GETS_BUF_MAX, NULL);
    if (ReturnValue->getVal<PointerType>(pc) != NULL)
    {
        char *EOLPos = strchr(Param[0]->getVal<PointerType>(pc), '\n');
        if (EOLPos != NULL)
            *EOLPos = '\0';
    }
}

void LibGetc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<int>(pc, PlatformGetCharacter());
}

void LibExit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    PlatformExit(Param[0]->getVal<int>(pc));
}

#ifdef PICOC_LIBRARY
void LibSin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  sin(Param[0]->getVal<double>(pc));
}

void LibCos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  cos(Param[0]->getVal<double>(pc));
}

void LibTan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  tan(Param[0]->getVal<double>(pc));
}

void LibAsin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  asin(Param[0]->getVal<double>(pc));
}

void LibAcos(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  acos(Param[0]->getVal<double>(pc));
}

void LibAtan(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  atan(Param[0]->getVal<double>(pc));
}

void LibSinh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  sinh(Param[0]->getVal<double>(pc));
}

void LibCosh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  cosh(Param[0]->getVal<double>(pc));
}

void LibTanh(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  tanh(Param[0]->getVal<double>(pc));
}

void LibExp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  exp(Param[0]->getVal<double>(pc));
}

void LibFabs(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  fabs(Param[0]->getVal<double>(pc));
}

void LibLog(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  log(Param[0]->getVal<double>(pc));
}

void LibLog10(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  log10(Param[0]->getVal<double>(pc));
}

void LibPow(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  pow(Param[0]->getVal<double>(pc), Param[1]->getVal<double>(pc));
}

void LibSqrt(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  sqrt(Param[0]->getVal<double>(pc));
}

void LibRound(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  floor(Param[0]->getVal<double>(pc) + 0.5);   /* XXX - fix for soft float */
}

void LibCeil(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  ceil(Param[0]->getVal<double>(pc));
}

void LibFloor(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<double>(pc,  floor(Param[0]->getVal<double>(pc));
}
#endif

#ifndef NO_STRING_FUNCTIONS
void LibMalloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<PointerType>(pc,  malloc(Param[0]->getVal<int>(pc));
}

#ifndef NO_CALLOC
void LibCalloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<PointerType>(pc,  calloc(Param[0]->getVal<int>(pc), Param[1]->getVal<int>(pc));
}
#endif

#ifndef NO_REALLOC
void LibRealloc(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->setVal<PointerType>(pc,  realloc(Param[0]->getVal<PointerType>(pc), Param[1]->getVal<int>(pc));
}
#endif

void LibFree(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    free(Param[0]->getVal<PointerType>(pc));
}

void LibStrcpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *To = (char *)Param[0]->getVal<PointerType>(pc);
    char *From = (char *)Param[1]->getVal<PointerType>(pc);
    
    while (*From != '\0')
        *To++ = *From++;
    
    *To = '\0';
}

void LibStrncpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *To = (char *)Param[0]->getVal<PointerType>(pc);
    char *From = (char *)Param[1]->getVal<PointerType>(pc);
    int Len = Param[2]->getVal<int>(pc);
    
    for (; *From != '\0' && Len > 0; Len--)
        *To++ = *From++;
    
    if (Len > 0)
        *To = '\0';
}

void LibStrcmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Str1 = (char *)Param[0]->getVal<PointerType>(pc);
    char *Str2 = (char *)Param[1]->getVal<PointerType>(pc);
    int StrEnded;
    
    for (StrEnded = FALSE; !StrEnded; StrEnded = (*Str1 == '\0' || *Str2 == '\0'), Str1++, Str2++)
    {
         if (*Str1 < *Str2) { ReturnValue->setVal<int>(pc, -1); return; } 
         else if (*Str1 > *Str2) { ReturnValue->setVal<int>(pc, 1); return; }
    }
    
    ReturnValue->setVal<int>(pc, 0);
}

void LibStrncmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Str1 = (char *)Param[0]->getVal<PointerType>(pc);
    char *Str2 = (char *)Param[1]->getVal<PointerType>(pc);
    int Len = Param[2]->getVal<int>(pc);
    int StrEnded;
    
    for (StrEnded = FALSE; !StrEnded && Len > 0; StrEnded = (*Str1 == '\0' || *Str2 == '\0'), Str1++, Str2++, Len--)
    {
         if (*Str1 < *Str2) { ReturnValue->setVal<int>(pc, -1); return; } 
         else if (*Str1 > *Str2) { ReturnValue->setVal<int>(pc, 1); return; }
    }
    
    ReturnValue->setVal<int>(pc, 0);
}

void LibStrcat(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *To = (char *)Param[0]->getVal<PointerType>(pc);
    char *From = (char *)Param[1]->getVal<PointerType>(pc);
    
    while (*To != '\0')
        To++;
    
    while (*From != '\0')
        *To++ = *From++;
    
    *To = '\0';
}

void LibIndex(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Pos = (char *)Param[0]->getVal<PointerType>(pc);
    int SearchChar = Param[1]->getVal<int>(pc);

    while (*Pos != '\0' && *Pos != SearchChar)
        Pos++;
    
    if (*Pos != SearchChar)
        ReturnValue->setVal<PointerType>(pc,  NULL;
    else
        ReturnValue->setVal<PointerType>(pc,  Pos;
}

void LibRindex(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Pos = (char *)Param[0]->getVal<PointerType>(pc);
    int SearchChar = Param[1]->getVal<int>(pc);

    ReturnValue->setVal<PointerType>(pc,  NULL;
    for (; *Pos != '\0'; Pos++)
    {
        if (*Pos == SearchChar)
            ReturnValue->setVal<PointerType>(pc,  Pos;
    }
}

void LibStrlen(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    char *Pos = (char *)Param[0]->getVal<PointerType>(pc);
    int Len;
    
    for (Len = 0; *Pos != '\0'; Pos++)
        Len++;
    
    ReturnValue->setVal<int>(pc, Len);
}

void LibMemset(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    /* we can use the system memset() */
    memset(Param[0]->getVal<PointerType>(pc), Param[1]->getVal<int>(pc), Param[2]->getVal<int>(pc));
}

void LibMemcpy(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    /* we can use the system memcpy() */
    memcpy(Param[0]->getVal<PointerType>(pc), Param[1]->getVal<PointerType>(pc), Param[2]->getVal<int>(pc));
}

void LibMemcmp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    unsigned char *Mem1 = (unsigned char *)Param[0]->getVal<PointerType>(pc);
    unsigned char *Mem2 = (unsigned char *)Param[1]->getVal<PointerType>(pc);
    int Len = Param[2]->getVal<int>(pc);
    
    for (; Len > 0; Mem1++, Mem2++, Len--)
    {
         if (*Mem1 < *Mem2) { ReturnValue->setVal<int>(pc, -1); return; } 
         else if (*Mem1 > *Mem2) { ReturnValue->setVal<int>(pc, 1); return; }
    }
    
    ReturnValue->setVal<int>(pc, 0);
}
#endif

/* list of all library functions and their prototypes */
struct LibraryFunction CLibrary[] =
{
    { LibPrintf,        "void printf(char *, ...);" },
    { LibSPrintf,       "char *sprintf(char *, char *, ...);" },
    { LibGets,          "char *gets(char *);" },
    { LibGetc,          "int getchar();" },
    { LibExit,          "void exit(int);" },
#ifdef PICOC_LIBRARY
    { LibSin,           "float sin(float);" },
    { LibCos,           "float cos(float);" },
    { LibTan,           "float tan(float);" },
    { LibAsin,          "float asin(float);" },
    { LibAcos,          "float acos(float);" },
    { LibAtan,          "float atan(float);" },
    { LibSinh,          "float sinh(float);" },
    { LibCosh,          "float cosh(float);" },
    { LibTanh,          "float tanh(float);" },
    { LibExp,           "float exp(float);" },
    { LibFabs,          "float fabs(float);" },
    { LibLog,           "float log(float);" },
    { LibLog10,         "float log10(float);" },
    { LibPow,           "float pow(float,float);" },
    { LibSqrt,          "float sqrt(float);" },
    { LibRound,         "float round(float);" },
    { LibCeil,          "float ceil(float);" },
    { LibFloor,         "float floor(float);" },
#endif
    { LibMalloc,        "void *malloc(int);" },
#ifndef NO_CALLOC
    { LibCalloc,        "void *calloc(int,int);" },
#endif
#ifndef NO_REALLOC
    { LibRealloc,       "void *realloc(void *,int);" },
#endif
    { LibFree,          "void free(void *);" },
#ifndef NO_STRING_FUNCTIONS
    { LibStrcpy,        "void strcpy(char *,char *);" },
    { LibStrncpy,       "void strncpy(char *,char *,int);" },
    { LibStrcmp,        "int strcmp(char *,char *);" },
    { LibStrncmp,       "int strncmp(char *,char *,int);" },
    { LibStrcat,        "void strcat(char *,char *);" },
    { LibIndex,         "char *index(char *,int);" },
    { LibRindex,        "char *rindex(char *,int);" },
    { LibStrlen,        "int strlen(char *);" },
    { LibMemset,        "void memset(void *,int,int);" },
    { LibMemcpy,        "void memcpy(void *,void *,int);" },
    { LibMemcmp,        "int memcmp(void *,void *,int);" },
#endif
    { NULL,             NULL }
};

#endif /* BUILTIN_MINI_STDLIB */
