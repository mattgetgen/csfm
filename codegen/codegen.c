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
#define CODEGEN_MARKER_TEXT_CAPACITY 200

typedef enum {
    START,
    END,
} CodeGen_CommentType;

typedef struct {
    char *name;
    size_t length;
    size_t index_start;
    size_t index_end;
    bool is_data;
    CodeGen_CommentType type;
} CodeGen_Comment;

typedef struct {
    char *name;
    size_t name_length;
    size_t start_comment_index_start;
    size_t start_comment_index_end;
    size_t end_comment_index_start;
    size_t end_comment_index_end;
    uint64_t content_hash;
    bool is_data;
} CodeGen_CommentBlock;

typedef enum {
    NOT_CODEGEN,
    INVALID_CODEGEN,
    CODEGEN,
} CodeGen_ParseResult;

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

CodeGen_ParseResult CodeGen_Comment_Parse(char *input, size_t *index,
    size_t line_count, size_t index_at_line_start, CodeGen_CommentType expected_type, CodeGen_Comment *comment)
{
    assert(*index == index_at_line_start);
    comment->index_start = *index;
    const char *codegen_comment_start = "// CSFM_CODEGEN ";
    const char *codegen_data_comment_start = "// CSFM_CODEGEN_DATA ";
    if (memcmp(&input[*index], codegen_comment_start, strlen(codegen_comment_start)) == 0)
    {
        *index += strlen(codegen_comment_start);
        comment->name = &input[*index];
        comment->is_data = false;
    }
    else if (memcmp(&input[*index], codegen_data_comment_start, strlen(codegen_data_comment_start)) == 0)
    {
        *index += strlen(codegen_data_comment_start);
        comment->name = &input[*index];
        comment->is_data = true;
    }
    else
    {
        return NOT_CODEGEN;
    }

    char c = input[*index];
    while (c != ' ')
    {
        (*index)++;
        comment->length++;
        c = input[*index];
    }
    if (comment->length == 0)
    {
        printf("CodeGen Error: Expected a codegen name after \"%s\" at %ld:%ld\n",
            comment->is_data ? codegen_data_comment_start : codegen_comment_start,
            line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    if (input[*index] != ' ')
    {
        return INVALID_CODEGEN;
        printf("CodeGen Error: Expected ' ' after \"%*.*s\" at %ld:%ld\n",
            (int)comment->length, (int)comment->length, comment->name,
            line_count, (*index-index_at_line_start)+1);
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
        printf("CodeGen Error: Expected \"%s\" or \"%s\" after \"%*.*s\" at %ld:%ld\n",
            start, end, (int)comment->length, (int)comment->length, comment->name,
            line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    if (comment->type != expected_type)
    {
        printf("CodeGen Error: Expected type \"%s\" but received \"%s\" at %ld:%ld\n",
            expected_type == START ? start : end, comment->type == START ? start : end,
            line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    *index += comment->type == START ? strlen(start) : strlen(end);
    if (input[*index] != '\n')
    {
        printf("CodeGen Error: Expected \\n after \"%s\" at %ld:%ld\n",
            comment->type == START ? start : end,
            line_count, (*index-index_at_line_start)+1);
        return INVALID_CODEGEN;
    }
    comment->index_end = *index + 1;
    return CODEGEN;
}

uint64_t CodeGen_ContentHash_Calculate(char *input, size_t start, size_t end)
{
    // NOTE(mattg): Based on the djb2 hash algorithm
    assert(input != NULL);
    assert (start <= end);
    uint64_t hash = 5381;
    for (size_t i = start; i < end; i++)
    {
        hash = ((hash << 5) + hash) + input[i];
    }
    return hash;
}

void CodeGen_CommentBlock_GetHash(char *input, CodeGen_CommentBlock *block)
{
    assert(input != NULL);
    assert(block != NULL);
    assert(!block->is_data);
    const char *multiline_comment_start = "/*";
    const char *multiline_comment_end = "*/";
    size_t start = block->start_comment_index_end;
    size_t end = block->end_comment_index_start;
    if (memcmp(&input[start], multiline_comment_start, strlen(multiline_comment_start)) == 0)
    {
        start += strlen(multiline_comment_start);
        // NOTE(mattg): advance "start" until after multiline comment
        while (start < end)
        {
            if (input[start] == '*' &&
                memcmp(&input[start], multiline_comment_end, strlen(multiline_comment_end)) == 0)
            {
                start += strlen(multiline_comment_end) + 1;
                break;
            }
            else
            {
                start++;
            }
        }
    }
    block->content_hash = CodeGen_ContentHash_Calculate(input, start, end);
}

void CodeGen_GenerationComments_Serialize(CodeGen_WriteBuffer *buffer, uint64_t content_hash)
{
    const char *start = "/* CSFM_CODEGEN_COMMENT --------------------------------------------------- *\n";
    const char *note  = " * NOTE: This code is generated via codegen. Please do not modify manually!\n";
    const char *date  = " * Last generated on: %F %T %Z\n";
    const char *hash  = " * Hash: %x\n";
    const char *end   = " * CSFM_CODEGEN_COMMENT --------------------------------------------------- */\n";
    CodeGen_WriteBuffer_Append(buffer, start, strlen(start));
    CodeGen_WriteBuffer_Append(buffer, note, strlen(note));

    char temp_time_buffer[TEMP_BUFFER_SIZE] = {0};
    time_t raw_time = time(NULL);
    struct tm time_info = {0};
    localtime_r(&raw_time, &time_info);
    size_t time_len = strftime(temp_time_buffer, TEMP_BUFFER_SIZE, date, &time_info);
    assert(time_len <= TEMP_BUFFER_SIZE);
    CodeGen_WriteBuffer_Append(buffer, (const char *)temp_time_buffer, time_len);

    char temp_hash_buffer[TEMP_BUFFER_SIZE] = {0};
    size_t hash_len = snprintf(temp_hash_buffer, TEMP_BUFFER_SIZE, hash, content_hash);
    assert(hash_len <= TEMP_BUFFER_SIZE);
    CodeGen_WriteBuffer_Append(buffer, (const char *)temp_hash_buffer, hash_len);
    CodeGen_WriteBuffer_Append(buffer, end, strlen(end));
}

bool CodeGen_USFM_CharacterClass_Serialize(USFM_CharacterClass *character_class, CodeGen_WriteBuffer *buffer,
    uint64_t current_hash)
{
    CodeGen_WriteBuffer temp_buffer = {
        .buffer = malloc(sizeof(char) * buffer->capacity),
        .capacity = buffer->capacity,
    };
    assert(temp_buffer.buffer != NULL);
    const char *start = "static const USFM_CharacterClass USFM_CharacterClass_From_Character[256] = {\n";
    CodeGen_WriteBuffer_Append(&temp_buffer, start, strlen(start));

    for (size_t i = 0; i < 256; i++)
    {
        USFM_CharacterClass class = character_class[i];
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
            case CLASS_PLUS:
                class_enum_str = "CLASS_PLUS";
                break;
            case CLASS_MINUS:
                class_enum_str = "CLASS_MINUS";
                break;
            case CLASS_ASTERISK:
                class_enum_str = "CLASS_ASTERISK";
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
            CodeGen_WriteBuffer_Append(&temp_buffer, (const char *)temp_line_buffer, len);
        }
    }

    const char *end = "};\n";
    CodeGen_WriteBuffer_Append(&temp_buffer, end, strlen(end));

    uint64_t hash = CodeGen_ContentHash_Calculate(temp_buffer.buffer, 0, temp_buffer.length);
    bool dont_rewrite = hash == current_hash;
    if (!dont_rewrite)
    {
        printf("Writing new version of character_class\n");
        CodeGen_GenerationComments_Serialize(buffer, hash);
        CodeGen_WriteBuffer_Append(buffer, temp_buffer.buffer, temp_buffer.length);
    }
    free(temp_buffer.buffer);
    return dont_rewrite;
}

char **CodeGen_USFM_MarkerType_Parse(char *input, CodeGen_CommentBlock block, size_t *length)
{
    const char *usfm_marker_prefix = "    USFM_MARKER_";
    const size_t usfm_marker_prefix_len = strlen(usfm_marker_prefix);
    bool usfm_marker_prefix_found = false;

    const char *unknown_marker = "UNKNOWN";
    const size_t unknown_marker_len = strlen(unknown_marker);
    bool unknown_marker_found = false;

    const char *close_marker = "CLOSE";
    const size_t close_marker_len = strlen(close_marker);
    bool close_marker_found = false;
    const char *close_marker_text = "*";

    *length = 0;
    char **array = malloc(sizeof(char *) * CODEGEN_MARKER_TEXT_CAPACITY);
    assert(array != NULL);

    size_t line_start = block.start_comment_index_start;
    size_t index = block.start_comment_index_start;

    while (index <= block.end_comment_index_start)
    {
        if (memcmp(&input[line_start], usfm_marker_prefix, usfm_marker_prefix_len) == 0)
        {
            usfm_marker_prefix_found = true;
            index += usfm_marker_prefix_len;
            char *marker = &input[index];
            size_t marker_length = 0;
            while (input[index] >= 'A' && input[index] <= 'Z')
            {
                marker_length++;
                index++;
            }
            if (memcmp(marker, unknown_marker, unknown_marker_len) == 0)
            {
                unknown_marker_found = true;
                marker = NULL;
                marker_length = 0;

            }
            else if (memcmp(marker, close_marker, close_marker_len) == 0)
            {
                close_marker_found = true;
                marker = (char *)close_marker_text;
                marker_length = 1;
            }
            char *text = malloc(marker_length+1);
            assert(text != NULL);
            memset(text, 0, marker_length+1);
            for (size_t i = 0; i < marker_length; i++)
            {
                if (marker[i] >= 'A' && marker[i] <= 'Z')
                {
                    text[i] = marker[i] + (97-65);
                }
                else
                {
                    text[i] = marker[i];
                }
            }
            if (*length < CODEGEN_MARKER_TEXT_CAPACITY)
            {
                array[*length] = text;
                (*length)++;
            }
            else
            {
                printf("CodeGen Error: Too many markers to fit in the default capacity of %d!\n", CODEGEN_MARKER_TEXT_CAPACITY);
                exit(1);
            }
        }
        // NOTE(mattg): go to end of newline
        while (index <= block.end_comment_index_start && input[index] != '\n')
        {
            index++;
            if (input[index] == '\n')
            {
                index++;
                line_start = index;
                break;
            }
        }
    }
    if (!usfm_marker_prefix_found)
    {
        printf("CodeGen Warning: \"%s\" prefix not found!\n", usfm_marker_prefix);
    }
    if (!unknown_marker_found)
    {
        printf("CodeGen Warning: \"%s\" marker not found!\n", unknown_marker);
    }
    if (!close_marker_found)
    {
        printf("CodeGen Warning: \"%s\" marker not found!\n", close_marker);
    }
    return array;
}

bool CodeGen_USFM_MarkerText_Serialize(CodeGen_WriteBuffer *buffer, char **array, size_t length,
    uint64_t current_hash)
{
    CodeGen_WriteBuffer temp_buffer = {
        .buffer = malloc(sizeof(char) * buffer->capacity),
        .capacity = buffer->capacity,
    };
    char start_temp_buffer[TEMP_BUFFER_SIZE] = {0};
    size_t start_len = snprintf(start_temp_buffer, TEMP_BUFFER_SIZE, "static const char *USFM_MarkerText_From_MarkerType[%ld] = {\n", length);
    assert(start_len <= TEMP_BUFFER_SIZE);
    CodeGen_WriteBuffer_Append(&temp_buffer, (const char *)start_temp_buffer, start_len);

    for (size_t i = 0; i < length; i++)
    {
        char temp_line_buffer[TEMP_BUFFER_SIZE] = {0};
        size_t len = snprintf(temp_line_buffer, TEMP_BUFFER_SIZE, "    \"%s\",\n", array[i]);
        assert(len <= TEMP_BUFFER_SIZE);
        CodeGen_WriteBuffer_Append(&temp_buffer, (const char *)temp_line_buffer, len);
    }

    const char *end = "};\n";
    CodeGen_WriteBuffer_Append(&temp_buffer, end, strlen(end));

    uint64_t hash = CodeGen_ContentHash_Calculate(temp_buffer.buffer, 0, temp_buffer.length);
    bool dont_rewrite = hash == current_hash;
    if (!dont_rewrite)
    {
        printf("Writing new version of marker_text\n");
        CodeGen_GenerationComments_Serialize(buffer, hash);
        CodeGen_WriteBuffer_Append(buffer, temp_buffer.buffer, temp_buffer.length);
    }
    free(temp_buffer.buffer);
    return dont_rewrite;
}

int main(int argc, char **argv)
{
    uint8_t character_class[256] = {0};
    USFM_CharacterClass_Generate((uint8_t *)&character_class);

    bool overwrite = false;
    const char *overwrite_arg_short = "-o";
    const char *overwrite_arg_long = "--overwrite";
    if (argc > 1)
    {
        for (size_t i = 1; i < (size_t)argc; i++)
        {
            if (memcmp(argv[i], overwrite_arg_short, strlen(overwrite_arg_short)) == 0)
            {
                overwrite = true;
            }
            else if (memcmp(argv[i], overwrite_arg_long, strlen(overwrite_arg_long)) == 0)
            {
                overwrite = true;
            }
        }
    }
    const char *input_path = "../csfm.h";
    const char *output_path = overwrite ? input_path : "../csfm_copy.h";
    printf("Writing to %s\n", output_path);
    int fd = open(input_path, O_RDONLY);
    assert(fd != -1);

    struct stat statbuf = {0};
    assert(fstat(fd, &statbuf) != -1);
    mode_t input_mode = statbuf.st_mode;
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
    CodeGen_Comment comments[10] = {0};
    size_t comment_capacity = sizeof(comments) / sizeof(comments[0]);
    size_t comment_length = 0;

    CodeGen_CommentType expected_type = START;
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
                    expected_type, &comments[comment_length]);
            switch (result)
            {
            case CODEGEN:
                assert(comment_length < comment_capacity);
                expected_type = expected_type == START ? END : START;
                comment_length++;
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
    if (comment_length % 2 != 0)
    {
        printf("CodeGen Error: Expected there to be an even number of comments (%ld)\n", comment_length);
        exit(1);
    }
    // NOTE(mattg): ensure pairs are next to each other
    for (size_t i = 0; i < comment_length; i++)
    {
        int modulo = i % 2;
        assert((modulo == 0 && comments[i].type == START) || (modulo == 1 && comments[i].type == END));
        if (comments[i].type == END)
        {
            if (comments[i].is_data != comments[i-1].is_data)
            {
                printf("CodeGen Error: Expected comment pairs to both be data comments\n");
                exit(1);
            }
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
    CodeGen_CommentBlock blocks[5] = {0};
    size_t block_capacity = sizeof(blocks) / sizeof(blocks[0]);
    size_t block_length = 0;

    for (size_t i = 0; i < comment_length; i++)
    {
        CodeGen_CommentBlock block = {0};

        assert(comments[i].type == START);
        CodeGen_Comment start_comment = comments[i];
        block.name = start_comment.name;
        block.name_length = start_comment.length;
        block.start_comment_index_start = start_comment.index_start;
        block.start_comment_index_end = start_comment.index_end;
        block.is_data = start_comment.is_data;

        i++;
        assert(comments[i].type == END);
        CodeGen_Comment end_comment = comments[i];
        block.end_comment_index_start = end_comment.index_start;
        block.end_comment_index_end = end_comment.index_end;

        assert(block_length < block_capacity);
        blocks[block_length] = block;
        block_length++;
    }
    
    const char *mt = "marker_type";
    const size_t mt_len = strlen(mt);
    char **marker_texts = NULL;
    size_t marker_texts_length = 0;
    for (size_t i = 0; i < block_length; i++)
    {
        if (blocks[i].is_data)
        {
            if (memcmp(blocks[i].name, mt, mt_len) == 0)
            {
                marker_texts = CodeGen_USFM_MarkerType_Parse(readbuf, blocks[i], &marker_texts_length);
            }
        }
        else
        {
            CodeGen_CommentBlock_GetHash(readbuf, &blocks[i]);
        }
    }

    size_t copy_section_start = 0;
    size_t copy_section_end = 0;
    if (block_length > 0)
    {
        copy_section_end = blocks[0].start_comment_index_end;
        size_t length = copy_section_end-copy_section_start;
        CodeGen_WriteBuffer_Append(&writebuf, (const char *)&readbuf[copy_section_start], length);
    }
    bool content_changed = false;
    for (size_t i = 0; i < block_length; i++)
    {
        CodeGen_CommentBlock block = blocks[i];
        bool should_copy_content = false;
        if (block.is_data)
        {
            should_copy_content = true;
        }
        else
        {
            const char *cc = "character_class";
            const char *mt = "marker_text";
            if (memcmp(block.name, cc, strlen(cc)) == 0)
            {
                if (CodeGen_USFM_CharacterClass_Serialize((uint8_t *)character_class, &writebuf,
                    block.content_hash))
                {
                    should_copy_content = true;
                }
                else
                {
                    content_changed = true;
                }
            }
            else if (memcmp(block.name, mt, strlen(mt)) == 0)
            {
                if (CodeGen_USFM_MarkerText_Serialize(&writebuf, marker_texts, marker_texts_length,
                    block.content_hash))
                {
                    should_copy_content = true;
                }
                else
                {
                    content_changed = true;
                }
            }
        }
        if (should_copy_content)
        {
            size_t start = block.start_comment_index_end;
            size_t end = block.end_comment_index_start;
            size_t length = end-start;
            CodeGen_WriteBuffer_Append(&writebuf, (const char *)&readbuf[start], length);
        }
        // copy current to next, or to end of file
        copy_section_start = block.end_comment_index_start;
        copy_section_end = i < block_length-1 ? blocks[i+1].start_comment_index_end : size;
        size_t length = copy_section_end - copy_section_start;
        CodeGen_WriteBuffer_Append(&writebuf, (const char *)&readbuf[copy_section_start], length);
    }
    if (writebuf.length > 0)
    {
        fd = open(output_path, O_CREAT|O_WRONLY|O_TRUNC, input_mode);
        assert(fd != -1);
        assert(write(fd, writebuf.buffer, writebuf.length) != -1);
        close(fd);
    }
    if (!content_changed)
    {
        printf("No content changed!\n");
    }

    for (size_t i = 0; i < marker_texts_length; i++)
    {
        free(marker_texts[i]);
    }
    free(marker_texts);

    free(readbuf);
    free(writebuf.buffer);
    return 0;
}
