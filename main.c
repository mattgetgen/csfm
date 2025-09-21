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
    const char *path = "/home/mgetgen/repos/example_usfm/HPUX/01GENHPUX.SFM";
    struct timespec start_time, end_time;

    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time) == -1) {
        printf("Error: `clock_gettime` failed\n");
        return 1;
    }

    /* TODO(matt): test if we can get consistent performance from O_DIRECT
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

    CSFMTokenize((const char *)filebuf, size);

    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time) == -1) {
        printf("Error: `clock_gettime` failed\n");
        return 1;
    }

    long diffInNanoseconds = (end_time.tv_sec - start_time.tv_sec) * (long)1e9 + (
        end_time.tv_nsec - start_time.tv_nsec
    );
    double diffInMillisecondsFloat = (double)diffInNanoseconds / (double)1e6;
    double diffInSecondsFloat = (double)diffInNanoseconds / (double)1e9;
    printf("took: %f ms\n", diffInMillisecondsFloat);
    printf("took: %f s\n", diffInSecondsFloat);

    long bytesPerSec = (long)((double)size / diffInSecondsFloat);
    
    /* long bytesPerMs = (long)size / diffInMilliseconds; */
    printf("bytes/s: %ld\n", bytesPerSec);


    free(filebuf);

    if (close(fd) == -1) {
        printf("Error: `close` failed\n");
        return 1;
    }
    return 0;
}
