#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.hpp"
#include "vec3.hpp"

class triangle : public hittable {
    public:
        triangle() {}
        triangle(point3 _a, point3 _b, point3 _c, shared_ptr<material> m)
            : a(_a), b(_b), c(_c), mat_ptr(m) {};

        virtual bool hit(
            const ray& r, double t_min, double t_max, hit_record& rec) const override;

    public:
        point3 a,b,c;
        shared_ptr<material> mat_ptr;
};

bool triangle::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {

    const double EPSILON = 0.0000001;

    /* begin calculating determinant - also used to calculate U parameter */
    vec3 ac= c-a;
    vec3 pvec= cross(r.direction(), ac);

    /* if determinant is near zero, ray lies in plane of triangle */
    vec3 ab= b-a;
    float det= dot(ab, pvec);
    if(det > -EPSILON && det < EPSILON)
        return false;

    float inv_det= 1.0f / det;

    /* calculate distance from vert0 to ray origin */
    vec3 tvec = r.origin() - a;

    /* calculate U parameter and test bounds */
    float u= dot(tvec, pvec) * inv_det;
    if(u < 0.0f || u > 1.0f)
        return false;

    /* prepare to test V parameter */
    vec3 qvec= cross(tvec, ab);

    /* calculate V parameter and test bounds */
    float v= dot(r.direction(), qvec) * inv_det;
    if(v < 0.0f || u + v > 1.0f)
        return false;

    /* calculate t, ray intersects triangle */
    double t= dot(ac, qvec) * inv_det;

    // ne renvoie vrai que si l'intersection est valide (comprise entre tmin et tmax du rayon)
    if (t <= t_max && t > EPSILON){
        if(t < rec.t){
            rec.t = t;
            rec.p = r.origin() + r.direction() * t;
            rec.normal = cross(ab,ac);
            rec.set_face_normal(r, rec.normal);
            rec.mat_ptr = mat_ptr;
            return true;
        }
    }
    return false;
}

#endif
