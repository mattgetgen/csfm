/* csmf - alpha - USFM Parser
   by Matthew Getgen

   SECURITY

   FEATURE OVERVIEW

   COMPILING & LINKING

   API

   EXAMPLE USAGE

   VERSION HISTORY

   TODO

   LICENSE
     zlib/libpng license

     Copyright (c) 2025 Matthew Getgen

     This software is provided 'as-is', without any express or implied warranty.
     In no event will the authors be held liable for any damages arising from the
     use of this software.

     Permission is granted to anyone to use this software for any purpose,
     including commercial applications, and to alter it and redistribute it
     freely, subject to the following restrictions:

         1. The origin of this software must not be misrepresented; you must not
         claim that you wrote the original software. If you use this software in a
         product, an acknowledgment in the product documentation would be
         appreciated but is not required.

         2. Altered source versions must be plainly marked as such, and must not
         be misrepresented as being the original software.

         3. This notice may not be removed or altered from any source
         distribution.
 */

#ifndef CSFM
#define CSFM

#define CSFM_VERSION_MAJOR 0
#define CSFM_VERSION_MINOR 0
#define CSFM_VERSION_PATCH 0
#define CSFM_VERSION "0.0.0-dev"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    uint8_t *ptr;
    uint32_t length;
} CSFM_String8Slice;

static inline char CSFM_String8Slice_get(CSFM_String8Slice slice, uint32_t idx);

typedef enum {
    CSFM_TOKEN_EOF,
    CSFM_TOKEN_WS,
    CSFM_TOKEN_CR,
    CSFM_TOKEN_LF,
    CSFM_TOKEN_CRLF,
    CSFM_TOKEN_FORWARDSLASH,
    CSFM_TOKEN_DOUBLE_FORWARDSLASH,
    CSFM_TOKEN_BACKSLASH,
    CSFM_TOKEN_PIPE,
    CSFM_TOKEN_PERIOD,
    CSFM_TOKEN_COMMA,
    CSFM_TOKEN_COLON,
    CSFM_TOKEN_SEMICOLON,
    CSFM_TOKEN_TILDE,
    CSFM_TOKEN_ASTERISK,
    CSFM_TOKEN_PLUS,
    CSFM_TOKEN_MINUS,
    CSFM_TOKEN_EQUAL,
    CSFM_TOKEN_DOUBLE_QUOTE,
    CSFM_TOKEN_NUMBER,
    CSFM_TOKEN_TEXT
} CSFM_TokenType;

typedef struct {
    uint32_t start;
    uint32_t end;
    uint32_t line;
    uint32_t column;
    CSFM_TokenType type;
} CSFM_Token;

// NOTE(mattg): For testing purposes only
static inline void printTokenText(CSFM_Token *token, CSFM_String8Slice str) {
    uint32_t length = token->end - token->start;
    char *string = (char *)&str.ptr[token->start];
    printf("%*.*s", length, length, string);
}

typedef struct {
    CSFM_Token *buffer;
    uint32_t length;
    uint32_t capacity;
} CSFM_TokenArray;

int CSFM_TokenArray_allocate(CSFM_TokenArray *array, uint32_t capacity);
void CSFM_TokenArray_deallocate(CSFM_TokenArray *array);
void CSFM_TokenArray_reuse(CSFM_TokenArray *array);
static int CSFM_TokenArray_resize(CSFM_TokenArray *array, uint32_t newCapacity);
static int CSFM_TokenArray_push(CSFM_TokenArray *array, CSFM_Token token);
CSFM_Token* CSFM_TokenArray_get(CSFM_TokenArray array, uint32_t index);

typedef struct {
    CSFM_TokenArray array;
    uint32_t position;
    uint32_t line;
    uint32_t column;
} CSFM_Tokenizer;

static inline char CSFM_String8Slice_get(CSFM_String8Slice slice, uint32_t index) {
    if (index >= slice.length) {
        // NOTE(mattg): This should be at the end of the slice
        // anyway, so an EOF isn't that odd to return.
        return '\0';
    }
    return slice.ptr[index];
}

int CSFM_TokenArray_allocate(CSFM_TokenArray *array, uint32_t capacity) {
    if (array == NULL) {
        return -1;
    }
    size_t size = sizeof(CSFM_Token) * capacity;
    array->buffer = malloc(size);
    if (array->buffer == NULL) {
        array->capacity = 0;
        array->length = 0;
        return -1;
    }
    // TODO(mattg): look into what secure memset is, use it if I should
    memset(array->buffer, 0, size);
    array->capacity = capacity;
    array->length = 0;
    return 0;
}

