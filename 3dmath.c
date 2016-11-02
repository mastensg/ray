#include "3dmath.h"

#include <math.h>

float
dot(const float x[3], const float y[3]) {
    return x[0] * y[0] + x[1] * y[1] + x[2] * y[2];
}

void
normalize(float x[3]) {
    float len = 1.0f / sqrtf(dot(x, x));

    x[0] *= len;
    x[1] *= len;
    x[2] *= len;
}

float
sphere_intersect(float* restrict y, float* restrict r,
                 const float* restrict s, const float* restrict d,
                 const float* restrict c, float R, int invert) {
    int i;
    float D, n[3], t, v[3];

    for(i = 0; i < 3; ++i)
        v[i] = s[i] - c[i];

    D = POW2(dot(v, d)) - (dot(v, v) - POW2(R));

    if(D < 0)
        return -1;

    if (invert)
      t = -dot(v, d) + D;
    else
      t = -dot(v, d) - D;

    if (t <= 0)
        return -1;

    for(i = 0; i < 3; ++i) {
        y[i] = s[i] + t * d[i];
        n[i] = y[i] - c[i];
    }

    float two_dot_nd_div_sq_n_mag = 2.0f * dot(n, d) / dot(n, n);

    for (i = 0; i < 3; ++i)
      r[i] = d[i] - two_dot_nd_div_sq_n_mag * n[i];

    return t;
}
