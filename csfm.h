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

#ifndef csfm_u8
#define csfm_u8 unsigned char
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct CSFM_String8Slice CSFM_String8Slice;
typedef enum CSFM_TokenType CSFM_TokenType;
typedef struct CSFM_Token CSFM_Token;
typedef struct CSFM_TokenArray CSFM_TokenArray;

struct CSFM_String8Slice {
    size_t len;
    char *buf;
};

enum CSFM_TokenType {
    CSFM_EOF,
    CSFM_WHITESPACE,
    CSFM_CARRIAGE_RETURN,
    CSFM_NEWLINE,
    CSFM_FORWARDSLASH,
    CSFM_BACKSLASH,
    CSFM_PIPE,
    CSFM_COLON,
    CSFM_SEMICOLON,
    CSFM_TILDE,
    CSFM_ASTERISK,
    CSFM_PLUS,
    CSFM_MINUS,
    CSFM_EQUAL,
    CSFM_DOUBLE_QUOTE,
    CSFM_NUMBER,
    CSFM_TEXT,
};

struct CSFM_Token {
    size_t start;
    size_t end;
    CSFM_TokenType type;
};

struct CSFM_TokenArray {
    size_t len;
    size_t cap;
    CSFM_Token *buf;
};

/*static CSFM_TokenArray allocTokenArray(size_t size);*/
static char getValueAtIdx(CSFM_String8Slice slice, size_t idx);

void CSFM_Tokenize(char *buf, size_t size);

#ifndef GET_TOKEN_2

static CSFM_TokenType getTokenType(char byte) {
    CSFM_TokenType type = CSFM_EOF;
    switch (byte) {
    case 0:
        type = CSFM_EOF;
        break;
    /* whitespace */
    case ' ':
    case '\t':
        type = CSFM_WHITESPACE;
        break;
    case '\r':
        type = CSFM_CARRIAGE_RETURN;
        break;
    case '\n':
        type = CSFM_NEWLINE;
        break;
    /* usfm-specific characters */
    case '/':
        type = CSFM_FORWARDSLASH;
        break;
    case '\\':
        type = CSFM_BACKSLASH;
        break;
    case '|':
        type = CSFM_PIPE;
        break;
    case ':':
        type = CSFM_COLON;
        break;
    case ';':
        type = CSFM_SEMICOLON;
        break;
    case '~':
        type = CSFM_TILDE;
        break;
    case '*':
        type = CSFM_ASTERISK;
        break;
    case '+':
        type = CSFM_PLUS;
        break;
    case '-':
        type = CSFM_MINUS;
        break;
    case '=':
        type = CSFM_EQUAL;
        break;
    case '"':
        type = CSFM_DOUBLE_QUOTE;
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
        type = CSFM_NUMBER;
        break;
    default:
        type = CSFM_TEXT;
    }
    return type;
}

