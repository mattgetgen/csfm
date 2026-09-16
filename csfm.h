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

#ifndef CSFM_HEADER
#define CSFM_HEADER

#define CSFM_VERSION_MAJOR 0
#define CSFM_VERSION_MINOR 0
#define CSFM_VERSION_PATCH 0
#define CSFM_VERSION "0.0.0-dev"

#if defined(__has_feature)
# if __has_feature(address_sanitizer)
#  define CSFM_ASAN_ENABLED 1
# endif
#endif

#if defined(__SANITIZE_ADDRESS__)
# define CSFM_ASAN_ENABLED 1
#endif

#ifndef CSFM_ASAN_ENABLED
# define CSFM_ASAN_ENABLED 0
#endif

#if CSFM_ASAN_ENABLED
# include <sanitizer/asan_interface.h>

# define CSFM_ARENA_ASAN_BUFFER_SIZE 16
# define CSFM_ARENA_ASAN_POISON(ptr, size) \
    ASAN_POISON_MEMORY_REGION((ptr), (size))
# define CSFM_ARENA_ASAN_UNPOISON(ptr, size) \
    ASAN_UNPOISON_MEMORY_REGION((ptr), (size))
#else
# define CSFM_ARENA_ASAN_BUFFER_SIZE 0
# define CSFM_ARENA_ASAN_POISON(ptr, size)
# define CSFM_ARENA_ASAN_UNPOISON(ptr, size)
#endif
#define CSFM_ARENA_BLOCK_SIZE_DEFAULT 4096

#ifndef CSFM_CODEGEN
# define CSFM_CODEGEN 0
#endif

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct USFM_ArenaBlock USFM_ArenaBlock;

struct USFM_ArenaBlock {
    USFM_ArenaBlock *previous;
    size_t capacity;
    size_t offset;
    // NOTE(mattg): this must be the last element in order to point at the memory after the struct.
    uint8_t memory[];
};

typedef struct {
    USFM_ArenaBlock *first;
    USFM_ArenaBlock *current;
} USFM_Arena;

typedef enum {
    USFM_TOKEN_UNKNOWN,
    USFM_TOKEN_WHITESPACE,
    USFM_TOKEN_NEWLINE,
    USFM_TOKEN_MARKER_START,
    USFM_TOKEN_MARKER_TEXT,
    USFM_TOKEN_MARKER_NUMBER,
    USFM_TOKEN_MARKER_NESTED,
    USFM_TOKEN_MARKER_CLOSE,
    USFM_TOKEN_MARKER_SUFFIX,
    USFM_TOKEN_TEXT,
    USFM_TOKEN_NUMBER,
    USFM_TOKEN_PLUS,
    USFM_TOKEN_MINUS,
    USFM_TOKEN_ASTERISK,
} USFM_TokenType;

typedef struct {
    uint32_t offset;
    uint32_t length;
    USFM_TokenType type;
} USFM_Token;

typedef struct {
    USFM_Token *buffer;
    uint32_t length;
    uint32_t capacity;
} USFM_TokenArray;

