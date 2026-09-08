#define CSFM_IMPLEMENTATION
#define CSFM_CODEGEN 1
#include "../csfm.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef enum {
    START,
    END,
} CodegenCommentType;

typedef struct {
    char *name;
    size_t length;
    size_t index_start;
    size_t index_end;
    CodegenCommentType type;
} CodegenComment;

typedef enum {
    NOT_CODEGEN,
    INVALID_CODEGEN,
    CODEGEN,
} CodegenParseResult;

CodegenParseResult parse_codegen_comment(char *input, size_t *index,
    size_t line_count, size_t index_at_line_start, CodegenCommentType expected_type, CodegenComment *comment)
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
        printf("codegen error: Expected a codegen name after \"%s\" at %ld:%ld\n", codegen_comment_start, line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    if (input[*index] != ' ')
    {
        return INVALID_CODEGEN;
        printf("codegen error: Expected ' ' after \"%*.*s\" at %ld:%ld\n", (int)comment->length, (int)comment->length, comment->name, line_count, (*index-index_at_line_start)+1);
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
        printf("codegen error: Expected \"%s\" or \"%s\" after \"%*.*s\" at %ld:%ld\n", start, end, (int)comment->length, (int)comment->length, comment->name, line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    if (comment->type != expected_type)
    {
        printf("codegen error: Expected type \"%s\" but received \"%s\" at %ld:%ld\n", expected_type == START ? start : end, comment->type == START ? start : end, line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    *index += comment->type == START ? strlen(start) : strlen(end);
    if (input[*index] != '\n')
    {
        printf("codegen error: Expected \\n after \"%s\" at %ld:%ld\n", comment->type == START ? start : end, line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    comment->index_end = *index + 1;
    return CODEGEN;
}

void USFM_CharacterClass_Serialize(USFM_CharacterClass *character_class, char *buffer, size_t *index,
    size_t buffer_size)
{
    const char *start = "static const uint8_t character_class[256] = {\n";
    size_t start_len = strlen(start);
    assert(*index + start_len <= buffer_size);
    memcpy(&buffer[*index], start, start_len);
    *index += start_len;

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
            int len = snprintf(&buffer[*index], buffer_size - *index, "    [%ld] = %s,\n", i, class_enum_str);
            assert(len > 0);
            assert(*index + len <= buffer_size);
            *index += len;
        }
    }

    const char *end = "};\n";
    size_t end_len = strlen(end);
    assert(*index + end_len <= buffer_size);
    memcpy(&buffer[*index], end, end_len);
    *index += end_len;
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
    size_t write_length = 0;
    char *writebuf = malloc(write_size);
    assert(writebuf != NULL);
    memset(writebuf, 0, write_size);

    assert(read(fd, readbuf, size) != -1);
    close(fd);
    CodegenComment comments[2] = {0};

    CodegenCommentType expected_type = START;
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
            CodegenParseResult result = parse_codegen_comment(readbuf, &index, line_count, index_at_line_start,
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
        printf("codegen error: Expected there to be an even number of comments (%ld)\n", comment_count);
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
                printf("codegen error: Expected \"%*.*s\" and \"%*.*s\" to equal\n",
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
            assert(write_length + length <= write_size);
            memcpy(&writebuf[write_length], &readbuf[copy_section_start], length);
            write_length += length;
        }
        else if (comments[i].type == END)
        {
            copy_section_start = comments[i].index_start;
            copy_section_end = size;
            // TODO: do the actual insertion
            const char *cc = "character_class";
            if (memcmp(comments[i].name, cc, strlen(cc)) == 0)
            {
                USFM_CharacterClass_Serialize((uint8_t *)character_class, writebuf, &write_length, write_size);
            }
        }
    }
    size_t length = copy_section_end-copy_section_start;
    assert(write_length + length <= write_size);
    memcpy(&writebuf[write_length], &readbuf[copy_section_start], length);
    write_length += length;

    fd = open(path, O_WRONLY|O_TRUNC);
    assert(fd != -1);
    assert(write(fd, writebuf, write_length) != -1);
    close(fd);

    free(readbuf);
    free(writebuf);
    return 0;
}
