#ifndef GEN_STRUCT_H
#define GEN_STRUCT_H

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

struct Token
{
		enum TokenTypes token_type;
		char *token_data;
};

struct Tokenizer
{
		u32 token_num;
		struct Token *tokens;
		struct Token *at;
};

struct Template
{
		char *template_name;
		char *template_type_name;
    
		struct Tokenizer tokenizer;
    
		/* in case of name collisions */
		struct Template *next;
};

struct TemplateHashTable
{
		struct Template *templates;
		u32 num;
};

struct MemoryArena
{
		void *memory;
    
		u32 offset;
		u32 size;
		u32 size_left;
};

struct TypeRequest
{
		char *template_name;
    
		char *type_name;
		char *struct_name;
};

struct TemplateTypeRequest
{
		struct TypeRequest *type_requests;
    
		u32 request_num;
};

static u64 get_hash(char *str);
static void init_arena(u32 size);
static void free_arena();
static void *arena_alloc(u32 size);
static void clear_arena();
static void copy_string_range(char *input_string, char *output_string, u32 start, u32 end);
static void print_token_type(struct Token token, FILE *file);
static b8 print_token_string(struct Token token, FILE *file);
static void print_tokenizer_at(struct Tokenizer *tokenizer, FILE *file);
static void reset_tokenizer(struct Tokenizer *tokenizer);
static b8 increment_tokenizer_no_whitespace(struct Tokenizer *tokenizer);
static b8 increment_tokenizer_all(struct Tokenizer *tokenizer);
static void write_template_to_file(struct Template *templates, FILE *file);
static u32 get_number_of_templates(struct Tokenizer *tokenizer);
static char *get_template_name(struct Tokenizer *tokenizer);
static char *get_template_type_name(struct Tokenizer *tokenizer);
static struct Template get_template_from_tokens(struct Tokenizer *tokenizer);
static struct TemplateHashTable get_template_hash_table(struct Tokenizer *tokenizer);
static struct Template lookup_hash_table(char *template_name, struct TemplateHashTable *hash_table);
static u32 get_number_of_template_type_requests(struct Tokenizer *tokenizer);
static struct TemplateTypeRequest get_template_type_requests(struct Tokenizer *file_tokens);
static void replace_type_name(struct Template *templates, char *type_name, char *struct_name);
static char *get_string_to_next_whitespace(struct Token *tokens, char *file_data, u32 *start_index);
static char *get_string_to_next_non_whitespace(struct Token *tokens, char *file_data, u32 *start_index);
static struct Tokenizer tokenize_file_data(char *file_data);
static char *get_filename_no_ext(char *file_path);
static char *get_file_ext(char *file_path);
static char *get_file_working_dir(char *file_path);
static char *read_file_data(char *file_path);
static void gen_code(u32 arg_count, char ** args);

#endif