typedef enum {
// CSFM_CODEGEN_DATA marker_type start
    USFM_MARKER_UNKNOWN,
    USFM_MARKER_ID,
    USFM_MARKER_USFM,
    USFM_MARKER_IDE,
    USFM_MARKER_STS,
    USFM_MARKER_REM,
    USFM_MARKER_H,
    USFM_MARKER_TOC,
    USFM_MARKER_TOCA,
    USFM_MARKER_IMT,
    USFM_MARKER_IS,
    USFM_MARKER_IP,
    USFM_MARKER_IPI,
    USFM_MARKER_IM,
    USFM_MARKER_IMI,
    USFM_MARKER_IPQ,
    USFM_MARKER_IMQ,
    USFM_MARKER_IPR,
    USFM_MARKER_IQ,
    USFM_MARKER_IB,
    USFM_MARKER_ILI,
    USFM_MARKER_IOT,
    USFM_MARKER_IO,
    USFM_MARKER_IOR,
    USFM_MARKER_IQT,
    USFM_MARKER_IEX,
    USFM_MARKER_IMTE,
    USFM_MARKER_IE,
    USFM_MARKER_MT,
    USFM_MARKER_MTE,
    USFM_MARKER_MS,
    USFM_MARKER_MR,
    USFM_MARKER_S,
    USFM_MARKER_SR,
    USFM_MARKER_R,
    USFM_MARKER_RQ,
    USFM_MARKER_D,
    USFM_MARKER_SP,
    USFM_MARKER_SD,
    USFM_MARKER_C,
    USFM_MARKER_CA,
    USFM_MARKER_CL,
    USFM_MARKER_CP,
    USFM_MARKER_CD,
    USFM_MARKER_V,
    USFM_MARKER_VA,
    USFM_MARKER_VP,
    USFM_MARKER_P,
    USFM_MARKER_M,
    USFM_MARKER_PO,
    USFM_MARKER_PR,
    USFM_MARKER_CLS,
    USFM_MARKER_PMO,
    USFM_MARKER_PM,
    USFM_MARKER_PMC,
    USFM_MARKER_PMR,
    USFM_MARKER_PI,
    USFM_MARKER_MI,
    USFM_MARKER_NB,
    USFM_MARKER_PC,
    USFM_MARKER_PH,
    USFM_MARKER_B,
    USFM_MARKER_Q,
    USFM_MARKER_QR,
    USFM_MARKER_QC,
    USFM_MARKER_QS,
    USFM_MARKER_QA,
    USFM_MARKER_QAC,
    USFM_MARKER_QM,
    USFM_MARKER_QD,
    USFM_MARKER_LH,
    USFM_MARKER_LI,
    USFM_MARKER_LF,
    USFM_MARKER_LIM,
    USFM_MARKER_LITL,
    USFM_MARKER_LIK,
    USFM_MARKER_LIV,
    USFM_MARKER_TR,
    USFM_MARKER_TH,
    USFM_MARKER_THR,
    USFM_MARKER_TC,
    USFM_MARKER_TCR,
    USFM_MARKER_F,
    USFM_MARKER_FE,
    USFM_MARKER_FR,
    USFM_MARKER_FQ,
    USFM_MARKER_FQA,
    USFM_MARKER_FK,
    USFM_MARKER_FL,
    USFM_MARKER_FW,
    USFM_MARKER_FP,
    USFM_MARKER_FV,
    USFM_MARKER_FT,
    USFM_MARKER_FDC,
    USFM_MARKER_FM,
    USFM_MARKER_X,
    USFM_MARKER_XO,
    USFM_MARKER_XK,
    USFM_MARKER_XQ,
    USFM_MARKER_XT,
    USFM_MARKER_XTA,
    USFM_MARKER_XOP,
    USFM_MARKER_XOT,
    USFM_MARKER_XNT,
    USFM_MARKER_XDC,
    USFM_MARKER_ADD,
    USFM_MARKER_BK,
    USFM_MARKER_DC,
    USFM_MARKER_K,
    USFM_MARKER_LIT,
    USFM_MARKER_ND,
    USFM_MARKER_ORD,
    USFM_MARKER_PN,
    USFM_MARKER_PNG,
    USFM_MARKER_ADDPN,
    USFM_MARKER_QT,
    USFM_MARKER_SIG,
    USFM_MARKER_SLS,
    USFM_MARKER_TL,
    USFM_MARKER_WJ,
    USFM_MARKER_EM,
    USFM_MARKER_BD,
    USFM_MARKER_IT,
    USFM_MARKER_BDIT,
    USFM_MARKER_NO,
    USFM_MARKER_SC,
    USFM_MARKER_SUP,
    USFM_MARKER_PB,
    USFM_MARKER_FIG,
    USFM_MARKER_NDX,
    USFM_MARKER_RB,
    USFM_MARKER_PRO,
    USFM_MARKER_W,
    USFM_MARKER_WG,
    USFM_MARKER_WH,
    USFM_MARKER_WA,
    USFM_MARKER_JMP,
    USFM_MARKER_TS,
    USFM_MARKER_EF,
    USFM_MARKER_EX,
    USFM_MARKER_ESB,
    USFM_MARKER_ESBE,
    USFM_MARKER_CAT,
    USFM_MARKER_PERIPH,
    USFM_MARKER_CLOSE,
// CSFM_CODEGEN_DATA marker_type end
} USFM_MarkerType;

