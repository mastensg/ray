#include "ray.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>

#include "3dmath.h"

#define LENGTH(array) (sizeof(array) / sizeof(array[0]))
#define MAX(x, y) (x > y ? x : y)
#define MIN(x, y) (x < y ? x : y)

#define TAU 6.28318531

#if __GNUC__ >= 3
#  define unlikely(cond) __builtin_expect ((cond), 0)
#  define likely(cond)   __builtin_expect ((cond), 1)
#else
#  define unlikely(cond) (cond)
#  define likely(cond) (cond)
#endif

typedef struct {
    float position[3];
    float radius;
    float diffuse[3];
    float specular[3];
    int subtract;
} Object;

typedef struct {
    float position[3];
    float diffuse[3];
} Light;

typedef struct {
    pthread_mutex_t mutex;

    int width, height;
    unsigned char* buffer;
    long next_line;
} ThreadArg;

static float* trace_vectors;
static int trace_vectors_width, trace_vectors_height;

static Object objects[] = {
    {.position={-1.414, -1, -3}, .radius=1, .diffuse={.8, .0, .8}, .specular={.7, .6, .7}, .subtract=0},
    {.position={0, 1.414, -3}, .radius=1, .diffuse={.0, .8, .8}, .specular={.6, .7, .7}, .subtract=0},
    {.position={0, 0, -3}, .radius=1.5, .diffuse={.8, .8, .8}, .specular={.7, .7, .7}, .subtract=1},
    {.position={1.414, -1, -3}, .radius=1, .diffuse={.8, .8, .0}, .specular={.7, .7, .6}, .subtract=0},
    {.position={0, 0, -3}, .radius=1.1, .diffuse={.9, .9, .9}, .specular={.9, .9, .9}, .subtract=2}
};
static const Light lights[] = {
    {.position={-3, 3, -4}, .diffuse={0, .6, .6}},
    {.position={0, 30, -4}, .diffuse={1, 1, 1}}
};
static const float ambient[3] = {0.2, 0.1, 0.1};

static void
trace(const float s[3], const float d[3], float pixel[3], int n) {
    // Reflections in concave objects can go really deep, so we need to limit
    // the recursion depth.
    if (n > 6) return;

    float nearest = HUGE_VAL;
    int nearest_object = -1;
    float nearest_y[3];
    float nearest_r[3];

    for(size_t j = 0; j < LENGTH(objects); ++j) {
        float r[3], t, y[3];

        if (objects[j].subtract == 1) continue;

        t = sphere_intersect(y, r, s, d, objects[j].position, objects[j].radius, 0);

        if(likely(t <= 0) || t > nearest)
          continue;

        if (objects[j].subtract == 0) {
          size_t k;
          for (k = 0; k < LENGTH(objects); ++k) {
              if (!objects[k].subtract) continue;
              if (POW2(y[0] - objects[k].position[0]) + POW2(y[1] - objects[k].position[1]) + POW2(y[2] - objects[k].position[2]) > POW2(objects[k].radius)) continue;

              t = sphere_intersect(y, r, s, d, objects[k].position, objects[k].radius, 1);

              break;
          }

          if(likely(t <= 0) || t > nearest)
            continue;
        }

        nearest = t;
        nearest_object = j;
        memcpy(nearest_y, y, sizeof(nearest_y));
        memcpy(nearest_r, r, sizeof(nearest_y));
    }

    if (nearest_object == -1) return;

    trace(nearest_y, nearest_r, pixel, n + 1);

    for (int k = 0; k < 3; ++k)
        pixel[k] = pixel[k] * objects[nearest_object].specular[k] + ambient[k] * objects[nearest_object].diffuse[k];

    for(int m = 0; m < LENGTH(lights); ++m) {
        float l[3];
        for(int i = 0; i < 3; ++i)
            l[i] = lights[m].position[i] - nearest_y[i];

        float lr_dot = dot(l, nearest_r);
        if (lr_dot <= 0) continue;

        float scale = lr_dot / sqrtf(dot(l, l)) / (1 << n);
        // The cutoff at 0.05 is for artistic reasons; 0.0 would be more
        // realistic.
        if (scale <= 0.05) continue;

        for(int k = 0; k < 3; ++k)
            pixel[k] += lights[m].diffuse[k] * objects[nearest_object].diffuse[k] * scale;
    }
}

