#ifndef GEN_STRUCT_H
#define GEN_STRUCT_H

typedef enum TokenTypes TokenTypes;
enum TokenTypes
{
		Token_Unknown,
		Token_SpecialProcess,
		Token_Template,
		Token_TemplateStart,
		Token_TemplateEnd,
		Token_TemplateTypeName,
		Token_TemplateName,
		Token_TemplateNameStatement,
		Token_TemplateTypeIndicator,
		Token_Comment,
		Token_GenStructName,
		Token_TemplateType,
		Token_Identifier,
		Token_Whitespace,
		Token_ParentheticalOpen,
		Token_ParentheticalClose,
		Token_BracketOpen,
		Token_BracketClose,
		Token_Semicolon,
		Token_FeedSymbol,
		Token_EndOfFile
};

typedef struct Token Token;
struct Token
{
		TokenTypes token_type;
		char *token_data;
};

typedef struct Tokenizer Tokenizer;
struct Tokenizer
{
		u32 token_num;
		Token *tokens;
		Token *at;
};

typedef struct Template Template;
struct Template
{
		char *template_name;
		char *template_type_name;
    
		Tokenizer tokenizer;
    
		/* in case of name collisions */
		Template *next;
};

typedef struct TemplateHashTable TemplateHashTable;
struct TemplateHashTable
{
		Template *templates;
		u32 num;
};

typedef struct MemoryArena MemoryArena;
struct MemoryArena
{
		void *memory;
    
		u32 offset;
		u32 size;
		u32 size_left;
};

typedef struct TypeRequest TypeRequest;
struct TypeRequest
{
		char *template_name;
    
		char *type_name;
		char *struct_name;
};

typedef struct TemplateTypeRequest TemplateTypeRequest;
struct TemplateTypeRequest
{
		TypeRequest *type_requests;
    
		u32 request_num;
};

static u64 GetHash(char *str);
static void InitArena(u32 size);
static void FreeArena();
static void *ArenaAlloc(u32 size);
static void ClearArena();
static void CopyStringRange(char *input_string, char *output_string, u32 start, u32 end);
static void PrintTokenType(Token token, FILE *file);
static b8 PrintTokenString(Token token, FILE *file);
static void PrintTokenizerAt(Tokenizer *tokenizer, FILE *file);
static void ResetTokenizer(Tokenizer *tokenizer);
static b8 IncrementTokenizerNoWhitespace(Tokenizer *tokenizer);
static b8 IncrementTokenizerAll(Tokenizer *tokenizer);
static Token *GetTokenizerAt(Tokenizer *tokenizer);
static void WriteTemplateToFile(Template *templates, FILE *file);
static u32 GetNumberOfTemplates(Tokenizer *tokenizer);
static char *GetTemplateName(Tokenizer *tokenizer);
static char *GetTemplateTypeName(Tokenizer *tokenizer);
static Template GetTemplateFromTokens(Tokenizer *tokenizer);
static TemplateHashTable GetTemplateHashTable(Tokenizer *tokenizer);
static Template LookupHashTable(char *template_name, TemplateHashTable *hash_table);
static u32 GetNumberOfTemplateTypeRequests(Tokenizer *tokenizer);
static TemplateTypeRequest GetTemplateTypeRequests(Tokenizer *file_tokens);
static void ReplaceTypeName(Template *templates, char *type_name, char *struct_name);
static char *GetStringToNextWhitespace(Token *tokens, char *file_data, u32 *start_index);
static char *GetStringToNextNonWhitespace(Token *tokens, char *file_data, u32 *start_index);
static Tokenizer TokenizeFileData(char *file_data);
static char *GetFilenameNoExt(char *file_path);
static char *GetFileExt(char *file_path);
static char *GetFileWorkingDir(char *file_path);
static char *ReadFileData(char *file_path);
static void GenCode(u32 arg_count, char ** args);

#endif