typedef struct USFM_Document USFM_Document;

bool USFM_Arena_Initialize(USFM_Arena *arena, size_t capacity);

void USFM_Arena_Deinitialize(USFM_Arena *arena);

void USFM_Arena_Reuse(USFM_Arena *arena); // TODO

void USFM_Tokenize(USFM_Arena *arena, USFM_Document *doc);

#endif // CSFM_HEADER


#ifdef CSFM_IMPLEMENTATION
#define CSFM_IMPLEMENTATION

static inline void *USFM_Arena_AlignForward(void *address)
{
    return (void *)(((uintptr_t)(address) + 0xf) & 0xfffffffffffffff0);
}

static inline USFM_ArenaBlock *USFM_ArenaBlock_Allocate(size_t size)
{
    USFM_ArenaBlock *block = malloc(size);
    if (block == NULL)
    {
        return NULL;
    }
    block->previous = NULL;
    block->capacity = size - sizeof(USFM_ArenaBlock);
    block->offset = 0;
    CSFM_ARENA_ASAN_POISON(block->memory, block->capacity);
    return block;
}

bool USFM_Arena_Initialize(USFM_Arena *arena, size_t capacity)
{
    if (arena == NULL)
    {
        return false;
    }
    size_t size = CSFM_ARENA_BLOCK_SIZE_DEFAULT;
    while (capacity + sizeof(USFM_ArenaBlock) > size)
    {
        size += CSFM_ARENA_BLOCK_SIZE_DEFAULT;
    }
    USFM_ArenaBlock *block = USFM_ArenaBlock_Allocate(size);
    if (block == NULL)
    {
        return false;
    }
    arena->first = block;
    arena->current = block;
    return true;
}

void USFM_Arena_Deinitialize(USFM_Arena *arena)
{
    if (arena == NULL)
    {
        return;
    }
    USFM_ArenaBlock *block = arena->current;
    while (block != NULL)
    {
        USFM_ArenaBlock *next = block->previous;
        free(block);
        block = next;
    }
    arena->first = NULL;
    arena->current = NULL;
}

static inline void *USFM_Arena_Push(USFM_Arena *arena, size_t size)
{
    if (arena == NULL || arena->first == NULL || arena->current == NULL)
    {
        return NULL;
    }

    size_t offset = arena->current->offset + CSFM_ARENA_ASAN_BUFFER_SIZE;
    void *memory = &arena->current->memory[offset];
    void *aligned = USFM_Arena_AlignForward(memory);
    size_t alignment_offset = (size_t)((uintptr_t)aligned - (uintptr_t)memory);
    size_t space = arena->current->capacity - (offset + alignment_offset);
    if (space >= size)
    {
        arena->current->offset += CSFM_ARENA_ASAN_BUFFER_SIZE + alignment_offset + size;
        CSFM_ARENA_ASAN_UNPOISON(aligned, size);
        return aligned;
    }
    size_t new_block_capacity = (arena->current->capacity + sizeof(USFM_ArenaBlock)) * 2;
    while ((size + sizeof(USFM_ArenaBlock)) > new_block_capacity)
    {
        new_block_capacity += new_block_capacity;
    }
    USFM_ArenaBlock *block = USFM_ArenaBlock_Allocate(new_block_capacity);
    if (block == NULL)
    {
        return NULL;
    }
    block->previous = arena->current;
    block->offset += CSFM_ARENA_ASAN_BUFFER_SIZE;
    memory = &block->memory[block->offset];
    aligned = USFM_Arena_AlignForward(memory);
    block->offset += (size_t)((uintptr_t)aligned - (uintptr_t)memory) + size;
    CSFM_ARENA_ASAN_UNPOISON(aligned, size);
    arena->current = block;
    return aligned;
}

