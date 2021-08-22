/* TODO(winston): error output */
/* TODO(winston): efficient memory usage */
/* TODO(winston): one-pass lexing */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "layer.h"
#include "gen_struct.h"

#ifdef _WIN32
#include "win32/win32_platform.c"
#elif __linux__
#include "linux/linux_platform.c"
#endif

/* utility macro for getting the range between two indices */
#define get_range(start, end) (end - start)

/* utility macro for allocating array */
#define alloc_array(size, array_num) \
arena_alloc(size * array_num)

static MemoryArena arena = {0};
static char file_ext[16];

/* djb2 hash function for string hashing */
static u64
get_hash(char *str)
{
	u64 hash = 5381;
	i32 c = 0;
    
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

/* initializes static arena for program use */
static void
init_arena(u32 size)
{
    void *memory = request_mem(size);
    void_check(memory);
    
	arena.memory = memory;
	arena.size = size;
	arena.size_left = size;
}

static void
free_arena()
{
    free_mem(arena.memory, arena.size);
    
    arena.memory = 0;
	arena.size = 0;
	arena.size_left = 0;
}

/*
allocates specified amount of memory and
returns a pointer to the start of the memory
*/
static void *
arena_alloc(u32 size)
{
	++size;
    
	void *result = 0;
    
	assert(arena.size_left >= size);
    
	result = &((cast(arena.memory, char *))[arena.offset]);
    
	arena.offset += size;
	arena.size_left -= size;
	memset(result, 0, size);
    
	return result;
}

/* resets arena offset but does not zero memory */
static void
clear_arena()
{
    arena.size_left = arena.size;
    arena.offset = 0;
}

/* copies a specific range of input_string */
static void
copy_string_range(char *input_string, char *output_string,
                  u32 start, u32 end)
{
    u32 string_length = get_range(start, end);
    
	for (u32 i = 0; i < string_length; ++i) {
		output_string[i] = input_string[i + start];
	}
    
	output_string[string_length] = '\0';
}

/*
 debug printing for tokenizer
 prints token type to specified FILE pointer.
 utility function for print_token_string
 */
static void
print_token_type(Token token, FILE *file)
{
	switch (token.token_type)
    {
#define token_print_case(token_const)                             \
    case token_const:                                             \
    {                                                             \
        fprintf(file, "%s: ", #token_const);                      \
    } break
        
        token_print_case(Token_Template);
        token_print_case(Token_TemplateStart);
        token_print_case(Token_TemplateEnd);
        token_print_case(Token_TemplateTypeName);
        token_print_case(Token_TemplateType);
        token_print_case(Token_TemplateName);
        token_print_case(Token_TemplateNameStatement);
        token_print_case(Token_TemplateTypeIndicator);
        token_print_case(Token_GenStructName);
        token_print_case(Token_Identifier);
        token_print_case(Token_Whitespace);
        token_print_case(Token_BracketOpen);
        token_print_case(Token_BracketClose);
        token_print_case(Token_ParentheticalOpen);
        token_print_case(Token_ParentheticalClose);
        token_print_case(Token_Semicolon);
        token_print_case(Token_EndOfFile);
        token_print_case(Token_FeedSymbol);
#undef token_print_case
        default:
        {
            fprintf(file, "Unknown token: ");
        } break;
    }
}

/*
Prints the string that token is holding to specified stream.
Returns FALSE if end of file and TRUE if anything else.
*/
static b8
print_token_string(Token token, FILE *file)
{
    if (token.token_type == Token_EndOfFile)
    {
        return FALSE;
    }
    
    if (token.token_type != Token_Whitespace)
    {
        fprintf(file, "%s", token.token_data);
        goto print_success;
    }
    
    for (u32 i = 0; i < strlen(token.token_data); ++i)
    {
        if (token.token_data[i] == '\n')
        {
            fprintf(file, "\\n");
        }
        else if (token.token_data[i] == '\t')
        {
            fprintf(file, "\\t");
        }
        else if (token.token_data[i] == ' ')
        {
            fprintf(file, "<space>");
        }
        else
        {
            fprintf(file, "%c", token.token_data[i]);
        }
    }
    
    print_success:
    return TRUE;
}

/* prints the at pointer of the tokenizer */
static void
print_tokenizer_at(Tokenizer *tokenizer, FILE *file)
{
    print_token_type(*(tokenizer->at), file);
    print_token_string(*(tokenizer->at), file);
    fprintf(file, "\n");
}

/* resets the at pointer of the tokenizer to the starting point */
static void
reset_tokenizer(Tokenizer *tokenizer)
{
    tokenizer->at = tokenizer->tokens;
}

/*
Increments the tokenizer at pointer, skips all whitespace and semicolons
Returns FALSE if hits end of file or gets out of array bounds
Returns TRUE if successfully incremented
*/
static b8
increment_tokenizer_no_whitespace(Tokenizer *tokenizer)
{
    do
    {
        if (tokenizer->at->token_type == Token_EndOfFile ||
            (tokenizer->at - tokenizer->tokens) >= tokenizer->token_num)
        {
            return FALSE;
        }
        ++tokenizer->at;
    } while (tokenizer->at->token_type == Token_Whitespace ||
             tokenizer->at->token_type == Token_Semicolon);
    
    return TRUE;
}

/*
 Increments tokenizer by token, skipping nothing
 Returns FALSE if hits end of file or out of array bounds
 Returns TRUE if successfully incremented
 */
static b8
increment_tokenizer_all(Tokenizer *tokenizer)
{
    if (tokenizer->at->token_type == Token_EndOfFile ||
        (tokenizer->at - tokenizer->tokens) >= tokenizer->token_num)
    {
        return FALSE;
    }
    
    ++tokenizer->at;
    return TRUE;
}

/*
 Utility function for getting the at pointer of the tokenizer
 Mainly for looks, not really needed though
 */
static Token *
get_tokenizer_at(Tokenizer *tokenizer)
{
    return tokenizer->at;
}


/*
 Writes contents of a template to specified file
 */
static void
write_template_to_file(Template *templates, FILE *file)
{
    reset_tokenizer(&templates->tokenizer);
    
    do
    {
        if (get_tokenizer_at(&templates->tokenizer)->token_type == Token_TemplateStart)
        {
            increment_tokenizer_no_whitespace(&templates->tokenizer);
            increment_tokenizer_no_whitespace(&templates->tokenizer);
            increment_tokenizer_no_whitespace(&templates->tokenizer);
            increment_tokenizer_no_whitespace(&templates->tokenizer);
        }
        else if (get_tokenizer_at(&templates->tokenizer)->token_type == Token_TemplateEnd)
        {
            break;
        }
        
        fprintf(file, "%s", get_tokenizer_at(&templates->tokenizer)->token_data);
    } while (increment_tokenizer_all(&templates->tokenizer));
    reset_tokenizer(&templates->tokenizer);
}

/*
 Gets the number of templates in a file
 Takes in a tokenized file
 Returns a u32 with the number of templates in file
 */
static u32
get_number_of_templates(Tokenizer *tokenizer)
{
    u32 count = 0;
    
    reset_tokenizer(tokenizer);
    
    do
    {
        if (get_tokenizer_at(tokenizer)->token_type == Token_TemplateEnd)
        {
            ++count;
        }
    } while (increment_tokenizer_no_whitespace(tokenizer));
    
    reset_tokenizer(tokenizer);
    return count;
}

/*
 Gets the template name
 Takes in a tokenizer where the tokens pointer points to the first
 Token in the template
 */
static char *
get_template_name(Tokenizer *tokenizer)
{
    char *template_name = 0;
    
    reset_tokenizer(tokenizer);
    
    do
    {
        if (get_tokenizer_at(tokenizer)->token_type == Token_TemplateName)
        {
            template_name = get_tokenizer_at(tokenizer)->token_data;
            break;
        }
    } while (increment_tokenizer_no_whitespace(tokenizer));
    
    reset_tokenizer(tokenizer);
    return template_name;
}

/*
 Takes in a template tokenizer and
 Returns the name of template
 */
static char *
get_template_type_name(Tokenizer *tokenizer)
{
    char *type_name = 0;
    reset_tokenizer(tokenizer);
    
    do
    {
        if (get_tokenizer_at(tokenizer)->token_type == Token_TemplateTypeName)
        {
            type_name = get_tokenizer_at(tokenizer)->token_data;
            break;
        }
    } while(increment_tokenizer_no_whitespace(tokenizer));
    
    reset_tokenizer(tokenizer);
    
    return type_name;
}

/*
  Gets struct Template from tokenizer where tokenizer starts at the
 start of the template

TODO (winston): complete this function and maybe implement a more
 versatile usage where the parameter does not have to point to the first
 token in the template.
 */
static Template
get_template_from_tokens(Tokenizer *tokenizer)
{
    Template template = {0};
    
    template.template_name = get_template_name(tokenizer);
    template.template_type_name = get_template_type_name(tokenizer);
    
    Tokenizer template_tokenizer = {0};
    template_tokenizer.tokens = tokenizer->at;
    
    u32 range_start = 0;;
    u32 range_end = 0;
    
    reset_tokenizer(tokenizer);
    do
    {
        if (get_tokenizer_at(tokenizer)->token_type ==
            Token_TemplateEnd)
        {
            range_end = tokenizer->at - tokenizer->tokens;
            break;
        }
    } while (increment_tokenizer_no_whitespace(tokenizer));
    
    template_tokenizer.token_num = get_range(range_start, range_end);
    template.tokenizer = template_tokenizer;
    
    reset_tokenizer(tokenizer);
    return template;
}

/*
 Constructs a hash table from a file tokenizer
 */
static TemplateHashTable
get_template_hash_table(Tokenizer *tokenizer)
{
    TemplateHashTable hash_table = {0};
    hash_table.num = get_number_of_templates(tokenizer);
    hash_table.templates =
        alloc_array(sizeof(*hash_table.templates), hash_table.num);
    
    do
    {
        if (get_tokenizer_at(tokenizer)->token_type == Token_TemplateStart)
        {
            Tokenizer template_tokenizer = {0};
            template_tokenizer.token_num =
                tokenizer->tokens + tokenizer->token_num - tokenizer->at;
            template_tokenizer.tokens = tokenizer->at;
            template_tokenizer.at = tokenizer->at;
            
            Template template =
                get_template_from_tokens(&template_tokenizer);
            
            u32 bucket = get_hash(template.template_name) % hash_table.num;
            
            if (hash_table.templates[bucket].template_name == 0)
            {
                hash_table.templates[bucket] = template;
                continue;
            }
            
            Template template_at =
                hash_table.templates[bucket];
            for (;;)
            {
                if (template_at.next != 0) {
                    template_at = *(template_at.next);
                    continue;
                }
                
                template_at.next =
                    arena_alloc(sizeof(*(template_at.next)));
                *(template_at.next) = template;
                break;
            }
        }
    } while (increment_tokenizer_no_whitespace(tokenizer));
    return hash_table;
}

/*
Pretty intuitive.
This looks up the definition of the template in the template hash table.
*/

static Template
lookup_hash_table(char *template_name, TemplateHashTable *hash_table)
{
    Template template = {0};
    u32 bucket = get_hash(template_name) % hash_table->num;
    
    template = hash_table->templates[bucket];
    
    while (strcmp(template_name, template.template_name) != 0)
    {
        if (template.next == 0)
        {
            break;
        }
        
        template = *(template.next);
    }
    
    return template;
}

/*
 Gets number of template "type requests" in file
 TODO (winston): maybe integrate this into the lexing process
 To speed up things
 */
static u32
get_number_of_template_type_requests(Tokenizer *tokenizer)
{
    u32 increment_thing = 0;
    
    reset_tokenizer(tokenizer);
    do
    {
        if (get_tokenizer_at(tokenizer)->token_type == Token_Template)
        {
            ++increment_thing;
        }
    } while (increment_tokenizer_no_whitespace(tokenizer));
    reset_tokenizer(tokenizer);
    
    return increment_thing;
}

/*
 Gets the type of template requested
 */
static TemplateTypeRequest
get_template_type_requests(Tokenizer *file_tokens)
{
    TemplateTypeRequest type_request = {0};
    
    type_request.request_num =
        get_number_of_template_type_requests(file_tokens);
    type_request.type_requests =
        alloc_array(sizeof(*type_request.type_requests),
                    type_request.request_num);
    
    reset_tokenizer(file_tokens);
    u32 index = 0;
    do
    {
        if (get_tokenizer_at(file_tokens)->token_type == Token_Template)
        {
            TypeRequest type_request_at = {0};
            
            do
            {
                increment_tokenizer_no_whitespace(file_tokens);
                
                switch (get_tokenizer_at(file_tokens)->token_type)
                {
                    case Token_TemplateName:
                    {
                        type_request_at.template_name =
                            get_tokenizer_at(file_tokens)->token_data;
                    } break;
                    case Token_TemplateType:
                    {
                        type_request_at.type_name =
                            get_tokenizer_at(file_tokens)->token_data;
                    } break;
                    case Token_GenStructName:
                    {
                        type_request_at.struct_name =
                            get_tokenizer_at(file_tokens)->token_data;
                    } break;
                }
            } while (get_tokenizer_at(file_tokens)->token_type !=
                     Token_GenStructName);
            type_request.type_requests[index] = type_request_at;
            
            ++index;
        }
    } while (increment_tokenizer_no_whitespace(file_tokens));
    reset_tokenizer(file_tokens);
    
    return type_request;
}

/*
 Replace the type name in a template
 */
static void
replace_type_name(Template *templates, char *type_name, char *struct_name)
{
    reset_tokenizer(&templates->tokenizer);
    char *type_name_real = arena_alloc(strlen(type_name));
    strcpy(type_name_real, type_name);
    
    char *struct_name_real = arena_alloc(strlen(struct_name));
    strcpy(struct_name_real, struct_name);
    
    do
    {
        if (get_tokenizer_at(&templates->tokenizer)->token_type ==
            Token_TemplateTypeName)
        {
            get_tokenizer_at(&templates->tokenizer)->token_data =
                type_name_real;
        }
        else if (get_tokenizer_at(&templates->tokenizer)->token_type ==
                 Token_TemplateNameStatement)
        {
            get_tokenizer_at(&templates->tokenizer)->token_data = struct_name_real;
        }
    } while (increment_tokenizer_no_whitespace(&templates->tokenizer));
    reset_tokenizer(&templates->tokenizer);
}

/*
 Gets the pointer of the string to the next whitespace
 or to the next semicolon or parenthesis
 */
static char *
get_string_to_next_whitespace(Token *tokens,
                              char *file_data,
                              u32 *start_index)
{
    u32 range_start = *start_index;
    u32 range_end = *start_index;
    
    do
    {
        if (tokens[range_end].token_type == Token_EndOfFile)
        {
            break;
        }
        ++range_end;
    } while (tokens[range_end].token_type != Token_Whitespace &&
             tokens[range_end].token_type != Token_Semicolon &&
             tokens[range_end].token_type != Token_ParentheticalOpen &&
             tokens[range_end].token_type != Token_ParentheticalClose);
    
    char *token_string =
        arena_alloc(get_range(range_start, range_end));
    
    for (u32 j = 0; j < get_range(range_start, range_end); ++j)
    {
        token_string[j] = file_data[range_start + j];
    }
    
    token_string[get_range(range_start, range_end)] = '\0';
    
    *start_index = range_end - 1;
    
    return token_string;
}

/*
 Does the opposite of the previous function
 Allocates string and points it to the from the start
 and null-terminates at the next whitespace
 */
static char *
get_string_to_next_non_whitespace(Token *tokens,
                                  char *file_data,
                                  u32 *start_index)
{
    u32 range_start = *start_index;
    u32 range_end = *start_index;
    
    do
    {
        if (tokens[range_end].token_type == Token_EndOfFile)
        {
            break;
        }
        ++range_end;
    } while (tokens[range_end].token_type == Token_Whitespace &&
             tokens[range_end].token_type == Token_Semicolon &&
             tokens[range_end].token_type != Token_ParentheticalOpen &&
             tokens[range_end].token_type != Token_ParentheticalClose);
    
    char *token_string =
        arena_alloc(get_range(range_start, range_end));
    
    for (u32 j = 0; j < get_range(range_start, range_end); ++j)
    {
        token_string[j] = file_data[range_start + j];
    }
    
    token_string[get_range(range_start, range_end)] = '\0';
    
    *start_index = range_end - 1;
    
    return token_string;
}

/*
 Lexes the file and tokenizes all data

TODO (winston): split this into multiple functions
 to shorten main function
 */
static Tokenizer
tokenize_file_data(char *file_data)
{
    if(file_data == 0)
    {
        return (Tokenizer){0};
    }
    
    u32 file_data_length = strlen(file_data) + 1;
    
    Tokenizer tokenizer = {0};
    
    Token *tokens =
        alloc_array(sizeof(*tokens), file_data_length);
    
    for (u32 i = 0; i < file_data_length; ++i)
    {
        Token token = {0};
        
        switch (file_data[i])
        {
            case '\n':
            case '\r':
            case ' ':
            case '\t':
            {
                token.token_type = Token_Whitespace;
            } break;
            case '~':
            {
                token.token_type = Token_SpecialProcess;
            } break;
            case '@':
            {
                token.token_type = Token_Template;
            } break;
            case '\0':
            {
                token.token_type = Token_EndOfFile;
            } break;
            case '{':
            {
                token.token_type = Token_BracketOpen;
            } break;
            case '}':
            {
                token.token_type = Token_BracketClose;
            } break;
            case '(':
            {
                token.token_type = Token_ParentheticalOpen;
            } break;
            case ')':
            {
                token.token_type = Token_ParentheticalClose;
            } break;
            case ';':
            {
                token.token_type = Token_Semicolon;
            } break;
            default:
            {
                token.token_type = Token_Identifier;
            } break;
        }
        
        tokens[i] = token;
    }
    
    u32 counter = 0;
    for (u32 i = 0; i < file_data_length; ++i)
    {
        char *token_string = 0;
        
        switch (tokens[i].token_type)
        {
            case Token_Template:
            case Token_Semicolon:
            case Token_Identifier:
            case Token_SpecialProcess:
            {
                tokens[counter].token_type = tokens[i].token_type;
                
                token_string = get_string_to_next_whitespace(tokens,
                                                             file_data,
                                                             &i);
            } break;
            case Token_BracketOpen:
            case Token_BracketClose:
            case Token_ParentheticalOpen:
            case Token_ParentheticalClose:
            {
                tokens[counter].token_type = tokens[i].token_type;
                
                token_string = arena_alloc(2);
                
                token_string[0] = file_data[i];
                token_string[1] = '\0';
            } break;
            case Token_Whitespace:
            {
                tokens[counter].token_type = tokens[i].token_type;
                token_string =
                    get_string_to_next_non_whitespace(tokens,
                                                      file_data,
                                                      &i);
            } break;
            default:
            {
                tokens[counter].token_type = tokens[i].token_type;
            } break;
        }
        tokens[counter].token_data = token_string;
        
        ++counter;
    }
    
    u32 tokens_actual_length = 0;
    for (u32 i = 0; i < file_data_length; ++i)
    {
        if (tokens[i].token_type == Token_EndOfFile)
        {
            tokens_actual_length = i + 1;
            break;
        }
        if (tokens[i].token_data[0] == '/' &&
            tokens[i].token_data[1] == '/')
        {
            do
            {
                TokenTypes type_before = tokens[i].token_type;
                tokens[i].token_type = Token_Comment;
                
                if (type_before == Token_Whitespace)
                {
                    b32 is_end_of_line = FALSE;
                    for (u32 j = 0; tokens[i].token_data[j] != '\0'; ++j) {
                        if (tokens[i].token_data[j] == '\n')
                        {
                            is_end_of_line = TRUE;
                            break;
                        }
                    }
                    
                    if (is_end_of_line == TRUE)
                    {
                        break;
                    }
                }
            } while (++i);
        }
        if (tokens[i].token_data[0] == '/' &&
            tokens[i].token_data[1] == '*')
        {
            do
            {
                TokenTypes type_before = tokens[i].token_type;
                tokens[i].token_type = Token_Comment;
                
                b32 is_end_of_comment = FALSE;
                for (u32 j = 0; tokens[i].token_data[j] != '\0';++j) {
                    if (tokens[i].token_data[j] == '*' &&
                        tokens[i].token_data[j + 1] == '/')
                    {
                        is_end_of_comment = TRUE;
                        break;
                    }
                }
                
                if (is_end_of_comment == TRUE)
                {
                    break;
                }
            } while (++i);
        }
        if (tokens[i].token_data[0] == '@')
        {
            if (strcmp(tokens[i].token_data, "@template_start") == 0)
            {
                tokens[i].token_type = Token_TemplateStart;
            }
            else if (strcmp(tokens[i].token_data, "@template_end") == 0)
            {
                tokens[i].token_type = Token_TemplateEnd;
            }
            else if (strcmp(tokens[i].token_data, "@template_name") == 0)
            {
                tokens[i].token_type = Token_TemplateNameStatement;
            }
            else if (strcmp(tokens[i].token_data, "@template") == 0)
            {
            }
            else
            {
                fprintf(stderr, "Unrecognized keyword: %s\n",
                        tokens[i].token_data);
                return tokenizer;
            }
            
            continue;
        }
        
        if (strcmp(tokens[i].token_data, "<-") == 0)
        {
            tokens[i].token_type = Token_FeedSymbol;
        }
        else if (strcmp(tokens[i].token_data, "->") == 0)
        {
            tokens[i].token_type = Token_TemplateTypeIndicator;
        }
        
        if (tokens[i].token_type == Token_SpecialProcess)
        {
            if (strcmp(tokens[i].token_data, "~output_ext") == 0)
            {
                while (tokens[++i].token_type == Token_Whitespace);
                strcpy(file_ext, tokens[i].token_data);
            }
        }
    }
    
    char *template_typename = 0;
    for (u32 i = 0; i < tokens_actual_length - 1; ++i)
    {
        if (tokens[i].token_type == Token_TemplateStart ||
            tokens[i].token_type == Token_Template)
        {
            while (tokens[++i].token_type == Token_Whitespace);
            tokens[i].token_type = Token_TemplateName;
        }
        if (tokens[i].token_type == Token_TemplateTypeIndicator)
        {
            while (tokens[++i].token_type == Token_Whitespace);
            tokens[i].token_type = Token_TemplateType;
            
            while (tokens[++i].token_type == Token_Whitespace);
            if(tokens[i].token_type == Token_TemplateTypeIndicator)
            {
                while (tokens[++i].token_type == Token_Whitespace);
                tokens[i].token_type = Token_GenStructName;
            }
        }
        if (tokens[i].token_type == Token_FeedSymbol)
        {
            while (tokens[++i].token_type == Token_Whitespace);
            
            tokens[i].token_type = Token_TemplateTypeName;
            template_typename = tokens[i].token_data;
            continue;
        }
        if (template_typename)
        {
            if (strcmp(tokens[i].token_data, template_typename) == 0)
            {
                tokens[i].token_type = Token_TemplateTypeName;
            }
        }
    }
    
    tokenizer.token_num = tokens_actual_length;
    tokenizer.tokens = tokens;
    tokenizer.at = tokenizer.tokens;
    
    return tokenizer;
}

/*
   Gets the file name from the path without the extension
  */
static char *
get_filename_no_ext(char *file_path)
{
    if(file_path == 0)
    {
        return 0;
    }
    
    u32 file_path_length = strlen(file_path);
    u32 filename_start = 0;
    u32 filename_end = 0;
    
    for (u32 i = 0; i < file_path_length; ++i)
    {
        if (file_path[i] == '\\' || file_path[i] == '/')
        {
            filename_start = i + 1;
        }
        else if ((file_path[i] == '.') && (file_path[i + 1] != '/'))
        {
            filename_end = i;
            break;
        }
    }
    
    u32 filename_length = get_range(filename_start, filename_end);
    char *filename = arena_alloc(filename_length + 1);
    
    copy_string_range(file_path, filename,
                      filename_start, filename_end);
    
    return filename;
}

/*
 Gets the extension of the file from the file path
 */
static char *
get_file_ext(char *file_path)
{
    if(file_path == 0)
    {
        return 0;
    }
    
    u32 file_path_length = strlen(file_path);
    
    u32 file_ext_start = 0;
    u32 file_ext_end = file_path_length;
    
    for (u32 i = 0; i < file_path_length; ++i)
    {
        if (file_path[i] == '.')
        {
            file_ext_start = i + 1;
            break;
        }
    }
    
    u32 file_ext_length = get_range(file_ext_start, file_ext_end);
    char *file_ext = arena_alloc(file_ext_length + 1);
    
    copy_string_range(file_path, file_ext,
                      file_ext_start, file_ext_end);
    
    return file_ext;
}

/*
 Gets the directory from the file path
 */
static char *
get_file_working_dir(char *file_path)
{
    if(file_path == 0)
    {
        return 0;
    }
    
    u32 file_path_length = strlen(file_path);
    
    u32 working_dir_start = 0;
    u32 working_dir_end = 0;
    
    for (u32 i = 0; i < file_path_length; ++i)
    {
        if (file_path[i] == '\\' || file_path[i] == '/')
        {
            working_dir_end = i + 1;
        }
    }
    
    u32 working_dir_length = get_range(working_dir_start, working_dir_end);
    char *working_dir = arena_alloc(working_dir_length + 1);
    
    copy_string_range(file_path, working_dir,
                      working_dir_start, working_dir_end);
    
    return working_dir;
}

static char *
read_file_data(char *file_path)
{
    FILE *file = fopen(file_path, "r");
    
    if (!file)
    {
        return 0;
    }
    
    fseek(file, 0, SEEK_END);
    u32 file_length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *file_contents = arena_alloc(file_length);
    
    fread(file_contents, 1, file_length, file);
    
    fclose(file);
    
    return file_contents;
}

static void
gen_code(u32 arg_count, char **args)
{
    for (u32 i = 1; i < arg_count; ++i)
    {
        char *file_path = args[i];
        char *filename_no_ext = get_filename_no_ext(file_path);
        char *file_working_dir = get_file_working_dir(file_path);
        char *output_file_path = (char *)arena_alloc(128);
        
        strcpy(output_file_path, file_working_dir);
        strcat(output_file_path, filename_no_ext);
        
        char *file_contents = read_file_data(file_path);
        
        if (file_contents == 0)
        {
            fprintf(stderr, "Failed to read file %s.\n", file_path);
            clear_arena();
            continue;
        }
        
        Tokenizer tokenizer = tokenize_file_data(file_contents);
        
        if (tokenizer.tokens == 0)
        {
            fprintf(stderr, "Failed to compile file.\n");
            clear_arena();
            continue;
        }
        
        if (file_ext[0] == '\0')
        {
            strcat(output_file_path, ".h");
        }
        else
        {
            strcat(output_file_path, file_ext);
        }
        
        TemplateHashTable hash_table =
            get_template_hash_table(&tokenizer);
        
        TemplateTypeRequest type_request =
            get_template_type_requests(&tokenizer);
        
        FILE *output_file = fopen(output_file_path, "w");
        
        for (u32 i = 0; i < type_request.request_num; ++i)
        {
            Template template_at =
                lookup_hash_table(type_request.type_requests[i].template_name,
                                  &hash_table);
            
            replace_type_name(&template_at,
                              type_request.type_requests[i].type_name,
                              type_request.type_requests[i].struct_name);
            
            if (template_at.template_name != 0)
            {
                write_template_to_file(&template_at, output_file);
            }
            
            fprintf(output_file, "\n");
        }
        
        fclose(output_file);

        file_ext[0] = '\0';
        clear_arena();
        printf("%s -> %s\n", file_path, output_file_path);
    }
}

i32
main(i32 arg_count, char **args)
{
    if (arg_count < 2)
    {
        fprintf(stderr, "Specify file name as first argument");
        return -1;
    }
    
    f32 time_start = get_time();
    init_arena(gigabytes((u32)2));
    gen_code(cast(arg_count, u32), args);
    free_arena();
    f32 time_end = get_time();
    
    //printf("Code generation in %f seconds.\n", time_end - time_start);
    return 0;
}