void CSFM_TokenArray_deallocate(CSFM_TokenArray *array) {
    if (array == NULL) {
        return;
    }
    if (array->buffer != NULL) {
        free(array->buffer);
        array->buffer = NULL;
    }
    array->length = 0;
    array->capacity = 0;
}

void CSFM_TokenArray_reuse(CSFM_TokenArray *array) {
    if (array == NULL) {
        return;
    }
    if (array->buffer != NULL) {
        size_t size = sizeof(CSFM_Token) * array->capacity;
        memset(array->buffer, 0, size);
    }
    array->length = 0;
}

static int CSFM_TokenArray_resize(CSFM_TokenArray *array, uint32_t newCapacity) {
    if (array == NULL || newCapacity < array->length) {
        return -1;
    }
    size_t newSize = sizeof(CSFM_Token) * newCapacity;
    CSFM_Token *newBuffer = malloc(newSize);
    if (newBuffer == NULL) {
        return -1;
    }
    memset(newBuffer, 0, newSize);
    if (array->buffer != NULL) {
        memcpy(newBuffer, array->buffer, sizeof(CSFM_Token) * array->length);
        free(array->buffer);
    }
    array->buffer = newBuffer;
    array->capacity = newCapacity;
    return 0;
}

static int CSFM_TokenArray_push(CSFM_TokenArray *array, CSFM_Token token) {
    if (array == NULL) {
        return -1;
    }
    if (array->length >= array->capacity) {
        if (CSFM_TokenArray_resize(array, array->capacity * 2) != 0) {
            return -1;
        }
    }

    array->buffer[array->length] = token;
    array->length++;
    return 0;
}

CSFM_Token* CSFM_TokenArray_get(CSFM_TokenArray array, uint32_t index) {
    if (index < array.length) {
        return &array.buffer[index];
    }
    return NULL;
}

static inline int tokenTypeIsWhitespace(CSFM_TokenType type) {
    switch (type) {
    case CSFM_TOKEN_EOF:
    case CSFM_TOKEN_WS:
    case CSFM_TOKEN_CR:
    case CSFM_TOKEN_LF:
    case CSFM_TOKEN_CRLF:
        return 1;
    default:
        return 0;
    }
}

static inline CSFM_TokenType peekTokenType(CSFM_String8Slice str, uint32_t index) {
    CSFM_TokenType type = CSFM_TOKEN_EOF;
    switch (CSFM_String8Slice_get(str, index)) {
    case '\0':
        type = CSFM_TOKEN_EOF;
        break;
    case ' ':
    case '\t':
        type = CSFM_TOKEN_WS;
        break;
    case '\r':
        type = CSFM_TOKEN_CR;
        break;
    case '\n':
        type = CSFM_TOKEN_LF;
        break;
    case '/':
        type = CSFM_TOKEN_FORWARDSLASH;
        break;
    case '\\':
        type = CSFM_TOKEN_BACKSLASH;
        break;
    case '|':
        type = CSFM_TOKEN_PIPE;
        break;
    case '.':
        type = CSFM_TOKEN_PERIOD;
        break;
    case ',':
        type = CSFM_TOKEN_COMMA;
        break;
    case ':':
        type = CSFM_TOKEN_COLON;
        break;
    case ';':
        type = CSFM_TOKEN_SEMICOLON;
        break;
    case '~':
        type = CSFM_TOKEN_TILDE;
        break;
    case '*':
        type = CSFM_TOKEN_ASTERISK;
        break;
    case '+':
        type = CSFM_TOKEN_PLUS;
        break;
    case '-':
        type = CSFM_TOKEN_MINUS;
        break;
    case '=':
        type = CSFM_TOKEN_EQUAL;
        break;
    case '"':
        type = CSFM_TOKEN_DOUBLE_QUOTE;
        break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        type = CSFM_TOKEN_NUMBER;
        break;
    default:
        type = CSFM_TOKEN_TEXT;
    }
    
    return type;
}

