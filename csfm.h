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
     MIT License

     Copyright (c) 2026 Matthew Getgen

     Permission is hereby granted, free of charge, to any person obtaining a copy
     of this software and associated documentation files (the “Software”), to deal
     in the Software without restriction, including without limitation the rights
     to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
     copies of the Software, and to permit persons to whom the Software is
     furnished to do so, subject to the following conditions:

     The above copyright notice and this permission notice shall be included in all
     copies or substantial portions of the Software.

     THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
     AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
     OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
     SOFTWARE.
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
    CSFM_TOKEN_NULL,
    CSFM_TOKEN_WS,
    CSFM_TOKEN_CR,
    CSFM_TOKEN_LF,
    CSFM_TOKEN_FORWARDSLASH,
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
    CSFM_TokenType type;
} CSFM_Token;

// NOTE(mattg): For testing purposes only
void debugPrintToken(CSFM_Token token, CSFM_String8Slice str) {
    uint32_t length = token.end - token.start;
    char *string = (char *)&str.ptr[token.start];
    switch (token.type) {
    case CSFM_TOKEN_NULL:
        printf("[NULL]\n");
        break;
    case CSFM_TOKEN_CR:
        printf("[CR]\n");
        break;
    case CSFM_TOKEN_LF:
        printf("[LF]\n");
        break;
    default:
        printf("[%*.*s]", length, length, string);
    }
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
CSFM_Token CSFM_TokenArray_get(CSFM_TokenArray array, uint32_t index);

typedef struct {
    CSFM_String8Slice input;
    CSFM_TokenArray tokens;
} CSFM_TokenResult;

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

CSFM_Token CSFM_TokenArray_get(CSFM_TokenArray array, uint32_t index) {
    if (index < array.length) {
        return array.buffer[index];
    }
    CSFM_Token stub = {0};
    return stub;
}

static inline CSFM_TokenType peekTokenType(CSFM_String8Slice str, uint32_t index) {
    CSFM_TokenType type = CSFM_TOKEN_NULL;
    switch (CSFM_String8Slice_get(str, index)) {
    case '\0':
        type = CSFM_TOKEN_NULL;
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
    CSFM_TokenType type = CSFM_TOKEN_NULL;
    switch (token.type) {
    // NOTE(mattg): These are 1 character tokens
    case CSFM_TOKEN_NULL:
    case CSFM_TOKEN_CR:
    case CSFM_TOKEN_LF:
    case CSFM_TOKEN_FORWARDSLASH:
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

static inline CSFM_Token consumeToken(CSFM_String8Slice str, uint32_t *index) {
    CSFM_Token token = peekToken(str, *index);
    *index = token.end;
    return token;
}

CSFM_TokenResult CSFM_TokenizeAll(uint8_t *buf, size_t size) {
    CSFM_TokenResult result = {
        .input = {
            .ptr = buf,
            .length = size,
        },
    };
    if (CSFM_TokenArray_allocate(&result.tokens, size) != 0) {
        return result;
    }

    uint32_t tokenIndex = 0;
    CSFM_Token token = {0};
    do {
        token = consumeToken(result.input, &tokenIndex);
        if (token.type == CSFM_TOKEN_NULL) {
            break;
        }
        if (CSFM_TokenArray_push(&result.tokens, token) != 0) {
            break;
        }
    } while (token.type != CSFM_TOKEN_NULL);

    return result;
}

typedef enum {
    CSFM_NODE_NULL,
    CSFM_NODE_MARKER,
    CSFM_NODE_TEXT,
    CSFM_NODE_WHITESPACE,
    CSFM_NODE_NEWLINE,
} CSFM_NodeType;

typedef enum {
    CSFM_MARKER_TYPE_NORMAL,
    CSFM_MARKER_TYPE_CLOSE,
    CSFM_MARKER_TYPE_NESTED,
    CSFM_MARKER_TYPE_NESTED_CLOSE,
} CSFM_MarkerType;

typedef struct {
    uint32_t start;
    uint32_t end;
    uint32_t line;
    uint32_t column;
    uint32_t first_child;
    uint32_t next;
    CSFM_NodeType type;
    CSFM_MarkerType marker_type;
    uint32_t marker_text_start;
    uint32_t marker_text_end;
} CSFM_Node;

// NOTE(mattg): For testing purposes only
void debugPrintNode(CSFM_Node node, CSFM_String8Slice str) {
    uint32_t length = node.end - node.start;
    char *string = (char *)&str.ptr[node.start];
    switch (node.type) {
    case CSFM_NODE_NULL:
        printf("[NULL]\n");
        break;
    case CSFM_NODE_NEWLINE:
        printf("[NL]\n");
        break;
    default:
        printf("[%*.*s]", length, length, string);
    }
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
static void CSFM_NodeArray_pop(CSFM_NodeArray *array);
CSFM_Node CSFM_NodeArray_get(CSFM_NodeArray array, uint32_t index);

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
    if (array == NULL || array->buffer == NULL) {
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

static void CSFM_NodeArray_pop(CSFM_NodeArray *array) {
    if (array == NULL || array->buffer == NULL) {
        return;
    }
    if (array->length > 0) {
        array->length--;
    }
}

CSFM_Node CSFM_NodeArray_get(CSFM_NodeArray array, uint32_t index) {
    if (index < array.length) {
        return array.buffer[index];
    }
    CSFM_Node stub = {0};
    return stub;
}

typedef struct {
    CSFM_String8Slice input;
    CSFM_NodeArray tree;
} CSFM_ParseResult;

void parseMarker(CSFM_String8Slice input, uint32_t *tokenIndex, CSFM_Token *token, CSFM_Node *node) {
    CSFM_Token currToken = peekToken(input, *tokenIndex);

    // accept '+' OR '*' OR text.
    switch (currToken.type) {
    case CSFM_TOKEN_ASTERISK:
        // end of marker
        node->marker_type = CSFM_MARKER_TYPE_CLOSE;
        node->end = currToken.end;
        *tokenIndex = currToken.end;
        *token = currToken;
        return;
    case CSFM_TOKEN_PLUS:
        node->marker_type = CSFM_MARKER_TYPE_NESTED;
        node->end = currToken.end;
        *tokenIndex = currToken.end;
        currToken = peekToken(input, *tokenIndex);
        
        // just get the text after the plus
        node->end = currToken.end;
        if (currToken.type != CSFM_TOKEN_TEXT) {
            // TODO(mattg): error handling. Something something unexpected token.
            return;
        }
        node->marker_text_start = currToken.start;
        node->marker_text_end = currToken.end;
        *tokenIndex = currToken.end;
        *token = currToken;
        currToken = peekToken(input, *tokenIndex);
        break;
    case CSFM_TOKEN_TEXT:
        node->end = currToken.end;
        node->marker_text_start = currToken.start;
        node->marker_text_end = currToken.end;
        *tokenIndex = currToken.end;
        *token = currToken;
        currToken = peekToken(input, *tokenIndex);
        break;
    default:
        node->end = currToken.start;
        // TODO(mattg): error handling. Something something unexpected token.
        return;
    }
    
    // accept number
    // NOTE(mattg): this would change the marker text length, but for now we don't track that.

    // accept -
    // if - expect text or number
    // accept *
    if (currToken.type == CSFM_TOKEN_ASTERISK) {
        node->marker_type++;
        node->end = currToken.end;
        *tokenIndex = currToken.end;
        *token = currToken;
        return;
    }
    // return on whitespace/newline/end of file

    // start with basics, expect text.
    // assert(token.type == CSFM_TOKEN_TEXT);
    // node->end = token.end;
    // parser->token_index++;

}

void parseText(CSFM_String8Slice input, uint32_t *tokenIndex, CSFM_Token *token, CSFM_Node *node) {
    CSFM_Token currToken = {0};
    int endParse = 0;
    while (!endParse) {
        currToken = peekToken(input, *tokenIndex);
        switch (token->type) {
        case CSFM_TOKEN_NULL:
        case CSFM_TOKEN_CR:
        case CSFM_TOKEN_LF:
        case CSFM_TOKEN_BACKSLASH:
            endParse = 1;
            break;
        default:
            node->end = token->end;
            *tokenIndex = token->end;
            *token = currToken;
        }
    }
}

CSFM_ParseResult CSFM_Parse(uint8_t *buf, size_t size) {
    CSFM_ParseResult result = {
        .input = {
            .ptr = buf,
            .length = size,
        },
    };
    if (CSFM_NodeArray_allocate(&result.tree, size) != 0) {
        return result;
    }

    uint32_t tokenIndex = 0;
    CSFM_Token token = {0};
    CSFM_Token prevToken = {0};
    CSFM_Node prevNode = {0};
    do {
        token = consumeToken(result.input, &tokenIndex);

        CSFM_Node node = {
            .start = token.start,
            .end = token.end,
        };

        switch (token.type) {
        case CSFM_TOKEN_NULL:
            node.type = CSFM_NODE_NULL;
            break;
        case CSFM_TOKEN_BACKSLASH:
            node.type = CSFM_NODE_MARKER;
            assert(node.marker_type == CSFM_MARKER_TYPE_NORMAL);
            parseMarker(result.input, &tokenIndex, &token, &node);
            break;
        case CSFM_TOKEN_WS:
            node.type = CSFM_NODE_WHITESPACE;
            break;
        case CSFM_TOKEN_CR:
            node.type = CSFM_NODE_NEWLINE;
            break;
        case CSFM_TOKEN_LF:
            node.type = CSFM_NODE_NEWLINE;
            if (prevToken.type == CSFM_TOKEN_CR) {
                node.start = prevNode.start;
                // NOTE(mattg): Remove the last element (it was a CR), replace it with 1 newline node.
                CSFM_NodeArray_pop(&result.tree);
            }
            break;
        default:
            node.type = CSFM_NODE_TEXT;
            parseText(result.input, &tokenIndex, &token, &node);
            break;
        }

        if (CSFM_NodeArray_push(&result.tree, node) != 0) {
            return result;
        }
        prevToken = token;
        prevNode = node;
    } while (token.type != CSFM_TOKEN_NULL);


    return result;
}

#endif // CSFM
