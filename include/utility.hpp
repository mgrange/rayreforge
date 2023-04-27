#ifndef UTILITY_H
#define UTILITY_H

#include <cmath>
#include <limits>
#include <memory>
#include <cstdlib>
#include <random>

// Usings

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

// Constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// Utility Functions

inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

inline double random_double() {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator;
    return distribution(generator);
}

inline double random_double(double min, double max) {
    // Returns a random real in [min,max).
    return min + (max-min)*random_double();
}

inline double clamp(double x, double min, double max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

inline int random_int(int min, int max) {
    // Returns a random integer in [min,max].
    return static_cast<int>(random_double(min, max+1));
}

inline unsigned int offset (const int x, const int y , const int m_height, const int m_width)
{
    return std::min(x, m_width-1) + std::min(y, m_height-1) * m_width;
}

// Common Headers

#include "struct/ray.hpp"
#include "struct/vec3.hpp"
#include "struct/hittable_list.hpp"

// b1, b2, n sont 3 axes orthonormes.
void branchlessONB(const vec3 &n, vec3 &b1, vec3 &b2)
{
    float sign = std::copysign(1.0f, n.z);
    const float a = -1.0f / (sign + n.z);
    const float b = n.x * n.y * a;
    b1 = vec3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
    b2 = vec3(b, sign + n.y * n.y * a, -n.y);
}

/* renvoie la enième directions de fibonacci */
vec3 fibo( const int i, const int nb )
{
    float cos_theta = 1.0 - ( 2.0 * i + 1.0) / ( 2.0 * nb );
    float phi = (sqrt(5.0) + 1.0)/2.0;
    float u = float(rand() % 10000) / 10000.f;
    float phi_theta = 2.0 * pi * (float(i+u)/phi - floor(float(i+u)/phi));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    return vec3(cos(phi_theta) * sin_theta, sin(phi_theta) * sin_theta, cos_theta);
}

/* transforme un vecteur vers le repère monde */
vec3 l2w(const vec3& v, const vec3& n, const vec3& b1, const vec3& b2)
{
    return v.x * b1 + v.y * b2 + v.z * n;
}

#endif