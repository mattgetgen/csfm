#define _DEFAULT_SOURCE 1
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "csfm.h"


int main(void) {
    /*const char *path = "/home/mgetgen/repos/simdusfm/src/usfm/HPUX.usfm";*/
    /*const char *path = "/home/mgetgen/repos/example_usfm/HPUX/01GENHPUX.SFM";*/
    /*const char *path = "/home/mgetgen/repos/example_usfm/WEB/25-JEReng-web.usfm";*/
    /**/const char *path = "./test.usfm";/**/
    struct timespec start_time, end_time;

    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time) == -1) {
        printf("Error: `clock_gettime` failed\n");
        return 1;
    }

    /*
     * TODO(matt): test if we can get consistent performance from O_DIRECT
     * (minimizes OS caching, check man 2 open)
     */
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

    CSFM_TokenArray array = CSFM_Tokenize((char *)filebuf, size);

    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time) == -1) {
        printf("Error: `clock_gettime` failed\n");
        return 1;
    }
    /**/
    size_t i = 0;
    for (i = 0; i < array.length; i++) {
        CSFM_Token token = array.buffer[i];
        switch (token.type) {
        case CSFM_TOKEN_EOF:
            printf("EOF\n");
            break;
        case CSFM_TOKEN_WHITESPACE:
            printf(" ");
            break;
        case CSFM_TOKEN_CARRIAGE_RETURN:
            printf("CR");
            break;
        case CSFM_TOKEN_NEWLINE:
            printf("LF\n");
            break;
        case CSFM_TOKEN_FORWARDSLASH:
            printf("/");
            break;
        case CSFM_TOKEN_BACKSLASH:
            printf("\\");
            break;
        case CSFM_TOKEN_PIPE:
            printf("|");
            break;
        case CSFM_TOKEN_COLON:
            printf(":");
            break;
        case CSFM_TOKEN_SEMICOLON:
            printf(";");
            break;
        case CSFM_TOKEN_TILDE:
            printf("~");
            break;
        case CSFM_TOKEN_ASTERISK:
            printf("*");
            break;
        case CSFM_TOKEN_PLUS:
            printf("+");
            break;
        case CSFM_TOKEN_MINUS:
            printf("-");
            break;
        case CSFM_TOKEN_EQUAL:
            printf("=");
            break;
        case CSFM_TOKEN_DOUBLE_QUOTE:
            printf("\"");
            break;
        case CSFM_TOKEN_NUMBER:
            printf("0");
            break;
        case CSFM_TOKEN_TEXT:
            printf("A");
            break;
        default:
            printf("UNKNOWN");
        }
    }
    /**/

    CSFM_Parse((char *)filebuf, size);

    /*
    long diffInNanoseconds = (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (
        end_time.tv_nsec - start_time.tv_nsec
    );
    double diffInMillisecondsFloat = (double)diffInNanoseconds / (double)1e6;
    double diffInSecondsFloat = (double)diffInNanoseconds / (double)1e9;

    long bytesPerMilli = (long)((double)size / diffInMillisecondsFloat);
    long bytesPerSec = (long)((double)size / diffInSecondsFloat);

    double sizeInKiB = (double)size / 1024;
    double sizeInMiB = sizeInKiB / 1024;
    double sizeInGiB = sizeInMiB / 1024;
    double kibPerSec = sizeInKiB / diffInSecondsFloat;
    double mibPerSec = sizeInMiB / diffInSecondsFloat;
    double gibPerSec = sizeInGiB / diffInSecondsFloat;
    
    printf("size: %ld\n", size);
    printf("took: %f ms\n", diffInMillisecondsFloat);
    printf("took: %f s\n", diffInSecondsFloat);
    printf("B/ms: %ld\n", bytesPerMilli);
    printf("B/s: %ld\n", bytesPerSec);
    printf("\n");
    printf("KiB/s: %f\n", kibPerSec);
    printf("MiB/s: %f\n", mibPerSec);
    printf("GiB/s: %f\n", gibPerSec);
    */

    free(array.buffer);
    free(filebuf);

    if (close(fd) == -1) {
        printf("Error: `close` failed\n");
        return 1;
    }
    return 0;
}
