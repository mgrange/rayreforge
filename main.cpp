
#include "include/utility.hpp"
#include "include/camera.hpp"
#include "include/material.hpp"
#include "include/color.hpp"
#include "include/ioutility.hpp"
#include "include/struct/bvh.hpp"

#include <iostream>

color ray_color(ray& r, color& background, hittable& world, int depth) {
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return color(0,0,0);

    // If the ray hits nothing, return the background color.
    if (!world.hit(r, 0.001, infinity, rec))
        return background;

    ray scattered;
    color attenuation;
    color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);

    if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        return emitted;

    return emitted + attenuation * ray_color(scattered, background, world, depth-1);
}

int main() {

    // Image
    double aspect_ratio = 16.0 / 9.0;
    int image_width = 400;
    int image_height = static_cast<int>(image_width / aspect_ratio);
    int samples_per_pixel = 1000;
    int max_depth = 5;

    // World
    color background(0,0,0);
    hittable_list world;
    camera cam;

    // open_test(world, cam, aspect_ratio);
    open_cornell(world, cam, aspect_ratio);
    // open_sportCar(world, cam, aspect_ratio);
    // open_sponza(world, cam, aspect_ratio);
    // open_bigguy(world, cam, aspect_ratio);
    // final_scene(world, cam, image_width, aspect_ratio);

    // Render
    std::vector<color> pixel_list;
    pixel_list.resize(image_width*image_height);

    #pragma omp parallel for schedule(dynamic, 16)
    for (int j = image_height-1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining : " << j << " " << std::flush;
        for (int i = 0; i < image_width; ++i) {
            color pixel_color(0, 0, 0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width-1);
                auto v = (j + random_double()) / (image_height-1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, background, world, max_depth);
            }
            pixel_list[offset(i,j,image_height,image_width)] = pixel_color;
        }
    }
    std::cerr << std::endl;
    // write image in format png, bmp and hdr
    write_image(pixel_list, image_width, image_height, samples_per_pixel);
    std::cerr << "Done\n";
}