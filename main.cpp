
#include "include/utility.hpp"
#include "include/camera.hpp"
#include "include/material.hpp"
#include "include/color.hpp"
#include "include/ioutility.hpp"
#include "include/struct/bvh.hpp"

#include <iostream>
#include <SDL2/SDL.h>

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

color indirect_ray_color(ray& r, color& background, hittable& world, int depth, const int & sample, const int & all_samples) {
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

    vec3 b1, b2;
    branchlessONB(scattered.dir, b1, b2);
    vec3 w = l2w(fibo(sample, all_samples), scattered.dir, b1, b2);

    ray fiboray(scattered.orig, w);

    return emitted + attenuation * ray_color(fiboray, background, world, depth-1);
}

color direct_ray_color(ray& r, color& background, hittable& world, std::vector<shared_ptr <hittable> >& light, int depth, const int & sample, const int & all_samples) {
    hit_record rec;

    // If the ray hits nothing, return the background color.
    if (!world.hit(r, 0.001, infinity, rec))
        return background;

    // select random light in scene
    int random_light_id = random_double(0, light.size());
    auto random_light = light[random_light_id];

    // select random point on light source
    float u = sqrt(random_double());
    float v = ( 1.0f - u ) * sqrt(random_double());
    point3 light_point = random_light->point(u,v);

    // create ray between light point and hit point
    ray direct_light_ray;
    direct_light_ray.dir = rec.p - light_point;
    direct_light_ray.orig = light_point;

    hit_record rec_light;
    // If the ray hits nothing, there is nothing between light and element, return material color.
    if (!world.hit(direct_light_ray, 0.001, 0.999, rec_light)){
        color attenuation;
        color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
        ray scatter;
        if (!rec.mat_ptr->scatter(r, rec, attenuation, scatter))
            return emitted;
        // if material is pure color return attenuation
        if(rec.mat_ptr->isMatMaterial())
            return attenuation;
        else
            return indirect_ray_color(r, background, world, depth, sample, all_samples);
    }

    return background;
}

int main( int argc, char **argv ) {

    bool PREVIEW = false;
    SDL_Window *window;
    SDL_Surface *window_surface;

    if(argc > 1){
        std::string value = argv[1];
        if(value == "--preview"){
            PREVIEW = true;
        }
    }

    if(PREVIEW){
        if(SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cout << "Failed to initialize the SDL2 library\n";
            return -1;
        }
    }

    std::cerr << "attention y et z inversé par rapport à blender" << std::endl;
    // Image
    double aspect_ratio = 16.0 / 9.0;
    int image_width = 800;
    int image_height = static_cast<int>(image_width / aspect_ratio);
    int samples_per_pixel = 1000;
    int max_depth = 50;

    // World
    color background(0,0,0);
    hittable_list mesh;
    hittable_list world;
    std::vector<shared_ptr <hittable> > light;
    camera cam;

    open_test(mesh, cam, aspect_ratio);
    // open_cornell(mesh, cam, aspect_ratio);
    // open_sportCar(mesh, cam, aspect_ratio);
    // open_sponza(mesh, cam, aspect_ratio);
    // open_spaceship(mesh, cam, aspect_ratio);
    // open_bigguy(mesh, cam, aspect_ratio);
    // final_scene(mesh, cam, image_width, aspect_ratio);

    // create BVH
    world.add(make_shared<bvh_node>(mesh, 0, 1));
    std::cerr << std::endl;

    world.add(make_shared<sphere>(point3(0,3.5,0),1,make_shared<dielectric>(1.5)));
    world.add(make_shared<sphere>(point3(-3,3.5,-1.5),1,make_shared<metal>(color(0.8,0.8,0.8),1)));

    // add light 
    for (int i = 0; i < mesh.objects.size(); ++i){
        if(mesh.objects[i]->have_material_light()){
            light.push_back(mesh.objects[i]);
            world.add(mesh.objects[i]);
        }
    }
    std::cerr << "light : " << light.size() << std::endl;

    if(PREVIEW){
        window = SDL_CreateWindow("render",
                                            SDL_WINDOWPOS_CENTERED,
                                            SDL_WINDOWPOS_CENTERED,
                                            image_width, image_height,
                                            0);

        if(!window)
        {
            std::cout << "Failed to create window\n";
            return -1;
        }

        window_surface = SDL_GetWindowSurface(window);

        if(!window_surface)
        {
            std::cout << "Failed to get the surface from the window\n";
            return -1;
        }
    }

    // Render
    std::vector<color> pixel_list;
    pixel_list.resize(image_width*image_height);

    for (int s = 0; s < samples_per_pixel; ++s) {
        std::cerr << "\rScanlines remaining : " << int((float(s)/float(samples_per_pixel))*100) << " %" << std::flush;
        #pragma omp parallel for schedule(dynamic, 16)
        for (int j = image_height-1; j >= 0; --j) {
            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0, 0, 0);
                auto u = (i + random_double()) / (image_width-1);
                auto v = (j + random_double()) / (image_height-1);
                ray r = cam.get_ray(u, v);
                pixel_color = (indirect_ray_color(r, background, world, max_depth, s, samples_per_pixel)*0.5
                            + direct_ray_color(r, background, world, light, max_depth, s, samples_per_pixel)*0.5);
                pixel_list[offset(i,j,image_height,image_width)] += pixel_color;

                if(PREVIEW){
                    // temporary render windows
                    SDL_Rect rect;
                    rect.x = i ;
                    rect.y = image_height - j ;
                    rect.h = rect.w = 1;

                    double scale = 1.0 / s;
                    Uint8 red,green,blue;
                    red = static_cast<unsigned char>(256 * clamp(sqrt(scale * pixel_list[offset(i,j,image_height,image_width)].x), 0.0, 0.999));
                    green = static_cast<unsigned char>(256 * clamp(sqrt(scale * pixel_list[offset(i,j,image_height,image_width)].y), 0.0, 0.999));
                    blue = static_cast<unsigned char>(256 * clamp(sqrt(scale * pixel_list[offset(i,j,image_height,image_width)].z), 0.0, 0.999));

                    Uint32 uintcolor = SDL_MapRGB(window_surface->format, red, green, blue);
                    SDL_FillRect (window_surface, &rect, uintcolor);
                }
            }
        }
        if(PREVIEW)
            SDL_UpdateWindowSurface(window);
    }

    std::cerr << std::endl;
    // write image in format png, bmp and hdr
    write_image(pixel_list, image_width, image_height, samples_per_pixel);
    std::cerr << "Done\n";
}