#ifndef CSFM
#define CSFM

#include <assert.h>
#include <stdio.h>

typedef struct {
    size_t length;
    const char *buffer;
} String8Slice;

typedef enum {
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
    CSFM_CHARACTER,
} CSFMTokenType;

typedef struct {
    size_t start;
    size_t end;
    CSFMTokenType type;
} CSFMToken;

static CSFMTokenType getTokenType(const char byte); 
static CSFMToken getToken(String8Slice str, size_t *idx);
void CSFMTokenize(const char *buf, size_t size);


static CSFMTokenType getTokenType(const char byte) {
    CSFMTokenType type = EOF;
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
    /* numbers and characters */
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
        type = CSFM_CHARACTER;
    }
    return type;
}

static void processWhitespace(String8Slice str, size_t *idx) {
    size_t i = *idx;
    char breakWhile = 0;

    assert(i < str.length);

    while (i < str.length && !breakWhile) {
        switch (str.buffer[i]) {
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

static void processNumber(String8Slice str, size_t *idx) {
    size_t i = *idx;
    char breakWhile = 0;

    assert(i < str.length);

    while (i < str.length && !breakWhile) {
        switch (str.buffer[i]) {
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

static void processCharacter(String8Slice str, size_t *idx) {
    size_t i = *idx;
    char breakWhile = 0;

    assert(i < str.length);

    while (i < str.length && !breakWhile) {
        switch (str.buffer[i]) {
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

static CSFMToken getToken(String8Slice str, size_t *idx) {
    size_t i = *idx;
    CSFMToken token;
    token.start = i;
    token.type = getTokenType(str.buffer[i]);

    i++;
    assert(i < str.length);

    switch (token.type) {
    case CSFM_EOF:
        break;
    case CSFM_WHITESPACE:
        processWhitespace(str, &i);
        break;
    case CSFM_CARRIAGE_RETURN:
        if (str.buffer[i] == '\n') {
            i++;
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
        processNumber(str, &i);
        break;
    case CSFM_CHARACTER:
        /* call character func */
        processCharacter(str, &i);
        break;
    }
    token.end = i;
    assert(i <= str.length);

    *idx = i;
    
    return token;
}


void CSFMTokenize(const char *buf, size_t size) {
    String8Slice str = {
        size,
        buf
    };
    size_t i = 0;
    while (i < str.length) {
        getToken(str, &i);
    }
    return;
}


#endif
