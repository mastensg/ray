#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include "ray.h"

static const size_t kFramesToRender = 100;

int
main(int argc, char** argv) {
    fprintf(stderr, "Rendering %zu frames\n", kFramesToRender);

    unsigned char* buffer = calloc(4, WIDTH * HEIGHT);

    struct timeval start;
    gettimeofday(&start, NULL);

    for (size_t i = 0; i < kFramesToRender; ++i)
        trace_scene(i * 0.01f, buffer, 0);

    struct timeval end;
    gettimeofday(&end, NULL);

    free(buffer);

    fprintf(stderr, "Average %.2f ms/frame\n",
            (1.0e3 * (end.tv_sec - start.tv_sec) +
             1.0e-3 * (end.tv_usec - start.tv_usec)) /
                kFramesToRender);

    return EXIT_SUCCESS;
}