typedef enum {
    CLASS_OTHER,
    CLASS_WHITESPACE,
    CLASS_CARRIAGE_RETURN,
    CLASS_LINE_FEED,
    CLASS_BACKSLASH,
    CLASS_PLUS,
    CLASS_MINUS,
    CLASS_ASTERISK,
    CLASS_LETTER,
    CLASS_DIGIT,
} USFM_CharacterClass;

#if CSFM_CODEGEN
void USFM_CharacterClass_Generate(uint8_t *characters)
{
    characters[' '] = CLASS_WHITESPACE;
    characters['\t'] = CLASS_WHITESPACE;
    characters['\r'] = CLASS_CARRIAGE_RETURN;
    characters['\n'] = CLASS_LINE_FEED;
    characters['\\'] = CLASS_BACKSLASH;
    characters['+'] = CLASS_PLUS;
    characters['-'] = CLASS_MINUS;
    characters['*'] = CLASS_ASTERISK;
    for (uint8_t c = '0'; c <= '9'; c++)
    {
        characters[c] = CLASS_DIGIT;
    }
    for (uint8_t c = 'A'; c <= 'Z'; c++)
    {
        characters[c] = CLASS_LETTER;
    }
    for (uint8_t c = 'a'; c <= 'z'; c++)
    {
        characters[c] = CLASS_LETTER;
    }
}
#endif

// CSFM_CODEGEN character_class start
/* CSFM_CODEGEN_COMMENT --------------------------------------------------- *
 * NOTE: This code is generated via codegen. Please do not modify manually!
 * Last generated: 2026-09-16 00:18:17 GMT
 * Hash: 6f585606
 * CSFM_CODEGEN_COMMENT --------------------------------------------------- */
static const USFM_CharacterClass USFM_CharacterClass_From_Character[256] = {
    [9] = CLASS_WHITESPACE,
    [10] = CLASS_LINE_FEED,
    [13] = CLASS_CARRIAGE_RETURN,
    [32] = CLASS_WHITESPACE,
    [42] = CLASS_ASTERISK,
    [43] = CLASS_PLUS,
    [45] = CLASS_MINUS,
    [48] = CLASS_DIGIT,
    [49] = CLASS_DIGIT,
    [50] = CLASS_DIGIT,
    [51] = CLASS_DIGIT,
    [52] = CLASS_DIGIT,
    [53] = CLASS_DIGIT,
    [54] = CLASS_DIGIT,
    [55] = CLASS_DIGIT,
    [56] = CLASS_DIGIT,
    [57] = CLASS_DIGIT,
    [65] = CLASS_LETTER,
    [66] = CLASS_LETTER,
    [67] = CLASS_LETTER,
    [68] = CLASS_LETTER,
    [69] = CLASS_LETTER,
    [70] = CLASS_LETTER,
    [71] = CLASS_LETTER,
    [72] = CLASS_LETTER,
    [73] = CLASS_LETTER,
    [74] = CLASS_LETTER,
    [75] = CLASS_LETTER,
    [76] = CLASS_LETTER,
    [77] = CLASS_LETTER,
    [78] = CLASS_LETTER,
    [79] = CLASS_LETTER,
    [80] = CLASS_LETTER,
    [81] = CLASS_LETTER,
    [82] = CLASS_LETTER,
    [83] = CLASS_LETTER,
    [84] = CLASS_LETTER,
    [85] = CLASS_LETTER,
    [86] = CLASS_LETTER,
    [87] = CLASS_LETTER,
    [88] = CLASS_LETTER,
    [89] = CLASS_LETTER,
    [90] = CLASS_LETTER,
    [92] = CLASS_BACKSLASH,
    [97] = CLASS_LETTER,
    [98] = CLASS_LETTER,
    [99] = CLASS_LETTER,
    [100] = CLASS_LETTER,
    [101] = CLASS_LETTER,
    [102] = CLASS_LETTER,
    [103] = CLASS_LETTER,
    [104] = CLASS_LETTER,
    [105] = CLASS_LETTER,
    [106] = CLASS_LETTER,
    [107] = CLASS_LETTER,
    [108] = CLASS_LETTER,
    [109] = CLASS_LETTER,
    [110] = CLASS_LETTER,
    [111] = CLASS_LETTER,
    [112] = CLASS_LETTER,
    [113] = CLASS_LETTER,
    [114] = CLASS_LETTER,
    [115] = CLASS_LETTER,
    [116] = CLASS_LETTER,
    [117] = CLASS_LETTER,
    [118] = CLASS_LETTER,
    [119] = CLASS_LETTER,
    [120] = CLASS_LETTER,
    [121] = CLASS_LETTER,
    [122] = CLASS_LETTER,
};
// CSFM_CODEGEN character_class end

