AM_CFLAGS = -Wall -pedantic -std=c99 -flto

bin_PROGRAMS = ray ray-headless


ray_SOURCES = main_glut.c ray.c 3dmath.c 3dmath.h
ray_LDADD = -lGL -lGLU -lglut -lm
ray_LDFLAGS = -pthread

ray_headless_SOURCES = main_headless.c ray.c 3dmath.c 3dmath.h
ray_headless_LDADD = -lm
ray_headless_LDFLAGS = -pthread
