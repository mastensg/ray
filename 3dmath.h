#define POW2(x) ((x) * (x))

float dot(const float x[3], const float y[3]);
void normalize(float x[3]);

float sphere_intersect(float* restrict y, float* restrict r,
                       const float* restrict s, const float* restrict d,
                       const float* restrict c, float R, int invert);