static const USFM_TokenType USFM_TokenType_From_CharacterClass[10] = {
    [CLASS_OTHER] = USFM_TOKEN_UNKNOWN,
    [CLASS_WHITESPACE] = USFM_TOKEN_WHITESPACE,
    [CLASS_CARRIAGE_RETURN] = USFM_TOKEN_NEWLINE,
    [CLASS_LINE_FEED] = USFM_TOKEN_NEWLINE,
    [CLASS_BACKSLASH] = USFM_TOKEN_MARKER_START,
    [CLASS_PLUS] = USFM_TOKEN_PLUS,
    [CLASS_MINUS] = USFM_TOKEN_MINUS,
    [CLASS_ASTERISK] = USFM_TOKEN_ASTERISK,
    [CLASS_LETTER] = USFM_TOKEN_TEXT,
    [CLASS_DIGIT] = USFM_TOKEN_NUMBER,
};

// CSFM_CODEGEN marker_text start
/* CSFM_CODEGEN_COMMENT --------------------------------------------------- *
 * NOTE: This code is generated via codegen. Please do not modify manually!
 * Last generated: 2026-09-16 00:18:17 GMT
 * Hash: 5910522e
 * CSFM_CODEGEN_COMMENT --------------------------------------------------- */
static const char *USFM_MarkerText_From_MarkerType[145] = {
    "",
    "id",
    "usfm",
    "ide",
    "sts",
    "rem",
    "h",
    "toc",
    "toca",
    "imt",
    "is",
    "ip",
    "ipi",
    "im",
    "imi",
    "ipq",
    "imq",
    "ipr",
    "iq",
    "ib",
    "ili",
    "iot",
    "io",
    "ior",
    "iqt",
    "iex",
    "imte",
    "ie",
    "mt",
    "mte",
    "ms",
    "mr",
    "s",
    "sr",
    "r",
    "rq",
    "d",
    "sp",
    "sd",
    "c",
    "ca",
    "cl",
    "cp",
    "cd",
    "v",
    "va",
    "vp",
    "p",
    "m",
    "po",
    "pr",
    "cls",
    "pmo",
    "pm",
    "pmc",
    "pmr",
    "pi",
    "mi",
    "nb",
    "pc",
    "ph",
    "b",
    "q",
    "qr",
    "qc",
    "qs",
    "qa",
    "qac",
    "qm",
    "qd",
    "lh",
    "li",
    "lf",
    "lim",
    "litl",
    "lik",
    "liv",
    "tr",
    "th",
    "thr",
    "tc",
    "tcr",
    "f",
    "fe",
    "fr",
    "fq",
    "fqa",
    "fk",
    "fl",
    "fw",
    "fp",
    "fv",
    "ft",
    "fdc",
    "fm",
    "x",
    "xo",
    "xk",
    "xq",
    "xt",
    "xta",
    "xop",
    "xot",
    "xnt",
    "xdc",
    "add",
    "bk",
    "dc",
    "k",
    "lit",
    "nd",
    "ord",
    "pn",
    "png",
    "addpn",
    "qt",
    "sig",
    "sls",
    "tl",
    "wj",
    "em",
    "bd",
    "it",
    "bdit",
    "no",
    "sc",
    "sup",
    "pb",
    "fig",
    "ndx",
    "rb",
    "pro",
    "w",
    "wg",
    "wh",
    "wa",
    "jmp",
    "ts",
    "ef",
    "ex",
    "esb",
    "esbe",
    "cat",
    "periph",
    "*",
};
// CSFM_CODEGEN marker_text end