static void processWhitespace(CSFM_String8Slice str, size_t *idx) {
    size_t i = *idx;
    char breakWhile = 0;

    assert(i < str.len);

    while (i < str.len && !breakWhile) {
        switch (getValueAtIdx(str, i)) {
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

    assert(i < str.len);

    while (i < str.len && !breakWhile) {
        switch (getValueAtIdx(str, i)) {
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

    assert(i < str.len);

    while (i < str.len && !breakWhile) {
        switch (getValueAtIdx(str, i)) {
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

static CSFM_Token getToken(CSFM_String8Slice str, size_t idx) {
    CSFM_Token token;
    token.start = idx;
    token.type = getTokenType(getValueAtIdx(str, idx));

    idx++;
    /*assert(idx < str.len);*/

    switch (token.type) {
    case CSFM_EOF:
        break;
    case CSFM_WHITESPACE:
        processWhitespace(str, &idx);
        break;
    case CSFM_CARRIAGE_RETURN:
        if (getValueAtIdx(str, idx) == '\n') {
            idx++;
            token.type = CSFM_NEWLINE;
        }
        /* TODO(matt): consider if a carriage return is whitespace or not */
        break;
    case CSFM_NEWLINE:
    case CSFM_FORWARDSLASH:
    case CSFM_BACKSLASH:
    case CSFM_PIPE:
    case CSFM_COLON:
    case CSFM_SEMICOLON:
    case CSFM_TILDE:
    case CSFM_ASTERISK:
    case CSFM_PLUS:
    case CSFM_MINUS:
    case CSFM_EQUAL:
    case CSFM_DOUBLE_QUOTE:
        break;
    case CSFM_NUMBER:
        processNumber(str, &idx);
        break;
    case CSFM_TEXT:
        processText(str, &idx);
        break;
    default:
        exit(1);
    }
    token.end = idx;
    /*assert(idx < str.len);*/
    
    return token;
}

#else

static CSFM_Token getToken(CSFM_String8Slice str, size_t idx) {
    CSFM_Token token = {0};
    token.type = CSFM_EOF;
    token.start = idx;
    char value = 0;
    CSFM_TokenType type = CSFM_EOF;

    get_token_type: {
        value = getValueAtIdx(str, idx);
        idx++;

        switch (value) {
        case 0:
            token.type = CSFM_EOF;
            goto end;
            break;
        /* whitespace */
        case ' ':
        case '\t':
            if (type != CSFM_EOF && type != CSFM_WHITESPACE) {
                token.type = type;
                goto end;
            } else {
                type = CSFM_WHITESPACE;
                goto get_token_type;
            }
            break;
        case '\r':
            type = CSFM_CARRIAGE_RETURN;
            if (getValueAtIdx(str, idx++) == '\n') {
                idx++;
                token.type = CSFM_NEWLINE;
            } else {
                idx--;
                token.type = type;
            }
            goto end;
            break;
        case '\n':
            token.type = CSFM_NEWLINE;
            goto end;
            break;
        /* usfm-specific characters */
        case '/':
            token.type = CSFM_FORWARDSLASH;
            goto end;
            break;
        case '\\':
            token.type = CSFM_BACKSLASH;
            goto end;
            break;
        case '|':
            token.type = CSFM_PIPE;
            goto end;
            break;
        case ':':
            token.type = CSFM_COLON;
            goto end;
            break;
        case ';':
            token.type = CSFM_SEMICOLON;
            goto end;
            break;
        case '~':
            token.type = CSFM_TILDE;
            goto end;
            break;
        case '*':
            token.type = CSFM_ASTERISK;
            goto end;
            break;
        case '+':
            token.type = CSFM_PLUS;
            goto end;
            break;
        case '-':
            token.type = CSFM_MINUS;
            goto end;
            break;
        case '=':
            token.type = CSFM_EQUAL;
            goto end;
            break;
        case '"':
            token.type = CSFM_DOUBLE_QUOTE;
            goto end;
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
            if (type != CSFM_EOF && type != CSFM_NUMBER) {
                token.type = type;
                goto end;
            } else {
                type = CSFM_NUMBER;
                goto get_token_type;
            }
            break;
        default:
            if (type != CSFM_EOF && type != CSFM_TEXT) {
                token.type = type;
                goto end;
            } else {
                type = CSFM_TEXT;
                goto get_token_type;
            }
        }
    }
    end: {
        token.end = idx;
    }
    return token;
}

#endif

/*
static CSFM_TokenArray allocTokenArray(size_t size) {
    CSFM_TokenArray array = {0};
    // TODO(matt): size this to be the normal distribution of tokens/byte //
    array.cap = size;
    array.len = 0;
    array.buf = malloc(sizeof(CSFM_Token) * size);
    return array;
}
*/

static char getValueAtIdx(CSFM_String8Slice slice, size_t idx) {
    if (idx >= slice.len) {
        /*
         * NOTE(matt): This should be at the end of the slice
         * anyway, so an EOF isn't that odd to return.
         */
        return 0;
    }
    return slice.buf[idx];
}

void CSFM_Tokenize(char *buf, size_t size) {
    CSFM_String8Slice str = {
        size,
        buf
    };
    size_t i = 0;
    while (i < str.len) {
        CSFM_Token token = getToken(str, i);
        i = token.end;
        
        /*
        switch (token.type) {
        case CSFM_EOF:
            printf("EOF\n");
            break;
        case CSFM_WHITESPACE:
            printf("_");
            break;
        case CSFM_CARRIAGE_RETURN:
            printf("CR");
            break;
        case CSFM_NEWLINE:
            printf("\n");
            break;
        case CSFM_FORWARDSLASH:
            printf("/");
            break;
        case CSFM_BACKSLASH:
            printf("\\");
            break;
        case CSFM_PIPE:
            printf("|");
            break;
        case CSFM_COLON:
            printf(":");
            break;
        case CSFM_SEMICOLON:
            printf(";");
            break;
        case CSFM_TILDE:
            printf("~");
            break;
        case CSFM_ASTERISK:
            printf("*");
            break;
        case CSFM_PLUS:
            printf("+");
            break;
        case CSFM_MINUS:
            printf("-");
            break;
        case CSFM_EQUAL:
            printf("=");
            break;
        case CSFM_DOUBLE_QUOTE:
            printf("\"");
            break;
        case CSFM_NUMBER:
            printf("0");
            break;
        case CSFM_TEXT:
            printf("A");
            break;
        default:
            exit(1);
        }
        */
    }
    return;
}


#endif
