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

#ifndef csfm_u8
#define csfm_u8 unsigned char
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct CSFM_String8Slice CSFM_String8Slice;
typedef enum CSFM_TokenType CSFM_TokenType;
typedef struct CSFM_Token CSFM_Token;
typedef struct CSFM_TokenArray CSFM_TokenArray;
typedef enum CSFM_MarkerType CSFM_MarkerType;
typedef enum CSFM_NodeType CSFM_NodeType;
typedef struct CSFM_Node CSFM_Node;

struct CSFM_String8Slice {
    size_t length;
    char *buffer;
};

enum CSFM_TokenType {
    CSFM_TOKEN_EOF,
    CSFM_TOKEN_WHITESPACE,
    CSFM_TOKEN_CARRIAGE_RETURN,
    CSFM_TOKEN_NEWLINE,
    CSFM_TOKEN_FORWARDSLASH,
    CSFM_TOKEN_BACKSLASH,
    CSFM_TOKEN_PIPE,
    CSFM_TOKEN_COLON,
    CSFM_TOKEN_SEMICOLON,
    CSFM_TOKEN_TILDE,
    CSFM_TOKEN_ASTERISK,
    CSFM_TOKEN_PLUS,
    CSFM_TOKEN_MINUS,
    CSFM_TOKEN_EQUAL,
    CSFM_TOKEN_DOUBLE_QUOTE,
    CSFM_TOKEN_NUMBER,
    CSFM_TOKEN_TEXT,
};

struct CSFM_Token {
    size_t start;
    size_t end;
    CSFM_TokenType type;
};

struct CSFM_TokenArray {
    size_t length;
    size_t capacity;
    CSFM_Token *buffer;
};

enum CSFM_NodeType {
    CSFM_NODE_UNKNOWN,
    CSFM_NODE_ROOT,
    CSFM_NODE_INVALID,
    CSFM_NODE_MARKER,
    CSFM_NODE_TEXT,
};

struct CSFM_Node {
    CSFM_Node *next;
    CSFM_Node *child;
    CSFM_NodeType type;
    size_t start;
    size_t end;
    size_t row;
    size_t column;
};

static char CSFM_String8Slice_get(CSFM_String8Slice slice, size_t idx);
static CSFM_TokenArray CSFM_TokenArray_allocate(size_t size);
static int pushToken(CSFM_TokenArray *array, CSFM_Token token);
static void tokenizeInternal(CSFM_String8Slice str, CSFM_TokenArray *array);
CSFM_TokenArray CSFM_Tokenize(char *buf, size_t size);
void CSFM_Parse(char *buf, size_t size);