static inline CSFM_Token peekToken(CSFM_String8Slice str, uint32_t index) {
    CSFM_Token token = {0};
    token.start = index;
    token.type = peekTokenType(str, index);
    CSFM_TokenType type = CSFM_TOKEN_EOF;
    switch (token.type) {
    // NOTE(mattg): These are 1 character tokens
    case CSFM_TOKEN_EOF:
    case CSFM_TOKEN_LF:
    // NOTE(mattg): CSFM_TOKEN_CRLF & CSFM_TOKEN_DOUBLE_FORWARDSLASH are never retured by peekTokenType
    case CSFM_TOKEN_CRLF:
    case CSFM_TOKEN_DOUBLE_FORWARDSLASH:
    case CSFM_TOKEN_BACKSLASH:
    case CSFM_TOKEN_PIPE:
    case CSFM_TOKEN_PERIOD:
    case CSFM_TOKEN_COMMA:
    case CSFM_TOKEN_COLON:
    case CSFM_TOKEN_SEMICOLON:
    case CSFM_TOKEN_TILDE:
    case CSFM_TOKEN_ASTERISK:
    case CSFM_TOKEN_PLUS:
    case CSFM_TOKEN_MINUS:
    case CSFM_TOKEN_EQUAL:
    case CSFM_TOKEN_DOUBLE_QUOTE:
        token.end = token.start + 1;
        break;
    // NOTE(mattg): These may be 1-2 character tokens
    case CSFM_TOKEN_CR:
        if (peekTokenType(str, index+1) == CSFM_TOKEN_LF) {
            token.type = CSFM_TOKEN_CRLF;
            token.end = token.start + 2;
        } else {
            token.end = token.start + 1;
        }
        break;
    case CSFM_TOKEN_FORWARDSLASH:
        if (peekTokenType(str, index+1) == CSFM_TOKEN_FORWARDSLASH) {
            token.type = CSFM_TOKEN_DOUBLE_FORWARDSLASH;
            token.end = token.start + 2;
        } else {
            token.end = token.start + 1;
        }
        break;
    // NOTE(mattg): These may be 1-N character tokens
    case CSFM_TOKEN_WS:
    case CSFM_TOKEN_NUMBER:
    case CSFM_TOKEN_TEXT:
        type = token.type;
        do {
            type = peekTokenType(str, ++index);
        } while (type == token.type);
        token.end = index;
    }
    return token;
}

static inline int consumeToken(CSFM_Tokenizer *tokenizer, CSFM_Token token) {
    token.line = tokenizer->line;
    token.column = tokenizer->column;
    if (CSFM_TokenArray_push(&tokenizer->array, token) != 0) {
        return -1;
    }
    tokenizer->position = token.end;
    if (token.type == CSFM_TOKEN_CR || token.type == CSFM_TOKEN_LF || token.type == CSFM_TOKEN_CRLF) {
        tokenizer->line++;
        tokenizer->column = 1;
    } else {
        tokenizer->column += (token.end - token.start); // push the column by the token length.
    }
    return 0;
}

static void tokenizeInternal(CSFM_Tokenizer *tokenizer, CSFM_String8Slice str) {
    tokenizer->line = 1;
    tokenizer->column = 1;
    CSFM_Token token = {0};
    do {
        token = peekToken(str, tokenizer->position);
        if (consumeToken(tokenizer, token) == -1) {
            break;
        }
    } while (token.type != CSFM_TOKEN_EOF);
    return;
}

CSFM_Tokenizer CSFM_Tokenize(uint8_t *buf, size_t size) {
    CSFM_String8Slice str = {
        .ptr = buf,
        .length = size,
    };

    CSFM_Tokenizer tokenizer = {0};
    if (CSFM_TokenArray_allocate(&tokenizer.array, size) != 0) {
        return tokenizer;
    }

    tokenizeInternal(&tokenizer, str);

    return tokenizer;
}

typedef enum {
    CSFM_NODE_NULL,
    CSFM_NODE_MARKER,
    CSFM_NODE_TEXT,
    CSFM_NODE_WHITESPACE,
    CSFM_NODE_NEWLINE,
} CSFM_NodeType;
//
// typedef enum {
//     CSFM_MARKER_TYPE_NORMAL,
//     CSFM_MARKER_TYPE_NESTED,
//     CSFM_MARKER_TYPE_CLOSE,
//     CSFM_MARKER_TYPE_NESTED_CLOSE,
// } CSFM_MarkerType;

typedef struct {
    uint32_t start;
    uint32_t end;
    uint32_t line;
    uint32_t column;
    uint32_t first_child;
    uint32_t next;
    CSFM_NodeType type;
    // CSFM_MarkerType marker_type;
} CSFM_Node;

// NOTE(mattg): For testing purposes only
static inline void printNodeText(CSFM_Node *node, CSFM_String8Slice str) {
    uint32_t length = node->end - node->start;
    char *string = (char *)&str.ptr[node->start];
    printf("%*.*s", length, length, string);
}

typedef struct {
    CSFM_Node *buffer;
    uint32_t length;
    uint32_t capacity;
} CSFM_NodeArray;

