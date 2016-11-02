#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ray.h"

static const size_t kFramesToRender = 126;
static const size_t kWidth = 512;
static const size_t kHeight = 512;

void save_ppm(const unsigned char *src, size_t w, size_t h, const char *fmt,
              ...) {
  const size_t kPathMax = 1024;
  char path[kPathMax];
  va_list ap;
  size_t y, x;
  FILE *f;

  va_start(ap, fmt);
  vsnprintf(path, kPathMax, fmt, ap);
  va_end(ap);

  f = fopen(path, "w");

  fprintf(f, "P3\n%zu %zu\n%u\n", w, h, 255);

  for (y = 0; y < h; ++y) {
    for (x = 0; x < w; ++x) {
      // R G B
      fprintf(f, "%4u%4u%4u", src[4 * w * y + 4 * x + 0],
              src[4 * w * y + 4 * x + 1], src[4 * w * y + 4 * x + 2]);
    }
    fprintf(f, "\n");
  }

  fclose(f);
}

int main() {
  fprintf(stderr, "Rendering %zu frames\n", kFramesToRender);

  unsigned char *buffer = calloc(4, kWidth * kHeight);

  for (size_t i = 0; i < kFramesToRender; ++i) {
    trace_scene(i * 0.1f, kWidth, kHeight, buffer, 1);
    save_ppm(buffer, kWidth, kHeight, "out_%06u.ppm", i);
    fprintf(stderr, "  %zu\n", i);
  }

  return EXIT_SUCCESS;
}