typedef struct {
    const char *str;
    uint32_t hash;
    uint8_t length;
} USFM_Marker;

static uint32_t USFM_Marker_Hash(const char *marker_text, uint8_t length)
{
    assert(marker_text != NULL && length > 0);

    uint8_t index_2 = length > 1 ? 1 : 0;
    uint8_t index_n_1 = length > 1 ? length-1 : 0;
    uint8_t index_n = length-1;
    uint32_t hash = (uint32_t)marker_text[0] & 0xFF;
    hash |= (uint32_t)marker_text[index_2] << 8;
    hash |= (uint32_t)marker_text[index_n_1] << 16;
    hash |= (uint32_t)marker_text[index_n] << 24;
    return hash;
}

#if CSFM_CODEGEN
USFM_Marker *USFM_MarkerMap_Generate(const char **markers, size_t *length)
{
    // TODO(mattg): allocate a USFM_Marker buffer, resizing it if necessary to make a perfect hash
    (void)markers;
    (void)length;
    return NULL;
}
#endif
// static void USFM_MarkerMap_Initialize(void)
// {
//     size_t length = sizeof(USFM_MarkerText_From_MarkerType) / sizeof(USFM_MarkerText_From_MarkerType[0]);
//     // NOTE(mattg): skip unknown marker text (index 0)
//     for (size_t i = 1; i < length; i++)
//     {
//         USFM_Marker marker = {0};
//         marker.length = strlen(USFM_MarkerText_From_MarkerType[i]);
//         marker.str = (const char *)USFM_MarkerText_From_MarkerType[i];
//         marker.hash = USFM_Marker_Hash((const char *)USFM_MarkerText_From_MarkerType[i], marker.length);
//         uint32_t mhash = marker.hash % (sizeof(marker_map) / sizeof(marker_map[0]));
//         if (marker_map[mhash].length != 0)
//         {
//             printf("%s (%d) collides with %s (%d)! (%d)\n", marker.str, marker.hash, marker_map[mhash].str, marker_map[mhash].hash, mhash);
//             // assert(false);
//         }
//         marker_map[mhash] = marker;
//     }
//     (void)marker_map;
// }

static inline void USFM_TokenArray_Push(USFM_TokenArray *array, USFM_Token element)
{
    assert(array != NULL);
    if (array->capacity == 0)
    {
        return;
    }
    assert(array->buffer != NULL);
    assert(array->length < array->capacity);
    array->buffer[array->length] = element;
    array->length++;
}

typedef struct {
    const uint8_t *buffer;
    uint32_t length;
} USFM_Buffer;

struct USFM_Document {
    USFM_Buffer input;
    USFM_TokenArray tokens;
};

USFM_Document *USFM_Document_Initialize(USFM_Arena *arena, const char *input, uint32_t length)
{
    if (arena == NULL || input == NULL || length == 0)
    {
        return NULL;
    }

    USFM_Document *doc = (USFM_Document *)USFM_Arena_Push(arena, sizeof(USFM_Document));
    if (doc == NULL)
    {
        return NULL;
    }
    doc->input.buffer = (const uint8_t *)input;
    doc->input.length = length;
    doc->tokens.buffer = (USFM_Token *)USFM_Arena_Push(arena, sizeof(USFM_Token) * length);
    doc->tokens.length = 0;
    if (doc->tokens.buffer == NULL)
    {
        doc->tokens.capacity = 0;
    }
    else
    {
        doc->tokens.capacity = length;
    }
    return doc;
}