int CSFM_NodeArray_allocate(CSFM_NodeArray *array, uint32_t capacity);
void CSFM_NodeArray_deallocate(CSFM_NodeArray *array);
void CSFM_NodeArray_reuse(CSFM_NodeArray *array);
static int CSFM_NodeArray_resize(CSFM_NodeArray *array, uint32_t newCapacity);
static int CSFM_NodeArray_push(CSFM_NodeArray *array, CSFM_Node node);
CSFM_Node* CSFM_NodeArray_get(CSFM_NodeArray array, uint32_t index);

int CSFM_NodeArray_allocate(CSFM_NodeArray *array, uint32_t capacity) {
    if (array == NULL) {
        return -1;
    }
    size_t size = sizeof(CSFM_Node) * capacity;
    array->buffer = malloc(size);
    if (array->buffer == NULL) {
        array->capacity = 0;
        array->length = 0;
        return -1;
    }
    memset(array->buffer, 0, size);
    array->capacity = capacity;
    array->length = 0;
    return 0;
}

void CSFM_NodeArray_deallocate(CSFM_NodeArray *array) {
    if (array == NULL) {
        return;
    }
    if (array->buffer != NULL) {
        free(array->buffer);
        array->buffer = NULL;
    }
    array->capacity = 0;
    array->length = 0;
}

void CSFM_NodeArray_reuse(CSFM_NodeArray *array) {
    if (array == NULL) {
        return;
    }
    if (array->buffer != NULL) {
        size_t size = sizeof(CSFM_Node) * array->capacity;
        memset(array->buffer, 0, size);
    }
    array->length = 0;
}

static int CSFM_NodeArray_resize(CSFM_NodeArray *array, uint32_t newCapacity) {
    if (array == NULL || newCapacity < array->capacity) {
        return -1;
    }
    size_t newSize = sizeof(CSFM_Node) * newCapacity;
    CSFM_Node *newBuffer = malloc(newSize);
    if (newBuffer == NULL) {
        return -1;
    }
    memset(newBuffer, 0, newSize);
    if (array->buffer != NULL) {
        memcpy(newBuffer, array->buffer, sizeof(CSFM_Node) * array->length);
        free(array->buffer);
    }
    array->buffer = newBuffer;
    array->capacity = newCapacity;
    return 0;
}

static int CSFM_NodeArray_push(CSFM_NodeArray *array, CSFM_Node node) {
    if (array == NULL) {
        return -1;
    }
    if (array->length >= array->capacity) {
        if (CSFM_NodeArray_resize(array, array->capacity * 2) != 0) {
            return -1;
        }
    }

    array->buffer[array->length] = node;
    array->length++;
    return 0;
}

CSFM_Node* CSFM_NodeArray_get(CSFM_NodeArray array, uint32_t index) {
    if (index < array.length) {
        return &array.buffer[index];
    }
    return NULL;
}

typedef struct {
    CSFM_TokenArray tokens;
    CSFM_NodeArray AST;
    uint32_t token_index;
} CSFM_Parser;

void parseMarker(CSFM_Parser *parser, CSFM_String8Slice str, CSFM_Node *node) {
    CSFM_Token *token = CSFM_TokenArray_get(parser->tokens, parser->token_index);

    // start with basics, expect text.
    assert(token->type == CSFM_TOKEN_TEXT);
    node->end = token->end;
    parser->token_index++;

    // accept +
    // int hasPlus = token->type == CSFM_TOKEN_PLUS;
    // if (hasPlus) {
    //     // node->marker_type = CSFM_MARKER_TYPE_NESTED;
    //     printTokenText(token, str);
    //     parser->token_index++;
    //     token = CSFM_TokenArray_get(parser->tokens, parser->token_index);
    // }

    // expect text or *
    // int hasAsterisk = 0;
    // switch (token->type) {
    // case CSFM_TOKEN_TEXT:
    //     printTokenText(token, str);
    //     parser->token_index++;
    //     break;
    // case CSFM_TOKEN_ASTERISK:
    //     hasAsterisk = 1;
    //     printTokenText(token, str);
    //     parser->token_index++;
    //     break;
    // default:
    //     printf("Unexpected token type: %d\n", token->type);
    //     return;
    // }
    //
    // token = CSFM_TokenArray_get(parser->tokens, parser->token_index);

    // if (!hasAsterisk) {
    //     // TODO(mattg): flesh out more complex markers.
    //
    //     // accept number
    //     // accept -
    //     // if - expect text or number
    //     // accept *
    //     if (token->type == CSFM_TOKEN_ASTERISK) {
    //         hasAsterisk = 1;
    //         printTokenText(token, str);
    //         parser->token_index++;
    //         token = CSFM_TokenArray_get(parser->tokens, parser->token_index);
    //     }
    // }

    // return on whitespace/newline/end of file
    // switch (token->type) {
    // case CSFM_TOKEN_EOF:
    // case CSFM_TOKEN_WS:
    // case CSFM_TOKEN_CR:
    // case CSFM_TOKEN_LF:
    // case CSFM_TOKEN_CRLF:
    //     node->end = token->start;
    //     // if (hasAsterisk) {
    //     //     if (node->marker_type == CSFM_MARKER_TYPE_NESTED) {
    //     //         node->marker_type = CSFM_MARKER_TYPE_NESTED_CLOSE;
    //     //     } else {
    //     //         node->marker_type = CSFM_MARKER_TYPE_CLOSE;
    //     //     }
    //     // }
    //
    //     // consume whitespace
    //     parser->token_index++;
    //     if (CSFM_NodeArray_push(&parser->AST, *node) != 0) {
    //         return;
    //     }
    //     break;
    // default:
    //     printf("Unexpected token type: %d\n", token->type);
    //     return;
    // }
}

