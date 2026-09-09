#define _DEFAULT_SOURCE 1
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <x86intrin.h>

#define CSFM_IMPLEMENTATION
#include "csfm.h"

typedef struct {
    long cycles;
    struct timespec time;
} Timer;

void getTime(Timer *timer)
{
    timer->cycles = __rdtsc();
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer->time) != 0) {
        exit(1);
    }
    return;
}

void printTimeData(Timer start, Timer end, size_t size)
{
    long cycles = end.cycles - start.cycles;
    float cyclesPerByte = (float)cycles / (float)size;
    long nanoseconds = ((end.time.tv_sec - start.time.tv_sec) * (long)1e9) + (
        end.time.tv_nsec - start.time.tv_nsec
    );
    float nanosPerByte = (float)nanoseconds / (float)size;
    printf("%ld bytes, %ld cycles, %ld ns\n", size, cycles, nanoseconds);
    printf("%.2f cycles/byte, %.2f ns/byte\n", cyclesPerByte, nanosPerByte);
}

int main(void)
{
    // const char *path = "/home/mgetgen/repos/usfm/simdusfm/src/usfm/HPUX.usfm";
    // const char *path = "/home/mgetgen/repos/usfm/example_usfm/HPUX/01GENHPUX.SFM";
    // const char *path = "/home/mgetgen/repos/usfm/example_usfm/WEB/25-JEReng-web.usfm";
    const char *path = "./test.usfm";

    printf("Reading file:\n");
    Timer start = {0};
    Timer end = {0};
    getTime(&start);
    // TODO(matt): test if we can get consistent performance from O_DIRECT
    // (minimizes OS caching, check man 2 open)
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        printf("Error: `open` failed\n");
        return 1;
    }

    struct stat statbuf = {0};
    if (fstat(fd, &statbuf) == -1)
    {
        printf("Error: `fstat` failed\n");
        return 1;
    }

    size_t size = (size_t)statbuf.st_size;
    void *filebuf = malloc(size);
    if (filebuf == NULL)
    {
        printf("Error: `malloc` failed\n");
        return 1;
    }

    if (read(fd, filebuf, size) == -1)
    {
        printf("Error: `read` failed\n");
        return 1;
    }
    getTime(&end);
    printTimeData(start, end, size);

    if (close(fd) == -1)
    {
        printf("Error: `close` failed\n");
        return 1;
    }

    printf("\nInitializing parser:\n");
    getTime(&start);
    USFM_Arena arena = {0};
    assert(USFM_Arena_Initialize(&arena, 0));
    USFM_Document *doc = USFM_Document_Initialize(&arena, (const char *)filebuf, size);
    assert(doc != NULL);
    getTime(&end);
    printTimeData(start, end, size);
    
    printf("\nTokenizing file:\n");
    getTime(&start);
    USFM_Tokenize(&arena, doc);
    getTime(&end);
    
    for (size_t i = 0; i < doc->tokens.length; i++)
    {
        USFM_Token token = doc->tokens.buffer[i];
        char *string = (char *)&doc->input.buffer[token.offset];
        switch (token.type)
        {
        case USFM_TOKEN_UNKNOWN:
            printf("[U<%*.*s>]", token.length, token.length, string);
            break;
        case USFM_TOKEN_WHITESPACE:
            printf(" ");
            break;
        case USFM_TOKEN_NEWLINE:
            printf("[NL]\n");
            break;
        case USFM_TOKEN_MARKER_START:
            printf("\\");
            break;
        case USFM_TOKEN_MARKER_TEXT:
            printf("[MT<%*.*s>]", token.length, token.length, string);
            break;
        case USFM_TOKEN_MARKER_NUMBER:
            printf("[M#<%*.*s>]", token.length, token.length, string);
            break;
        case USFM_TOKEN_MARKER_NESTED:
            printf("+");
            break;
        case USFM_TOKEN_MARKER_CLOSE:
            printf("*");
            break;
        case USFM_TOKEN_MARKER_SUFFIX:
            printf("[MS<%*.*s>]", token.length, token.length, string);
            break;
        case USFM_TOKEN_TEXT:
            printf("[T<%*.*s>]", token.length, token.length, string);
            break;
        case USFM_TOKEN_NUMBER:
            printf("[#<%*.*s>]", token.length, token.length, string);
            break;
        default:
            printf("[%*.*s]", token.length, token.length, string);
        }
    }
    float tokens_per_byte = (float)doc->tokens.length / (float)size;
    printf("\n# tokens: %d\n", doc->tokens.length);
    printf("tokens/byte: %.2f\n", tokens_per_byte);
    
    printTimeData(start, end, size);

    // printf("\nParsing file:\n");
    // getTime(&start);
    //
    // CSFM_ParseResult parseResult = CSFM_Parse((uint8_t *)filebuf, size);
    //
    // getTime(&end);
    //
    // // for (uint32_t i = 0; i < parseResult.tree.length; i++) {
    // //     CSFM_Node node = CSFM_NodeArray_get(parseResult.tree, i);
    // //     CSFM_Node_print(node, parseResult.input);
    // // }
    // uint32_t numNodes = parseResult.tree.length;
    // printf("\n# nodes: %d\n", numNodes);
    // printTimeData(start, end, size);
    //
    // float tokensPerByte = (float)numTokens / (float)size;
    // float nodesPerByte = (float)numNodes / (float)size;
    // float nodesPerToken = (float)numNodes / (float)numTokens;
    // printf(
    //     "\n%.4f tokens/byte, %.4f nodes/byte, %.4f nodes/token\n",
    //     tokensPerByte, nodesPerByte, nodesPerToken
    // );
    //
    // CSFM_NodeArray_deallocate(&parseResult.tree);
    // CSFM_TokenArray_deallocate(&tokenResult.tokens);
    USFM_Arena_Deinitialize(&arena);
    free(filebuf);

    return 0;
}