void USFM_Tokenize(USFM_Arena *arena, USFM_Document *doc)
{
    (void)arena;
    if (doc == NULL || doc->input.buffer == NULL || doc->tokens.buffer == NULL)
    {
        return;
    }
    // TODO(mattg): Convert the tokenization into a function which returns one token at a time.
    USFM_CharacterClass previous_c = CLASS_OTHER;
    USFM_Token previous = {0};
    size_t i = 0;
    while (i < doc->input.length)
    {
        USFM_CharacterClass c = USFM_CharacterClass_From_Character[doc->input.buffer[i]];
        USFM_Token token = {
            .offset = i,
            .length = 1,
            .type = USFM_TokenType_From_CharacterClass[c],
        };
        bool push_previous = previous.type == USFM_TOKEN_MARKER_START;
        switch (token.type)
        {
        case USFM_TOKEN_UNKNOWN:
        case USFM_TOKEN_WHITESPACE:
            if (previous.type == USFM_TOKEN_WHITESPACE || previous.type == USFM_TOKEN_TEXT)
            {
                previous.length++;
            }
            else
            {
                push_previous = true;
            }
            break;
        case USFM_TOKEN_NEWLINE:
            if (previous.type == USFM_TOKEN_NEWLINE &&
                (previous_c == CLASS_CARRIAGE_RETURN && c == CLASS_LINE_FEED))
            {
                previous.length++;
            }
            else
            {
                push_previous = true;
            }
            break;
        case USFM_TOKEN_MARKER_START:
            push_previous = true;
            break;
        case USFM_TOKEN_MARKER_TEXT:
        case USFM_TOKEN_MARKER_NUMBER:
        case USFM_TOKEN_MARKER_NESTED:
        case USFM_TOKEN_MARKER_CLOSE:
        case USFM_TOKEN_MARKER_SUFFIX:
            // NOTE(mattg): these should be converted to, but never created by the table.
            assert(false);
            break;
        case USFM_TOKEN_TEXT:
            if (previous.type == USFM_TOKEN_MARKER_START || previous.type == USFM_TOKEN_MARKER_NESTED)
            {
                token.type = USFM_TOKEN_MARKER_TEXT;
                push_previous = true;
            }
            else if (previous.type == USFM_TOKEN_TEXT || previous.type == USFM_TOKEN_MARKER_TEXT ||
                previous.type == USFM_TOKEN_MARKER_SUFFIX)
            {
                previous.length++;
            }
            else
            {
                push_previous = true;
            }
            break;
        case USFM_TOKEN_NUMBER:
            if (previous.type == USFM_TOKEN_MARKER_TEXT)
            {
                token.type = USFM_TOKEN_MARKER_NUMBER;
                push_previous = true;
            }
            else if (previous.type == USFM_TOKEN_NUMBER || previous.type == USFM_TOKEN_MARKER_NUMBER)
            {
                previous.length++;
            }
            else
            {
                push_previous = true;
            }
            break;
        case USFM_TOKEN_PLUS:
            if (previous.type == USFM_TOKEN_MARKER_START)
            {
                token.type = USFM_TOKEN_MARKER_NESTED;
            }
            push_previous = true;
            break;
        case USFM_TOKEN_MINUS:
            if (previous.type == USFM_TOKEN_MARKER_TEXT || previous.type == USFM_TOKEN_MARKER_NUMBER)
            {
                token.type = USFM_TOKEN_MARKER_SUFFIX;
            }
            push_previous = true;
            break;
        case USFM_TOKEN_ASTERISK:
            if (previous.type == USFM_TOKEN_MARKER_START || previous.type == USFM_TOKEN_MARKER_TEXT ||
                previous.type == USFM_TOKEN_MARKER_NUMBER || previous.type == USFM_TOKEN_MARKER_SUFFIX)
            {
                token.type = USFM_TOKEN_MARKER_CLOSE;
            }
            push_previous = true;
            break;
        }
        if (push_previous)
        {
            if (i > 0)
            {
                USFM_TokenArray_Push(&doc->tokens, previous);
            }
            previous = token;
        }
        previous_c = c;
        i++;
    }
    USFM_TokenArray_Push(&doc->tokens, previous);
}


#endif // CSFM_IMPLEMENTATION