void parseText(CSFM_Parser *parser, CSFM_String8Slice str, CSFM_Node *node) {
    CSFM_Token *token = NULL;
    int endParse = 0;
    do {
        token = CSFM_TokenArray_get(parser->tokens, parser->token_index);
        switch (token->type) {
        case CSFM_TOKEN_EOF:
        case CSFM_TOKEN_CR:
        case CSFM_TOKEN_LF:
        case CSFM_TOKEN_CRLF:
        case CSFM_TOKEN_BACKSLASH:
            endParse = 1;
            break;
        default:
            node->end = token->end;
            parser->token_index++;
        }
    } while (!endParse);
}

// typedef struct {
//     uint32_t start;
//     uint32_t end;
//     uint32_t line;
//     uint32_t column;
//     uint32_t first_child;
//     uint32_t next;
//     CSFM_NodeType type;
//     CSFM_MarkerType marker_type;
// } CSFM_Node;

void parseInternal(CSFM_Parser *parser, CSFM_String8Slice str) {
    CSFM_Token *token = NULL;
    parser->token_index = 0;
    while (parser->token_index < parser->tokens.length) {
        token = CSFM_TokenArray_get(parser->tokens, parser->token_index);
        CSFM_Node node = {
            .start = token->start,
            .end = token->end,
            .line = token->line,
            .column = token->column,
        };
        switch (token->type) {
        case CSFM_TOKEN_BACKSLASH:
            node.type = CSFM_NODE_MARKER;
            // node.marker_type = CSFM_MARKER_TYPE_NORMAL;
            parser->token_index++;
            parseMarker(parser, str, &node);

            assert(CSFM_NodeArray_push(&parser->AST, node) == 0);
            printNodeText(&node, str);
            break;
        case CSFM_TOKEN_WS:
            node.type = CSFM_NODE_WHITESPACE;

            assert(CSFM_NodeArray_push(&parser->AST, node) == 0);
            printNodeText(&node, str);

            parser->token_index++;
            break;
        case CSFM_TOKEN_CR:
        case CSFM_TOKEN_LF:
        case CSFM_TOKEN_CRLF:
            node.type = CSFM_NODE_NEWLINE;

            assert(CSFM_NodeArray_push(&parser->AST, node) == 0);
            printNodeText(&node, str);

            parser->token_index++;
            break;
        case CSFM_TOKEN_EOF:
            return;
        default:
            node.type = CSFM_NODE_TEXT;
            parser->token_index++;
            parseText(parser, str, &node);

            assert(CSFM_NodeArray_push(&parser->AST, node) == 0);
            printNodeText(&node, str);
        }
    }
    return;
}

CSFM_Parser CSFM_Parse(uint8_t *buf, size_t size) {
    CSFM_Parser parser = {0};
    CSFM_String8Slice str = {
        .ptr = buf,
        .length = size,
    };

    CSFM_Tokenizer tokenizer = {0};
    if (CSFM_TokenArray_allocate(&tokenizer.array, size) != 0) {
        return parser;
    }

    tokenizeInternal(&tokenizer, str);
    parser.tokens = tokenizer.array;

    if (CSFM_NodeArray_allocate(&parser.AST, size) != 0) {
        return parser;
    }

    parseInternal(&parser, str);
    return parser;
}

#endif // CSFM
