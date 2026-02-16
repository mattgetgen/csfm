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

typedef struct {
    char *ptr;
    size_t length;
} CSFM_String8Slice;

static void CSFM_String8Slice_new(CSFM_String8Slice *slice, char *ptr, size_t length);
static char CSFM_String8Slice_get(CSFM_String8Slice slice, size_t idx);

typedef enum {
    CSFM_TOKEN_EOF,
    CSFM_TOKEN_WHITESPACE,
    CSFM_TOKEN_CARRIAGE_RETURN,
    CSFM_TOKEN_NEWLINE,
    CSFM_TOKEN_FORWARDSLASH,
    CSFM_TOKEN_DOUBLE_FORWARDSLASH,
    CSFM_TOKEN_BACKSLASH,
    CSFM_TOKEN_PIPE,
    CSFM_TOKEN_PERIOD,
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
    size_t start;
    size_t end;
    size_t row;
    size_t col;
    CSFM_TokenType type;
} CSFM_Token;

typedef struct {
    CSFM_Token *buffer;
    size_t length;
    size_t capacity;
} CSFM_TokenArray;

int CSFM_TokenArray_allocate(CSFM_TokenArray *array, size_t capacity);
void CSFM_TokenArray_deallocate(CSFM_TokenArray *array);
void CSFM_TokenArray_reuse(CSFM_TokenArray *array);
static int CSFM_TokenArray_resize(CSFM_TokenArray *array, size_t newCapacity);
static int CSFM_TokenArray_push(CSFM_TokenArray *array, CSFM_Token token);
CSFM_Token* CSFM_TokenArray_get(CSFM_TokenArray array, size_t index);

typedef struct {
    CSFM_TokenArray array;
    size_t position;
    size_t row;
    size_t col;
} CSFM_Tokenizer;

static void CSFM_String8Slice_new(CSFM_String8Slice *slice, char *ptr, size_t length) {
    if (slice == NULL) {
        return;
    }
    slice->ptr = ptr;
    slice->length = length;
}

static char CSFM_String8Slice_get(CSFM_String8Slice slice, size_t index) {
    if (index >= slice.length) {
        /*
         * NOTE(mattg): This should be at the end of the slice
         * anyway, so an EOF isn't that odd to return.
         */
        return '\0';
    }
    return slice.ptr[index];
}