static void
trace_line(int l, int width, unsigned char *buf) {
    static const float s[3] = {0, 0, 8};

    for(int i = 0; i < width; ++i, buf += 4) {
        float pixel[3] = { 0, 0, 0 };

        trace(s, &trace_vectors[(l * width + i) * 3], pixel, 1);

        buf[0] = MIN(pixel[0], 1.0f) * 255;
        buf[1] = MIN(pixel[1], 1.0f) * 255;
        buf[2] = MIN(pixel[2], 1.0f) * 255;
    }
}

static void *
thread(void *arg) {
    ThreadArg* thread_arg = arg;

    for (;;) {
        pthread_mutex_lock(&thread_arg->mutex);
        if (thread_arg->next_line == thread_arg->height) break;
        long line = thread_arg->next_line++;
        pthread_mutex_unlock(&thread_arg->mutex);

        trace_line(line, thread_arg->width, thread_arg->buffer + line * 4 * thread_arg->width);
    }

    pthread_mutex_unlock(&thread_arg->mutex);

    return NULL;
}

static void
initialize_trace_vectors(int width, int height) {
    trace_vectors = calloc(width * height, 3 * sizeof(float));
    trace_vectors_width = width;
    trace_vectors_height = height;
    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
          float* d = &trace_vectors[(y * width + x) * 3];
          d[0] = ((float)x / width - 0.5f) * 0.5f * ((float)width / height);
          d[1] = ((float)y / height - 0.5f) * 0.5f;
          d[2] = -1;
          normalize(d);
        }
    }
}

void
trace_scene(float time, int width, int height, unsigned char *buf, int threaded) {
    if (trace_vectors && (trace_vectors_width != width || trace_vectors_height != height)) {
      free(trace_vectors);
      trace_vectors = 0;
    }
    if (!trace_vectors)
      initialize_trace_vectors(width, height);

    objects[0].position[0] = (1.5 + 0.35 * sin(1.1 * time + 0.0)) * cos(0.5 * time);
    objects[0].position[1] = (1.5 + 0.35 * sin(1.1 * time + 2.5)) * sin(0.5 * time);
    objects[1].position[0] = (1.5 + 0.35 * sin(1.1 * time + 2.0)) * cos(0.5 * time + 1/3. * TAU);
    objects[1].position[1] = (1.5 + 0.35 * sin(1.1 * time + 1.5)) * sin(0.5 * time + 1/3. * TAU);
    objects[3].position[0] = (1.5 + 0.35 * sin(1.1 * time + 1.0)) * cos(0.5 * time + 2/3. * TAU);
    objects[3].position[1] = (1.5 + 0.35 * sin(1.1 * time + 0.5)) * sin(0.5 * time + 2/3. * TAU);
    objects[2].position[2] = -3 + 0.2 * sin(time * 1.2);
    memcpy(objects[4].position, objects[2].position, sizeof(objects[4].position));

    if(threaded) {
        ThreadArg arg;
        memset(&arg, 0, sizeof(arg));
        arg.width = width;
        arg.height = height;
        pthread_mutex_init(&arg.mutex, NULL);
        arg.buffer = buf;

        int num_threads = sysconf(_SC_NPROCESSORS_CONF) - 1;
        pthread_t* threads = NULL;
        if (num_threads > 0) {
          threads = calloc(sizeof(*threads), num_threads);

          for (int i = 0; i < num_threads; ++i)
            pthread_create(&threads[i], NULL, thread, &arg);
        }

        thread(&arg);

        for(int i = 0; i < num_threads; ++i)
            pthread_join(threads[i], NULL);
        free(threads);
    } else {
        for(int i = 0; i < height; ++i)
            trace_line(i, width, buf + i * 4 * width);
    }
}