static CSFM_TokenType getTokenType(char byte) {
    CSFM_TokenType type = CSFM_TOKEN_EOF;
    switch (byte) {
    case 0:
        type = CSFM_TOKEN_EOF;
        break;
    /* whitespace */
    case ' ':
    case '\t':
        type = CSFM_TOKEN_WHITESPACE;
        break;
    case '\r':
        type = CSFM_TOKEN_CARRIAGE_RETURN;
        break;
    case '\n':
        type = CSFM_TOKEN_NEWLINE;
        break;
    /* usfm-specific characters */
    case '/':
        type = CSFM_TOKEN_FORWARDSLASH;
        break;
    case '\\':
        type = CSFM_TOKEN_BACKSLASH;
        break;
    case '|':
        type = CSFM_TOKEN_PIPE;
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
    /* numbers and text */
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

static void processWhitespace(CSFM_String8Slice str, size_t *idx) {
    size_t i = *idx;
    char breakWhile = 0;

    assert(i < str.length);

    while (i < str.length && !breakWhile) {
        switch (CSFM_String8Slice_get(str, i)) {
        case ' ':
        case '\t':
            i++;
            break;
        default:
            breakWhile = 1;
        }
    }
    *idx = i;
    return;
}

static void processNumber(CSFM_String8Slice str, size_t *idx) {
    size_t i = *idx;
    char breakWhile = 0;

    assert(i < str.length);

    while (i < str.length && !breakWhile) {
        switch (CSFM_String8Slice_get(str, i)) {
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
            i++;
            break;
        default:
            breakWhile = 1;
        }
    }
    *idx = i;
    return;
}

static void processText(CSFM_String8Slice str, size_t *idx) {
    size_t i = *idx;
    char breakWhile = 0;

    assert(i < str.length);

    while (i < str.length && !breakWhile) {
        switch (CSFM_String8Slice_get(str, i)) {
        case 0:
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '/':
        case '\\':
        case '|':
        case ':':
        case ';':
        case '~':
        case '*':
        case '+':
        case '-':
        case '=':
        case '"':
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
            breakWhile = 1;
            break;
        default:
            i++;
        }
    }
    *idx = i;
    return;
}

static CSFM_Token processToken(CSFM_String8Slice str, size_t idx) {
    CSFM_Token token;
    token.start = idx;
    token.type = getTokenType(CSFM_String8Slice_get(str, idx));

    idx++;
    assert(idx <= str.length);

    switch (token.type) {
    case CSFM_TOKEN_EOF:
        break;
    case CSFM_TOKEN_WHITESPACE:
        processWhitespace(str, &idx);
        break;
    case CSFM_TOKEN_CARRIAGE_RETURN:
    case CSFM_TOKEN_NEWLINE:
    case CSFM_TOKEN_FORWARDSLASH:
    case CSFM_TOKEN_BACKSLASH:
    case CSFM_TOKEN_PIPE:
    case CSFM_TOKEN_COLON:
    case CSFM_TOKEN_SEMICOLON:
    case CSFM_TOKEN_TILDE:
    case CSFM_TOKEN_ASTERISK:
    case CSFM_TOKEN_PLUS:
    case CSFM_TOKEN_MINUS:
    case CSFM_TOKEN_EQUAL:
    case CSFM_TOKEN_DOUBLE_QUOTE:
        break;
    case CSFM_TOKEN_NUMBER:
        processNumber(str, &idx);
        break;
    case CSFM_TOKEN_TEXT:
        processText(str, &idx);
        break;
    default:
        exit(1);
    }
    token.end = idx;
    assert(idx <= str.length);
    
    return token;
}

static char CSFM_String8Slice_get(CSFM_String8Slice slice, size_t idx) {
    if (idx >= slice.length) {
        /*
         * NOTE(matt): This should be at the end of the slice
         * anyway, so an EOF isn't that odd to return.
         */
        return 0;
    }
    return slice.buffer[idx];
}

static CSFM_TokenArray CSFM_TokenArray_allocate(size_t size) {
    CSFM_TokenArray array = {0};
    /* TODO(matt): size this to be the normal distribution of tokens/byte */
    array.capacity = size;
    array.length = 0;
    array.buffer = malloc(sizeof(CSFM_Token) * array.capacity);
    if (array.buffer == NULL) {
        array.capacity = 0;
        return array;
    }
    /* TODO(matt): look into what secure memset is, use it if I should */
    memset(array.buffer, 0, array.capacity);
    return array;
}

static CSFM_Token* CSFM_TokenArray_get(CSFM_TokenArray array, size_t index) {
    if (index < array.length) {
        return &array.buffer[index];
    }
    return NULL;
}

static int pushToken(CSFM_TokenArray *array, CSFM_Token token) {
    if (array->length >= array->capacity) {
        return 0;
    }

    array->buffer[array->length] = token;
    array->length++;
    return 1;
}

static void tokenizeInternal(CSFM_String8Slice str, CSFM_TokenArray *array) {
    size_t i = 0;
    while (i < str.length) {
        CSFM_Token token = processToken(str, i);
        i = token.end;
        pushToken(array, token);
    }
    return;
}

CSFM_TokenArray CSFM_Tokenize(char *buf, size_t size) {
    CSFM_String8Slice str = {
        size,
        buf
    };
    CSFM_TokenArray array = CSFM_TokenArray_allocate(size);

    tokenizeInternal(str, &array);

    return array;
}

static CSFM_Node* allocateNode() {
    CSFM_Node *node = malloc(sizeof(CSFM_Node));
    if (node == NULL) {
        /* TODO(matt): handle failed allocation */
    }
    /* TODO(matt): look into what secure memset is, use it if I should */
    memset(node, 0, sizeof(CSFM_Node));

    return node;
}

static void parseAny(CSFM_TokenArray array, size_t *tokenIndex, CSFM_Node *parent);
static void parseMarker(CSFM_TokenArray array, size_t *tokenIndex, CSFM_Node *parent);
static void parseText(CSFM_TokenArray array, size_t *tokenIndex, CSFM_Node *parent);

static void parseAny(CSFM_TokenArray array, size_t *tokenIndex, CSFM_Node *parent) {
    CSFM_Token *token = CSFM_TokenArray_get(array, *tokenIndex);
    if (token->type != CSFM_TOKEN_BACKSLASH) {
        parseText(array, tokenIndex, parent);
        return;
    }
    parseMarker(array, tokenIndex, parent);
    if (parent->next != NULL) {
        parseText(array, tokenIndex, parent->next);
    }
    return;
}

static void parseMarker(CSFM_TokenArray array, size_t *tokenIndex, CSFM_Node *parent) {
    CSFM_Token *token = NULL;
    CSFM_Node *marker = NULL;

    token = CSFM_TokenArray_get(array, *tokenIndex);
    if (token == NULL) {
        return;
    }
    do {
        switch (token->type) {
        case CSFM_TOKEN_EOF:
        case CSFM_TOKEN_WHITESPACE:
        case CSFM_TOKEN_CARRIAGE_RETURN:
        case CSFM_TOKEN_NEWLINE:
            if (marker != NULL) {
                marker->end = token->start;
                return;
            }
            break;
        case CSFM_TOKEN_BACKSLASH:
            if (marker == NULL) {
                marker = allocateNode();
                if (marker != NULL) {
                    marker->type = CSFM_NODE_MARKER;
                    marker->start = token->start;
                    parent->next = marker;
                }
            }
            break;
        case CSFM_TOKEN_FORWARDSLASH:
        case CSFM_TOKEN_PIPE:
        case CSFM_TOKEN_COLON:
        case CSFM_TOKEN_SEMICOLON:
        case CSFM_TOKEN_TILDE:
        case CSFM_TOKEN_ASTERISK:
        case CSFM_TOKEN_PLUS:
        case CSFM_TOKEN_MINUS:
        case CSFM_TOKEN_EQUAL:
        case CSFM_TOKEN_DOUBLE_QUOTE:
        case CSFM_TOKEN_NUMBER:
            if (marker != NULL) {
                marker->end = token->start;
                return;
            }
            break;
        case CSFM_TOKEN_TEXT:
            break;
        }

        (*tokenIndex)++;
        token = CSFM_TokenArray_get(array, *tokenIndex);
    } while (token != NULL);
}

static void parseText(CSFM_TokenArray array, size_t *tokenIndex, CSFM_Node *parent) {
    CSFM_Token *token = NULL;
    CSFM_Node *text = NULL;
    token = CSFM_TokenArray_get(array, *tokenIndex);
    if (token == NULL) {
        return;
    }
    do {
        switch (token->type) {
        case CSFM_TOKEN_EOF:
            if (text != NULL) {
                text->end = token->start;
            }
            return;
            break;
        case CSFM_TOKEN_WHITESPACE:
            break;
        case CSFM_TOKEN_CARRIAGE_RETURN:
        case CSFM_TOKEN_NEWLINE:
            if (text != NULL) {
                text->end = token->start;
                return;
            }
            break;
        case CSFM_TOKEN_BACKSLASH:
            if (text != NULL) {
                text->end = token->start;
            }
            return;
            break;
        case CSFM_TOKEN_FORWARDSLASH:
        case CSFM_TOKEN_PIPE:
        case CSFM_TOKEN_COLON:
        case CSFM_TOKEN_SEMICOLON:
        case CSFM_TOKEN_TILDE:
        case CSFM_TOKEN_ASTERISK:
        case CSFM_TOKEN_PLUS:
        case CSFM_TOKEN_MINUS:
        case CSFM_TOKEN_EQUAL:
        case CSFM_TOKEN_DOUBLE_QUOTE:
        case CSFM_TOKEN_NUMBER:
        case CSFM_TOKEN_TEXT:
            if (text == NULL) {
                text = allocateNode();
                if (text != NULL) {
                    text->type = CSFM_NODE_TEXT;
                    text->start = token->start;
                    parent->child = text;
                }
            }
            break;
        }

        (*tokenIndex)++;
        token = CSFM_TokenArray_get(array, *tokenIndex);
    } while (token != NULL);
}

static int printNode(CSFM_Node *node, int indent);

static int printNode(CSFM_Node *node, int indent) {
    int returnVal = 0;
    printf("%d ", indent);
    switch (node->type) {
    case CSFM_NODE_UNKNOWN:
        printf("UNKNOWN\n");
        break;
    case CSFM_NODE_ROOT:
        returnVal = 1;
        printf("ROOT\n");
        break;
    case CSFM_NODE_INVALID:
        printf("INVALID\n");
        break;
    case CSFM_NODE_MARKER:
        printf("MARKER\n");
        break;
    case CSFM_NODE_TEXT:
        printf("TEXT\n");
        break;
    }
    if (node->child != NULL) {
        returnVal = printNode(node->child, indent+1);
        free(node->child);
        node->child = NULL;
    }
    if (node->next != NULL) {
        returnVal = printNode(node->next, indent);
        free(node->next);
        node->next = NULL;
    }
    return returnVal;
}

void CSFM_Parse(char *buf, size_t size) {
    CSFM_String8Slice str = {
        size,
        buf,
    };
    CSFM_TokenArray array = CSFM_TokenArray_allocate(size);

    tokenizeInternal(str, &array);

    CSFM_Node *root = allocateNode();
    root->type = CSFM_NODE_ROOT;
    
    size_t tokenIndex = 0;

    while (tokenIndex < array.length) {
        parseAny(array, &tokenIndex, root);
    }

    while (1) {
        if (printNode(root, 1) == 1) {
            break;
        }
    }
    free(array.buffer);

    return;
}

#endif
