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
#define GetRange(start, end) (end - start)

/* utility macro for allocating array */
#define AllocArray(size, array_num) \
ArenaAlloc(size * array_num)

global MemoryArena arena = {0};
global char file_ext[16];

/* djb2 hash function for string hashing */
internal u64
GetHash(char *str)
{
	u64 hash = 5381;
	s32 c = 0;
    
	while ((c = *str++))
    {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

/* initializes global arena for program use */
internal void
InitArena(u32 size)
{
    void *memory = RequestMem(size);
    VOID_CHECK(memory);
    
	arena.memory = memory;
	arena.size = size;
	arena.size_left = size;
}

internal void
FreeArena()
{
    FreeMem(arena.memory, arena.size);
    
    arena.memory = 0;
	arena.size = 0;
	arena.size_left = 0;
}

/*
allocates specified amount of memory and
returns a pointer to the start of the memory
*/
internal void *
ArenaAlloc(u32 size)
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
internal void
ClearArena()
{
    arena.size_left = arena.size;
    arena.offset = 0;
}

/* copies a specific range of input_string */
internal void
CopyStringRange(char *input_string, char *output_string,
                u32 start, u32 end)
{
	u32 string_length = GetRange(start, end);
    
	for (u32 i = 0; i < string_length; ++i)
    {
		output_string[i] = input_string[i + start];
	}
    
	output_string[string_length] = '\0';
}

/*
 debug printing for tokenizer
 prints token type to specified FILE pointer.
 utility function for print_token_string
 */
internal void
PrintTokenType(Token token, FILE *file)
{
	switch (token.token_type)
    {
#define TOKEN_PRINT_CASE(token_const)                             \
case token_const:                                             \
{                                                             \
fprintf(file, "%s: ", #token_const);                      \
} break
        
        TOKEN_PRINT_CASE(Token_Template);
        TOKEN_PRINT_CASE(Token_TemplateStart);
        TOKEN_PRINT_CASE(Token_TemplateEnd);
        TOKEN_PRINT_CASE(Token_TemplateTypeName);
        TOKEN_PRINT_CASE(Token_TemplateType);
        TOKEN_PRINT_CASE(Token_TemplateName);
        TOKEN_PRINT_CASE(Token_TemplateNameStatement);
        TOKEN_PRINT_CASE(Token_TemplateTypeIndicator);
        TOKEN_PRINT_CASE(Token_GenStructName);
        TOKEN_PRINT_CASE(Token_Identifier);
        TOKEN_PRINT_CASE(Token_Whitespace);
        TOKEN_PRINT_CASE(Token_BracketOpen);
        TOKEN_PRINT_CASE(Token_BracketClose);
        TOKEN_PRINT_CASE(Token_ParentheticalOpen);
        TOKEN_PRINT_CASE(Token_ParentheticalClose);
        TOKEN_PRINT_CASE(Token_Semicolon);
        TOKEN_PRINT_CASE(Token_EndOfFile);
        TOKEN_PRINT_CASE(Token_FeedSymbol);
#undef TOKEN_PRINT_CASE
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
internal b8
PrintTokenString(Token token, FILE *file)
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
internal void
PrintTokenizerAt(Tokenizer *tokenizer, FILE *file)
{
    PrintTokenType(*(tokenizer->at), file);
    PrintTokenString(*(tokenizer->at), file);
    fprintf(file, "\n");
}

/* resets the at pointer of the tokenizer to the starting point */
internal void
ResetTokenizer(Tokenizer *tokenizer)
{
    tokenizer->at = tokenizer->tokens;
}

/*
Increments the tokenizer at pointer, skips all whitespace and semicolons
Returns FALSE if hits end of file or gets out of array bounds
Returns TRUE if successfully incremented
*/
internal b8
IncrementTokenizerNoWhitespace(Tokenizer *tokenizer)
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
internal b8
IncrementTokenizerAll(Tokenizer *tokenizer)
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
internal Token *
GetTokenizerAt(Tokenizer *tokenizer)
{
    return tokenizer->at;
}


/*
 Writes contents of a template to specified file
 */
internal void
WriteTemplateToFile(Template *templates, FILE *file)
{
    ResetTokenizer(&templates->tokenizer);
    
    do
    {
        if (GetTokenizerAt(&templates->tokenizer)->token_type == Token_TemplateStart)
        {
            IncrementTokenizerNoWhitespace(&templates->tokenizer);
            IncrementTokenizerNoWhitespace(&templates->tokenizer);
            IncrementTokenizerNoWhitespace(&templates->tokenizer);
            IncrementTokenizerNoWhitespace(&templates->tokenizer);
        }
        else if (GetTokenizerAt(&templates->tokenizer)->token_type == Token_TemplateEnd)
        {
            break;
        }
        
        fprintf(file, "%s", GetTokenizerAt(&templates->tokenizer)->token_data);
    } while (IncrementTokenizerAll(&templates->tokenizer));
    ResetTokenizer(&templates->tokenizer);
}

/*
 Gets the number of templates in a file
 Takes in a tokenized file
 Returns a u32 with the number of templates in file
 */
internal u32
GetNumberOfTemplates(Tokenizer *tokenizer)
{
    u32 count = 0;
    
    ResetTokenizer(tokenizer);
    
    do
    {
        if (GetTokenizerAt(tokenizer)->token_type == Token_TemplateEnd)
        {
            ++count;
        }
    } while (IncrementTokenizerNoWhitespace(tokenizer));
    
    ResetTokenizer(tokenizer);
    return count;
}

/*
 Gets the template name
 Takes in a tokenizer where the tokens pointer points to the first
 Token in the template
 */
internal char *
GetTemplateName(Tokenizer *tokenizer)
{
    char *template_name;
    
    ResetTokenizer(tokenizer);
    
    do
    {
        if (GetTokenizerAt(tokenizer)->token_type == Token_TemplateName)
        {
            template_name = GetTokenizerAt(tokenizer)->token_data;
            break;
        }
    } while (IncrementTokenizerNoWhitespace(tokenizer));
    
    ResetTokenizer(tokenizer);
    return template_name;
}

/*
 Takes in a template tokenizer and
 Returns the name of template
 */
internal char *
GetTemplateTypeName(Tokenizer *tokenizer)
{
    char *type_name = 0;
    ResetTokenizer(tokenizer);
    
    do
    {
        if (GetTokenizerAt(tokenizer)->token_type == Token_TemplateTypeName)
        {
            type_name = GetTokenizerAt(tokenizer)->token_data;
            break;
        }
    } while(IncrementTokenizerNoWhitespace(tokenizer));
    
    ResetTokenizer(tokenizer);
    
    return type_name;
}

/*
  Gets struct Template from tokenizer where tokenizer starts at the
 start of the template

TODO (winston): complete this function and maybe implement a more
 versatile usage where the parameter does not have to point to the first
 token in the template.
 */
internal Template
GetTemplateFromTokens(Tokenizer *tokenizer)
{
    Template template = {0};
    
    template.template_name = GetTemplateName(tokenizer);
    template.template_type_name = GetTemplateTypeName(tokenizer);
    
    Tokenizer template_tokenizer = {0};
    template_tokenizer.tokens = tokenizer->at;
    
    u32 range_start = 0;;
    u32 range_end = 0;
    
    ResetTokenizer(tokenizer);
    do
    {
        if (GetTokenizerAt(tokenizer)->token_type ==
            Token_TemplateEnd)
        {
            range_end = tokenizer->at - tokenizer->tokens;
            break;
        }
    } while (IncrementTokenizerNoWhitespace(tokenizer));
    
    template_tokenizer.token_num = GetRange(range_start, range_end);
    template.tokenizer = template_tokenizer;
    
    ResetTokenizer(tokenizer);
    return template;
}

/*
 Constructs a hash table from a file tokenizer
 */
internal TemplateHashTable
GetTemplateHashTable(Tokenizer *tokenizer)
{
    TemplateHashTable hash_table = {0};
    hash_table.num = GetNumberOfTemplates(tokenizer);
    hash_table.templates =
        AllocArray(sizeof(*hash_table.templates), hash_table.num);
    
    do
    {
        if (GetTokenizerAt(tokenizer)->token_type == Token_TemplateStart)
        {
            Tokenizer template_tokenizer = {0};
            template_tokenizer.token_num =
                tokenizer->tokens + tokenizer->token_num - tokenizer->at;
            template_tokenizer.tokens = tokenizer->at;
            template_tokenizer.at = tokenizer->at;
            
            Template template =
                GetTemplateFromTokens(&template_tokenizer);
            
            u32 bucket = GetHash(template.template_name) % hash_table.num;
            
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
                    ArenaAlloc(sizeof(*(template_at.next)));
                *(template_at.next) = template;
                break;
            }
        }
    } while (IncrementTokenizerNoWhitespace(tokenizer));
    return hash_table;
}

/*
Pretty intuitive.
This looks up the definition of the template in the template hash table.
*/

internal Template
LookupHashTable(char *template_name, TemplateHashTable *hash_table)
{
    Template template = {0};
    u32 bucket = GetHash(template_name) % hash_table->num;
    
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
internal u32
GetNumberOfTemplateTypeRequests(Tokenizer *tokenizer)
{
    u32 increment_thing = 0;
    
    ResetTokenizer(tokenizer);
    do
    {
        if (GetTokenizerAt(tokenizer)->token_type == Token_Template)
        {
            ++increment_thing;
        }
    } while (IncrementTokenizerNoWhitespace(tokenizer));
    ResetTokenizer(tokenizer);
    
    return increment_thing;
}

/*
 Gets the type of template requested
 */
internal TemplateTypeRequest
GetTemplateTypeRequests(Tokenizer *file_tokens)
{
    TemplateTypeRequest type_request = {0};
    
    type_request.request_num =
        GetNumberOfTemplateTypeRequests(file_tokens);
    type_request.type_requests =
        AllocArray(sizeof(*type_request.type_requests),
                   type_request.request_num);
    
    ResetTokenizer(file_tokens);
    u32 index = 0;
    do
    {
        if (GetTokenizerAt(file_tokens)->token_type == Token_Template)
        {
            TypeRequest type_request_at = {0};
            
            do
            {
                IncrementTokenizerNoWhitespace(file_tokens);
                
                switch (GetTokenizerAt(file_tokens)->token_type)
                {
                    case Token_TemplateName:
                    {
                        type_request_at.template_name =
                            GetTokenizerAt(file_tokens)->token_data;
                    } break;
                    case Token_TemplateType:
                    {
                        type_request_at.type_name =
                            GetTokenizerAt(file_tokens)->token_data;
                    } break;
                    case Token_GenStructName:
                    {
                        type_request_at.struct_name =
                            GetTokenizerAt(file_tokens)->token_data;
                    } break;
                }
            } while (GetTokenizerAt(file_tokens)->token_type !=
                     Token_GenStructName);
            type_request.type_requests[index] = type_request_at;
            
            ++index;
        }
    } while (IncrementTokenizerNoWhitespace(file_tokens));
    ResetTokenizer(file_tokens);
    
    return type_request;
}

/*
 Replace the type name in a template
 */
internal void
ReplaceTypeName(Template *templates, char *type_name, char *struct_name)
{
    ResetTokenizer(&templates->tokenizer);
    char *type_name_real = ArenaAlloc(strlen(type_name));
    strcpy(type_name_real, type_name);
    
    char *struct_name_real = ArenaAlloc(strlen(struct_name));
    strcpy(struct_name_real, struct_name);
    
    do
    {
        if (GetTokenizerAt(&templates->tokenizer)->token_type ==
            Token_TemplateTypeName)
        {
            GetTokenizerAt(&templates->tokenizer)->token_data =
                type_name_real;
        }
        else if (GetTokenizerAt(&templates->tokenizer)->token_type ==
                 Token_TemplateNameStatement)
        {
            GetTokenizerAt(&templates->tokenizer)->token_data = struct_name_real;
        }
    } while (IncrementTokenizerNoWhitespace(&templates->tokenizer));
    ResetTokenizer(&templates->tokenizer);
}

/*
 Gets the pointer of the string to the next whitespace
 or to the next semicolon or parenthesis
 */
internal char *
GetStringToNextWhitespace(Token *tokens,
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
        ArenaAlloc(GetRange(range_start, range_end));
    
    for (u32 j = 0; j < GetRange(range_start, range_end); ++j)
    {
        token_string[j] = file_data[range_start + j];
    }
    
    token_string[GetRange(range_start, range_end)] = '\0';
    
    *start_index = range_end - 1;
    
    return token_string;
}

/*
 Does the opposite of the previous function
 Allocates string and points it to the from the start
 and null-terminates at the next whitespace
 */
internal char *
GetStringToNextNonWhitespace(Token *tokens,
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
        ArenaAlloc(GetRange(range_start, range_end));
    
    for (u32 j = 0; j < GetRange(range_start, range_end); ++j)
    {
        token_string[j] = file_data[range_start + j];
    }
    
    token_string[GetRange(range_start, range_end)] = '\0';
    
    *start_index = range_end - 1;
    
    return token_string;
}

/*
 Lexes the file and tokenizes all data

TODO (winston): split this into multiple functions
 to shorten main function
 */
internal Tokenizer
TokenizeFileData(char *file_data)
{
    if(file_data == 0)
    {
        return (Tokenizer){0};
    }
    
    u32 file_data_length = strlen(file_data) + 1;
    
    Tokenizer tokenizer = {0};
    
    Token *tokens =
        AllocArray(sizeof(*tokens), file_data_length);
    
    for (u32 i = 0; i < file_data_length; ++i)
    {
        Token token = {0};
        
        switch (file_data[i])
        {
            case '\n':
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
                
                token_string = GetStringToNextWhitespace(tokens,
                                                         file_data,
                                                         &i);
            } break;
            case Token_BracketOpen:
            case Token_BracketClose:
            case Token_ParentheticalOpen:
            case Token_ParentheticalClose:
            {
                tokens[counter].token_type = tokens[i].token_type;
                
                token_string = ArenaAlloc(2);
                
                token_string[0] = file_data[i];
                token_string[1] = '\0';
            } break;
            case Token_Whitespace:
            {
                tokens[counter].token_type = tokens[i].token_type;
                token_string =
                    GetStringToNextNonWhitespace(tokens,
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
internal char *
GetFilenameNoExt(char *file_path)
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
    
    u32 filename_length = GetRange(filename_start, filename_end);
    char *filename = ArenaAlloc(filename_length + 1);
    
    CopyStringRange(file_path, filename,
                    filename_start, filename_end);
    
    return filename;
}

/*
 Gets the extension of the file from the file path
 */
internal char *
GetFileExt(char *file_path)
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
    
    u32 file_ext_length = GetRange(file_ext_start, file_ext_end);
    char *file_ext = ArenaAlloc(file_ext_length + 1);
    
    CopyStringRange(file_path, file_ext,
                    file_ext_start, file_ext_end);
    
    return file_ext;
}

/*
 Gets the directory from the file path
 */
internal char *
GetFileWorkingDir(char *file_path)
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
    
    u32 working_dir_length = GetRange(working_dir_start, working_dir_end);
    char *working_dir = ArenaAlloc(working_dir_length + 1);
    
    CopyStringRange(file_path, working_dir,
                    working_dir_start, working_dir_end);
    
    return working_dir;
}

internal char *
ReadFileData(char *file_path)
{
    FILE *file = fopen(file_path, "r");
    
    if (!file)
    {
        return 0;
    }
    
    fseek(file, 0, SEEK_END);
    u32 file_length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *file_contents = ArenaAlloc(file_length);
    
    fread(file_contents, 1, file_length, file);
    
    fclose(file);
    
    return file_contents;
}

internal void
GenCode(u32 arg_count, char **args)
{
    for (u32 i = 1; i < arg_count; ++i)
    {
        char *file_path = args[i];
        char *filename_no_ext = GetFilenameNoExt(file_path);
        char *file_working_dir = GetFileWorkingDir(file_path);
        char *output_file_path = (char *)ArenaAlloc(128);
        
        strcpy(output_file_path, file_working_dir);
        strcat(output_file_path, filename_no_ext);
        
        char *file_contents = ReadFileData(file_path);
        
        if (file_contents == 0)
        {
            fprintf(stderr, "Failed to read file %s.\n", file_path);
            ClearArena();
            continue;
        }
        
        Tokenizer tokenizer = TokenizeFileData(file_contents);
        
        if (tokenizer.tokens == 0)
        {
            fprintf(stderr, "Failed to compile file.\n");
            ClearArena();
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
            GetTemplateHashTable(&tokenizer);
        
        TemplateTypeRequest type_request =
            GetTemplateTypeRequests(&tokenizer);
        
        FILE *output_file = fopen(output_file_path, "w");
        
        for (u32 i = 0; i < type_request.request_num; ++i)
        {
            Template template_at =
                LookupHashTable(type_request.type_requests[i].template_name,
                                &hash_table);
            
            ReplaceTypeName(&template_at,
                            type_request.type_requests[i].type_name,
                            type_request.type_requests[i].struct_name);
            
            if (template_at.template_name != 0)
            {
                WriteTemplateToFile(&template_at, output_file);
            }
            
            fprintf(output_file, "\n");
        }
        
        fclose(output_file);
        
        file_ext[0] = '\0';
        ClearArena();
        
        printf("%s -> %s\n", file_path, output_file_path);
    }
}

s32
main(s32 arg_count, char **args)
{
    if (arg_count < 2)
    {
        fprintf(stderr, "Specify file name as first argument");
        return -1;
    }
    
    f64 time_start = GetTime();
    {
        InitArena(gigabytes((u32)2));
        GenCode(cast(arg_count, u32), args);
        FreeArena();
    }
    f64 time_end = GetTime();
    
    printf("Code generation succeeded in %f seconds.\n",
           time_end - time_start);
    
    return 0;
}
