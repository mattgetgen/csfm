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

void getTime(Timer *timer) {
    timer->cycles = __rdtsc();
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer->time) != 0) {
        exit(1);
    }
    return;
}

void printTimeData(Timer start, Timer end, size_t size) {
    long cycles = end.cycles - start.cycles;
    float cyclesPerByte = (float)cycles / (float)size;
    long nanoseconds = ((end.time.tv_sec - start.time.tv_sec) * (long)1e9) + (
        end.time.tv_nsec - start.time.tv_nsec
    );
    float nanosPerByte = (float)nanoseconds / (float)size;
    printf("%ld bytes, %ld cycles, %ld ns\n", size, cycles, nanoseconds);
    printf("%.2f cycles/byte, %.2f ns/byte\n", cyclesPerByte, nanosPerByte);
}

int main(void) {
    const char *path = "/home/mgetgen/repos/usfm/simdusfm/src/usfm/HPUX.usfm";
    // const char *path = "/home/mgetgen/repos/usfm/example_usfm/HPUX/01GENHPUX.SFM";
    // const char *path = "/home/mgetgen/repos/usfm/example_usfm/WEB/25-JEReng-web.usfm";
    // const char *path = "./test.usfm";

    printf("Reading file:\n");
    Timer start = {0};
    Timer end = {0};
    getTime(&start);
    // TODO(matt): test if we can get consistent performance from O_DIRECT
    // (minimizes OS caching, check man 2 open)
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Error: `open` failed\n");
        return 1;
    }

    struct stat statbuf = {0};
    if (fstat(fd, &statbuf) == -1) {
        printf("Error: `fstat` failed\n");
        return 1;
    }

    size_t size = (size_t)statbuf.st_size;
    void *filebuf = malloc(size);
    if (filebuf == NULL) {
        printf("Error: `malloc` failed\n");
        return 1;
    }

    if (read(fd, filebuf, size) == -1) {
        printf("Error: `read` failed\n");
        return 1;
    }
    getTime(&end);
    printTimeData(start, end, size);
    
    printf("\nTokenizing file:\n");
    getTime(&start);

    CSFM_TokenResult tokenResult = CSFM_TokenizeAll((uint8_t *)filebuf, size);

    getTime(&end);
    
    // for (uint32_t i = 0; i < tokenResult.tokens.length; i++) {
    //     CSFM_Token token = CSFM_TokenArray_get(tokenResult.tokens, i);
    //     CSFM_Token_print(token, tokenResult.input);
    // }
    uint32_t numTokens = tokenResult.tokens.length;
    printf("\n# tokens: %d\n", numTokens);
    
    printTimeData(start, end, size);

    printf("\nParsing file:\n");
    getTime(&start);

    CSFM_ParseResult parseResult = CSFM_Parse((uint8_t *)filebuf, size);

    getTime(&end);

    // for (uint32_t i = 0; i < parseResult.tree.length; i++) {
    //     CSFM_Node node = CSFM_NodeArray_get(parseResult.tree, i);
    //     CSFM_Node_print(node, parseResult.input);
    // }
    uint32_t numNodes = parseResult.tree.length;
    printf("\n# nodes: %d\n", numNodes);
    printTimeData(start, end, size);

    float tokensPerByte = (float)numTokens / (float)size;
    float nodesPerByte = (float)numNodes / (float)size;
    float nodesPerToken = (float)numNodes / (float)numTokens;
    printf(
        "\n%.4f tokens/byte, %.4f nodes/byte, %.4f nodes/token\n",
        tokensPerByte, nodesPerByte, nodesPerToken
    );

    CSFM_NodeArray_deallocate(&parseResult.tree);
    CSFM_TokenArray_deallocate(&tokenResult.tokens);
    free(filebuf);

    if (close(fd) == -1) {
        printf("Error: `close` failed\n");
        return 1;
    }
    return 0;
}
