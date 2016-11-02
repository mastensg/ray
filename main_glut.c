#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "ray.h"

static int threaded = 1;
static int viewport_width, viewport_height;

static int
init(int argc, char **argv, int w, int h) {
    glutInit(&argc, argv);

    glutInitWindowPosition(0, 0);
    glutInitWindowSize(w, h);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutCreateWindow(argv[0]);

    glDepthMask(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    return 0;
}

static void
display(void) {
    static int count = 0;
    ++count;
    if(count > 10000)
        exit(0);
    float time = (float)glutGet(GLUT_ELAPSED_TIME) / 1000;

    unsigned char* buffer = calloc(viewport_width * viewport_height, 4);
    trace_scene(time, viewport_width, viewport_height, buffer, threaded);
    glClear(GL_COLOR_BUFFER_BIT);
    glRasterPos2d(-1.0, -1.0);
    glDrawPixels(viewport_width, viewport_height, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
    free(buffer);

    glutSwapBuffers();
}

static void
reshape(int w, int h) {
    viewport_width = w;
    viewport_height = h;
    glViewport(0, 0, w, h);
}

static void
keyboard(unsigned char key, int x, int y) {
    switch(key) {
    case 27:
        exit(EXIT_SUCCESS);
        break;

    case 't':
        threaded = !threaded;
        break;
    }
}


int
main(int argc, char **argv) {
    if (init(argc, argv, 800, 600))
        return EXIT_FAILURE;

    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();

    return EXIT_SUCCESS;
}
