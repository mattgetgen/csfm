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
# define CSFM_ARENA_SAN_BUFFER_SIZE 0
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
    USFM_TOKEN_BACKSLASH,
    USFM_TOKEN_TEXT,
    USFM_TOKEN_NUMBER,
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
    CLASS_LETTER,
    CLASS_DIGIT,
} USFM_CharacterClass;

// CSFM_CODEGEN character_class start
static const uint8_t character_class[256] = {
    [9] = CLASS_WHITESPACE,
    [10] = CLASS_LINE_FEED,
    [13] = CLASS_CARRIAGE_RETURN,
    [32] = CLASS_WHITESPACE,
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

#if CSFM_CODEGEN
void USFM_CharacterClass_Generate(uint8_t *characters)
{
    characters['\\'] = (uint8_t)CLASS_BACKSLASH;
    characters[' '] = (uint8_t)CLASS_WHITESPACE;
    characters['\t'] = (uint8_t)CLASS_WHITESPACE;
    characters['\r'] = (uint8_t)CLASS_CARRIAGE_RETURN;
    characters['\n'] = (uint8_t)CLASS_LINE_FEED;
    for (uint8_t c = '0'; c <= '9'; c++)
    {
        characters[c] = (uint8_t)CLASS_DIGIT;
    }
    for (uint8_t c = 'A'; c <= 'Z'; c++)
    {
        characters[c] = (uint8_t)CLASS_LETTER;
    }
    for (uint8_t c = 'a'; c <= 'z'; c++)
    {
        characters[c] = (uint8_t)CLASS_LETTER;
    }
}
#endif

typedef struct {
    const char *str;
    uint32_t hash;
    uint8_t length;
} USFM_Marker;

static USFM_Marker marker_map[256] = {0};

const char *markers[] = {
    "id",
    "usfm",
    "ide",
    "h",
    "c",
    "p",
    "v",
};

static uint32_t USFM_Marker_Hash(const char *marker_text, uint8_t length)
{
    size_t min_length = length > sizeof(uint32_t) ? sizeof(uint32_t) : length;
    uint32_t hash = 0;
    for (size_t i = 0; i < min_length; i++)
    {
        hash |= marker_text[i] << (8 * i);
    }
    return hash;
}

static void USFM_MarkerMap_Initialize(void)
{
    size_t length = sizeof(markers) / sizeof(markers[0]);
    for (size_t i = 0; i < length; i++)
    {
        USFM_Marker marker = {0};
        marker.length = strlen(markers[i]);
        // if (marker.length <= sizeof(uintptr_t))
        // {
        //     uintptr_t str = 0;
        //     for (size_t j = 0; j < marker.length; j++)
        //     {
        //         str |= markers[i][j] << (8 * j);
        //     }
        //     marker.str = (const char *)str;
        // }
        // else
        {
            marker.str = (const char *)markers[i];
        }
        marker.hash = USFM_Marker_Hash((const char *)markers[i], marker.length);
        uint32_t hash = marker.hash % (sizeof(marker_map) / sizeof(marker_map[0]));
        if (marker_map[hash].length != 0)
        {
            printf("%s collides with %s!\n", marker.str, marker_map[hash].str);
            assert(false);
        }
        marker_map[hash] = marker;
    }
    (void)marker_map;
}

static inline USFM_CharacterClass classify_character(uint8_t character)
{
    return (USFM_CharacterClass)character_class[character];
}

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
    // USFM_MarkerMap_Initialize();

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

    USFM_CharacterClass previous_c = CLASS_OTHER;
    USFM_Token previous = {0};
    size_t i = 0;
    while (i < doc->input.length)
    {
        USFM_Token token = {
            .offset = i,
            .length = 1,
        };
        USFM_CharacterClass c = classify_character(doc->input.buffer[i]);
        switch (c)
        {
        case CLASS_OTHER:
            token.type = USFM_TOKEN_UNKNOWN;
            break;
        case CLASS_WHITESPACE:
            token.type = USFM_TOKEN_WHITESPACE;
            break;
        case CLASS_CARRIAGE_RETURN:
        case CLASS_LINE_FEED:
            token.type = USFM_TOKEN_NEWLINE;
            break;
        case CLASS_BACKSLASH:
            token.type = USFM_TOKEN_BACKSLASH;
            break;
        case CLASS_LETTER:
            token.type = USFM_TOKEN_TEXT;
            break;
        case CLASS_DIGIT:
            token.type = USFM_TOKEN_NUMBER;
            break;
        }
        bool push_previous = previous.type == USFM_TOKEN_BACKSLASH;
        switch (token.type)
        {
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
        case USFM_TOKEN_BACKSLASH:
            push_previous = true;
            break;
        default:
            if (previous.type == token.type)
            {
                previous.length++;
            }
            else
            {
                push_previous = true;
            }
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