int CSFM_TokenArray_allocate(CSFM_TokenArray *array, size_t capacity) {
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
    /* TODO(mattg): look into what secure memset is, use it if I should */
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

static int CSFM_TokenArray_resize(CSFM_TokenArray *array, size_t newCapacity) {
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

CSFM_Token* CSFM_TokenArray_get(CSFM_TokenArray array, size_t index) {
    if (index < array.length) {
        return &array.buffer[index];
    }
    return NULL;
}

static CSFM_TokenType peekTokenType(CSFM_String8Slice str, size_t index) {
    CSFM_TokenType type = CSFM_TOKEN_EOF;
    switch (CSFM_String8Slice_get(str, index)) {
    case '\0':
        type = CSFM_TOKEN_EOF;
        break;
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

static CSFM_Token peekToken(CSFM_String8Slice str, size_t index) {
    CSFM_Token token = {0};
    token.start = index;
    token.type = peekTokenType(str, index);
    CSFM_TokenType type = CSFM_TOKEN_EOF;
    switch (token.type) {
    /* NOTE(mattg): These are 1 character tokens */
    case CSFM_TOKEN_EOF:
    case CSFM_TOKEN_NEWLINE:
    /* NOTE(mattg): CSFM_TOKEN_DOUBLE_FORWARDSLASH is never retured by peekTokenType */
    case CSFM_TOKEN_DOUBLE_FORWARDSLASH:
    case CSFM_TOKEN_BACKSLASH:
    case CSFM_TOKEN_PIPE:
    case CSFM_TOKEN_PERIOD:
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
    /* NOTE(mattg): These may be 1-2 character tokens */
    case CSFM_TOKEN_CARRIAGE_RETURN:
        if (peekTokenType(str, index+1) == CSFM_TOKEN_NEWLINE) {
            token.type = CSFM_TOKEN_NEWLINE;
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
    /* NOTE(mattg): These may be 1-N character tokens */
    case CSFM_TOKEN_WHITESPACE:
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

static int consumeToken(CSFM_Tokenizer *tokenizer, CSFM_Token token) {
    token.row = tokenizer->row;
    token.col = tokenizer->col;
    if (CSFM_TokenArray_push(&tokenizer->array, token) != 0) {
        return -1;
    }
    tokenizer->position = token.end;
    if (token.type == CSFM_TOKEN_NEWLINE) {
        tokenizer->row = 1;
        tokenizer->col++;
    } else {
        tokenizer->row += (token.end - token.start); /* push the row by the token length. */
    }
    return 0;
}

static void tokenizeInternal(CSFM_Tokenizer *tokenizer, CSFM_String8Slice str) {
    tokenizer->row = 1;
    tokenizer->col = 1;
    CSFM_Token token = {0};
    do {
        token = peekToken(str, tokenizer->position);
        if (consumeToken(tokenizer, token) == -1) {
            break;
        }
    } while (token.type != CSFM_TOKEN_EOF);
    return;
}

CSFM_Tokenizer CSFM_Tokenize(char *buf, size_t size) {
    CSFM_String8Slice str = {0};
    CSFM_String8Slice_new(&str, buf, size);

    CSFM_Tokenizer tokenizer = {0};
    if (CSFM_TokenArray_allocate(&tokenizer.array, size) != 0) {
        return tokenizer;
    }

    tokenizeInternal(&tokenizer, str);

    return tokenizer;
}

typedef enum {
    CSFM_NODE_MARKER,
    CSFM_NODE_TEXT
} CSFM_NodeType;

typedef struct CSFM_NodeArray CSFM_NodeArray;

/*
typedef struct {
    CSFM_NodeArray children;
    size_t start;
    size_t end;
    size_t row;
    size_t col;
    CSFM_NodeType type;
} CSFM_Node;

struct CSFM_NodeArray {
    CSFM_Node *buffer;
    size_t length;
    size_t capacity;
};

int CSFM_NodeArray_allocate(CSFM_NodeArray *array, size_t capacity);
void CSFM_NodeArray_deallocate(CSFM_NodeArray *array);
void CSFM_NodeArray_reuse(CSFM_NodeArray *array);
static int CSFM_NodeArray_resize(CSFM_NodeArray *array, size_t newCapacity);
static int CSFM_NodeArray_push(CSFM_NodeArray *array, CSFM_Node node);
CSFM_Node* CSFM_NodeArray_get(CSFM_NodeArray array, size_t index);

int CSFM_NodeArray_allocate(CSFM_NodeArray *array, size_t capacity) {
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

static int CSFM_NodeArray_resize(CSFM_NodeArray *array, size_t newCapacity) {
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

CSFM_Node* CSFM_NodeArray_get(CSFM_NodeArray array, size_t index) {
    if (index < array.length) {
        return &array.buffer[index];
    }
    return NULL;
}

void CSFM_Parse(char *buf, size_t size) {
}
*/

/*
static CSFM_Node* allocateNode() {
    CSFM_Node *node = malloc(sizeof(CSFM_Node));
    if (node == NULL) {
        *//* TODO(matt): handle failed allocation *//*
    }
    *//* TODO(matt): look into what secure memset is, use it if I should */
    /*memset(node, 0, sizeof(CSFM_Node));

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
    CSFM_String8Slice str = {0};
    CSFM_String8Slice_new(&str, buf, size);
    CSFM_TokenArray array = {0};
    if (CSFM_TokenArray_allocate(&array, size) != 0) {
        return;
    }

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
    free(root);
    CSFM_TokenArray_deallocate(&array);

    return;
}*/

#endif /* CSFM */
