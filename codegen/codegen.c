#define CSFM_IMPLEMENTATION
#define CSFM_CODEGEN 1
#define _POSIX_C_SOURCE 1
#include "../csfm.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define TEMP_BUFFER_SIZE 128

typedef enum {
    START,
    END,
} CodeGen_CommentType;

typedef struct {
    char *name;
    size_t length;
    size_t index_start;
    size_t index_end;
    CodeGen_CommentType type;
} CodeGen_Comment;

typedef enum {
    NOT_CODEGEN,
    INVALID_CODEGEN,
    CODEGEN,
} CodeGen_ParseResult;

CodeGen_ParseResult CodeGen_Comment_Parse(char *input, size_t *index,
    size_t line_count, size_t index_at_line_start, CodeGen_CommentType expected_type, CodeGen_Comment *comment)
{
    comment->index_start = *index;
    const char *codegen_comment_start = "// CSFM_CODEGEN ";
    if (memcmp(&input[*index], codegen_comment_start, strlen(codegen_comment_start)) != 0)
    {
        return NOT_CODEGEN;
    }
    *index += strlen(codegen_comment_start);
    comment->name = &input[*index];

    char c = input[*index];
    while (c != ' ')
    {
        (*index)++;
        comment->length++;
        c = input[*index];
    }
    if (comment->length == 0)
    {
        printf("CodeGen Error: Expected a codegen name after \"%s\" at %ld:%ld\n", codegen_comment_start, line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    if (input[*index] != ' ')
    {
        return INVALID_CODEGEN;
        printf("CodeGen Error: Expected ' ' after \"%*.*s\" at %ld:%ld\n", (int)comment->length, (int)comment->length, comment->name, line_count, (*index-index_at_line_start)+1);
    }
    (*index)++;
    const char *start = "start";
    const char *end = "end";
    if (memcmp(&input[*index], start, strlen(start)) == 0)
    {
        comment->type = START;
    }
    else if (memcmp(&input[*index], end, strlen(end)) == 0)
    {
        comment->type = END;
    }
    else
    {
        printf("CodeGen Error: Expected \"%s\" or \"%s\" after \"%*.*s\" at %ld:%ld\n", start, end, (int)comment->length, (int)comment->length, comment->name, line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    if (comment->type != expected_type)
    {
        printf("CodeGen Error: Expected type \"%s\" but received \"%s\" at %ld:%ld\n", expected_type == START ? start : end, comment->type == START ? start : end, line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    *index += comment->type == START ? strlen(start) : strlen(end);
    if (input[*index] != '\n')
    {
        printf("CodeGen Error: Expected \\n after \"%s\" at %ld:%ld\n", comment->type == START ? start : end, line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    comment->index_end = *index + 1;
    return CODEGEN;
}

typedef struct {
    char *buffer;
    size_t length;
    size_t capacity;
} CodeGen_WriteBuffer;

void CodeGen_WriteBuffer_Append(CodeGen_WriteBuffer *buffer, const char *text, size_t length)
{
    assert(buffer->length + length <= buffer->capacity);
    memcpy(&buffer->buffer[buffer->length], text, length);
    buffer->length += length;
}

void CodeGen_GenerationComments_Serialize(CodeGen_WriteBuffer *buffer)
{
    const char *start = "// NOTE: This code is generated via codegen. Please do not modify manually!\n";
    CodeGen_WriteBuffer_Append(buffer, start, strlen(start));
    char temp_buffer[TEMP_BUFFER_SIZE] = {0};
    time_t raw_time = time(NULL);
    struct tm time_info = {0};
    localtime_r(&raw_time, &time_info);
    size_t len = strftime(temp_buffer, TEMP_BUFFER_SIZE, "// Last generated on: %Y-%m-%d\n", &time_info);
    CodeGen_WriteBuffer_Append(buffer, (const char *)temp_buffer, len);
}

void CodeGen_USFM_CharacterClass_Serialize(USFM_CharacterClass *character_class, CodeGen_WriteBuffer *buffer)
{
    CodeGen_GenerationComments_Serialize(buffer);
    const char *start = "static const uint8_t character_class[256] = {\n";
    CodeGen_WriteBuffer_Append(buffer, start, strlen(start));

    for (size_t i = 0; i < 256; i++)
    {
        USFM_CharacterClass class = (USFM_CharacterClass)character_class[i];
        if (class != CLASS_OTHER)
        {
            char *class_enum_str = NULL;
            switch (class)
            {
            case CLASS_OTHER:
                class_enum_str = "CLASS_OTHER";
                break;
            case CLASS_WHITESPACE:
                class_enum_str = "CLASS_WHITESPACE";
                break;
            case CLASS_CARRIAGE_RETURN:
                class_enum_str = "CLASS_CARRIAGE_RETURN";
                break;
            case CLASS_LINE_FEED:
                class_enum_str = "CLASS_LINE_FEED";
                break;
            case CLASS_BACKSLASH:
                class_enum_str = "CLASS_BACKSLASH";
                break;
            case CLASS_LETTER:
                class_enum_str = "CLASS_LETTER";
                break;
            case CLASS_DIGIT:
                class_enum_str = "CLASS_DIGIT";
                break;
            }
            char temp_line_buffer[TEMP_BUFFER_SIZE] = {0};
            int len = snprintf(temp_line_buffer, TEMP_BUFFER_SIZE, "    [%ld] = %s,\n", i, class_enum_str);
            assert(len <= TEMP_BUFFER_SIZE && len > 0);
            CodeGen_WriteBuffer_Append(buffer, (const char *)temp_line_buffer, len);
        }
    }

    const char *end = "};\n";
    CodeGen_WriteBuffer_Append(buffer, end, strlen(end));
}

int main(void)
{
    uint8_t character_class[256] = {0};
    USFM_CharacterClass_Generate((uint8_t *)&character_class);

    const char *path = "../csfm.h";
    int fd = open(path, O_RDONLY);
    assert(fd != -1);

    struct stat statbuf = {0};
    assert(fstat(fd, &statbuf) != -1);

    size_t size = (size_t)statbuf.st_size;
    char *readbuf = malloc(size);
    assert(readbuf != NULL);

    size_t write_size = size * 2;
    CodeGen_WriteBuffer writebuf = {
        .buffer = malloc(write_size),
        .capacity = write_size,
    };
    assert(writebuf.buffer != NULL);
    memset(writebuf.buffer, 0, writebuf.capacity);

    assert(read(fd, readbuf, size) != -1);
    close(fd);
    CodeGen_Comment comments[2] = {0};

    CodeGen_CommentType expected_type = START;
    size_t comment_count = 0;
    size_t index = 0;
    size_t line_count = 1;
    size_t index_at_line_start = 0;
    bool line_start = true;
    while (index < size)
    {
        char c = readbuf[index];
        if (c == '\n')
        {
            index++;
            line_count++;
            index_at_line_start = index;
            line_start = true;
            continue;
        }
        else if (line_start && c == '/')
        {
            CodeGen_ParseResult result = CodeGen_Comment_Parse(readbuf, &index, line_count, index_at_line_start,
                    expected_type, &comments[comment_count]);
            switch (result)
            {
            case CODEGEN:
                assert(comment_count < (sizeof(comments) / sizeof(comments[0])));
                expected_type = expected_type == START ? END : START;
                comment_count++;
                break;
            case INVALID_CODEGEN:
                exit(1);
                break;
            case NOT_CODEGEN:
                line_start = false;
                index++;
            }
        }
        else
        {
            line_start = false;
            index++;
        }
    }
    // NOTE(mattg): there should be an even number of comments at all times
    if (comment_count % 2 != 0)
    {
        printf("CodeGen Error: Expected there to be an even number of comments (%ld)\n", comment_count);
        exit(1);
    }
    // NOTE(mattg): ensure pairs are next to each other
    for (size_t i = 0; i < comment_count; i++)
    {
        int modulo = i % 2;
        assert((modulo == 0 && comments[i].type == START) || (modulo == 1 && comments[i].type == END));
        if (comments[i].type == END)
        {
            // NOTE(mattg): ensure start/end pairs have the same name
            if (comments[i].length != comments[i-1].length ||
                memcmp(comments[i].name, comments[i-1].name, comments[i].length) != 0)
            {
                printf("CodeGen Error: Expected \"%*.*s\" and \"%*.*s\" to equal\n",
                    (int)comments[i-1].length, (int)comments[i-1].length, comments[i-1].name,
                    (int)comments[i].length, (int)comments[i].length, comments[i].name);
                exit(1);
            }
        }
    }
    size_t copy_section_start = 0;
    size_t copy_section_end = 0;
    for (size_t i = 0; i < comment_count; i++)
    {
        if (comments[i].type == START)
        {
            copy_section_end = comments[i].index_end;
            size_t length = copy_section_end-copy_section_start;
            CodeGen_WriteBuffer_Append(&writebuf, (const char *)&readbuf[copy_section_start], length);
        }
        else if (comments[i].type == END)
        {
            copy_section_start = comments[i].index_start;
            copy_section_end = size;
            // TODO: do the actual insertion
            const char *cc = "character_class";
            if (memcmp(comments[i].name, cc, strlen(cc)) == 0)
            {
                CodeGen_USFM_CharacterClass_Serialize((uint8_t *)character_class, &writebuf);
            }
        }
    }
    size_t length = copy_section_end-copy_section_start;
    CodeGen_WriteBuffer_Append(&writebuf, (const char *)&readbuf[copy_section_start], length);

    fd = open(path, O_WRONLY|O_TRUNC);
    assert(fd != -1);
    assert(write(fd, writebuf.buffer, writebuf.length) != -1);
    close(fd);

    free(readbuf);
    free(writebuf.buffer);
    return 0;
}
