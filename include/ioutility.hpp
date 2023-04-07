#ifndef IOUTILITY_H
#define IOUTILITY_H

#include <cstdio>
#include <ctype.h>
#include <climits>
#include <memory>

#include <algorithm>

#include "material.hpp"

#include "struct/vec3.hpp"
#include "struct/hittable_list.hpp"
#include "struct/sphere.hpp"
#include "struct/triangle.hpp"
#include "struct/bvh.hpp"

void write_image(std::vector<color> & pixel_list, const int image_width, const int image_height, 
    const int samples_per_pixel){
        
        int bytes_per_pixel = 3;
        unsigned char * data;
        float * datahdr;
        data = new unsigned char[pixel_list.size()*bytes_per_pixel];
        datahdr = new float[pixel_list.size()*bytes_per_pixel];

        int j = 0;
        for(int i = pixel_list.size()-1; i >= 0; --i){
            double r = pixel_list[i].x;
            double g = pixel_list[i].y;
            double b = pixel_list[i].z;

            // Divide the color by the number of samples and gamma-correct for gamma=2.0.
            double scale = 1.0 / samples_per_pixel;
            r = sqrt(scale * r);
            g = sqrt(scale * g);
            b = sqrt(scale * b);

            // Write the translated [0,255] value of each color component.
            data[j*3] = static_cast<unsigned char>(256 * clamp(r, 0.0, 0.999));
            data[j*3+1] = static_cast<unsigned char>(256 * clamp(g, 0.0, 0.999));
            data[j*3+2] = static_cast<unsigned char>(256 * clamp(b, 0.0, 0.999));

            // Write the translated [0,255] value of each color component.
            datahdr[j*3] = float(r);
            datahdr[j*3+1] = float(g);
            datahdr[j*3+2] = float(b);
            j++;
        }

        if(stbi_write_png("result.png", image_width, image_height, bytes_per_pixel, data, 0) == 1){
            std::cerr << "image png generated" << std::endl;
        }
        if(stbi_write_bmp("result.bmp", image_width, image_height, bytes_per_pixel, data) == 1){
            std::cerr << "image bmp generated" << std::endl;
        }
        if(stbi_write_hdr("result.hdr", image_width, image_height, bytes_per_pixel, datahdr) == 1){
            std::cerr << "image hdr generated" << std::endl;
        }

        free(data);
        free(datahdr);
}

hittable_list read_obj( const char *filename)
{
    hittable_list world;
    FILE *in= fopen(filename, "rt");
    if(in == NULL)
    {
        std::cerr << "[error] loading mesh "<< filename << "...\n" << std::endl;
    }
    
    std::cerr << "loading mesh " << filename << "...\n";
    
    std::vector<vec3> positions;
    std::vector<vec3> texcoords;
    std::vector<vec3> normals;
    shared_ptr<material> materials;
    int default_material_id= -1;
    int material_id= -1;
    
    std::vector<int> idp;
    std::vector<int> idt;
    std::vector<int> idn;
    
    char tmp[1024];
    char line_buffer[1024];
    bool error= true;
    for(;;)
    {
        // charge une ligne du fichier
        if(fgets(line_buffer, sizeof(line_buffer), in) == NULL)
        {
            error= false;       // fin du fichier, pas d'erreur detectee
            break;
        }
        
        // force la fin de la ligne, au cas ou
        line_buffer[sizeof(line_buffer) -1]= 0;
        
        // saute les espaces en debut de ligne
        char *line= line_buffer;
        while(*line && isspace(*line))
            line++;
        
        if(line[0] == 'v')
        {
            float x, y, z;
            if(line[1] == ' ')          // position x y z
            {
                if(sscanf(line, "v %f %f %f", &x, &y, &z) != 3)
                    break;
                positions.push_back( vec3(x, y, z) );
            }
            else if(line[1] == 'n')     // normal x y z
            {
                if(sscanf(line, "vn %f %f %f", &x, &y, &z) != 3)
                    break;
                normals.push_back( vec3(x, y, z) );
            }
            else if(line[1] == 't')     // texcoord x y
            {
                if(sscanf(line, "vt %f %f", &x, &y) != 2)
                    break;
                texcoords.push_back( vec3(x, y, 0) );
            }
        }
        
        else if(line[0] == 'f')         // triangle a b c, les sommets sont numerotes a partir de 1 ou de la fin du tableau (< 0)
        {
            idp.clear();
            idt.clear();
            idn.clear();
            
            int next;
            for(line= line +1; ; line= line + next)
            {
                idp.push_back(0); 
                idt.push_back(0); 
                idn.push_back(0);         // 0: invalid index
                
                next= 0;
                if(sscanf(line, " %d/%d/%d %n", &idp.back(), &idt.back(), &idn.back(), &next) == 3) 
                    continue;
                else if(sscanf(line, " %d/%d %n", &idp.back(), &idt.back(), &next) == 2)
                    continue;
                else if(sscanf(line, " %d//%d %n", &idp.back(), &idn.back(), &next) == 2)
                    continue;
                else if(sscanf(line, " %d %n", &idp.back(), &next) == 1)
                    continue;
                else if(next == 0)      // fin de ligne
                    break;
            }

            for(int v= 2; v +1 < (int) idp.size(); v++)
            {
                std::vector<point3> abc;
                std::vector<vec3> normal;
                int idv[3]= { 0, v -1, v };
                for(int i= 0; i < 3; i++)
                {
                    int k= idv[i];
                    int p= (idp[k] < 0) ? (int) positions.size() + idp[k] : idp[k] -1;
                    int t= (idt[k] < 0) ? (int) texcoords.size() + idt[k] : idt[k] -1;
                    int n= (idn[k] < 0) ? (int) normals.size()   + idn[k] : idn[k] -1;
                    
                    if(p < 0) break; // error
                    // if(t >= 0) data.texcoord(texcoords[t]);
                    if(n >= 0) normal.push_back(normals[n]);
                    abc.push_back(positions[p]);
                }
                auto material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
                world.add(make_shared<triangle>(abc[0], abc[1], abc[2], material));
            }
        }
        
        // else if(line[0] == 'm')
        // {
        //    if(sscanf(line, "mtllib %[^\r\n]", tmp) == 1)
        //    {
        //        materials= read_materials( std::string(pathname(filename) + tmp).c_str() );
        //        // enregistre les matieres dans le mesh
        //        data.mesh_materials(materials.data);
        //    }
        // }
        
        // else if(line[0] == 'u')
        // {
        //    if(sscanf(line, "usemtl %[^\r\n]", tmp) == 1)
        //    {
        //        material_id= -1;
        //        for(unsigned int i= 0; i < (unsigned int) materials.names.size(); i++)
        //         if(materials.names[i] == tmp)
        //             material_id= i;
                
        //         if(material_id == -1)
        //         {
        //             // force une matiere par defaut, si necessaire
        //             if(default_material_id == -1)
        //                 default_material_id= data.mesh_material(Material());
                    
        //             material_id= default_material_id;
        //         }
                
        //         // selectionne une matiere pour le prochain triangle
        //         data.material(material_id);
        //    }
        // }
    }
    
    fclose(in);
    
    if(error)
        std::cerr << "loading mesh "<< filename << "[error]" << line_buffer <<"...\n" << std::endl;
    
    return world;
}

void open_cornell(hittable_list & world, camera & cam, double aspect_ratio)
{
     // World
    color background(0,0,0);
    hittable_list objects = read_obj("../data/cornell.obj");

    // adding light
    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    point3 a(-0.24,1.98,0.16);
    point3 b(-0.24,1.98,-0.22);
    point3 c(0.23,1.98,-0.22);
    point3 d(0.23,1.98,0.16);
    objects.add(make_shared<triangle>(a,b,c,difflight));
    objects.add(make_shared<triangle>(a,b,d,difflight));

    // world.add(make_shared<bvh_node>(objects, 0, 1));
    world = objects;

    point3 lookfrom(0,2,7);
    point3 lookat(0,1,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = (lookfrom-lookat).length();
    auto aperture = 2.0;

    cam = camera(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);
}

void open_bigguy(hittable_list & world, camera & cam, double aspect_ratio)
{
     // World
    color background(0,0,0);
    hittable_list objects = read_obj("../data/bigguy.obj");
    
    // adding light
    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    point3 a(10,10,10);
    point3 b(20,10,10);
    point3 c(10,20,10);
    point3 d(10,10,20);
    objects.add(make_shared<triangle>(a,b,c,difflight));
    objects.add(make_shared<triangle>(a,b,d,difflight));

    world.add(make_shared<bvh_node>(objects, 0, 1));

    point3 lookfrom(20,5,50);
    point3 lookat(0,3,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = (lookfrom-lookat).length();
    auto aperture = 2.0;

    cam = camera(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);
}

void final_scene(hittable_list & objects, camera & cam, const int image_width, const double aspect_ratio) {

    int samples_per_pixel = 10;
    color background = color(0,0,0);
    vec3 lookfrom = point3(478, 278, -600);
    vec3 lookat = point3(278, 278, 0);
    double vfov = 40.0;
    vec3 vup(0,1,0);
    auto aperture = 0.0;
    auto dist_to_focus = (lookfrom-lookat).length();

    auto ground = make_shared<lambertian>(color(0.48, 0.83, 0.53));

    auto light = make_shared<diffuse_light>(color(7, 7, 7));

    point3 a(123,127,554);
    point3 b(443,412,554);
    point3 c(123,412,554);
    point3 d(443,127,554);
    objects.add(make_shared<triangle>(a, b, c, light));
    objects.add(make_shared<triangle>(a, b, d, light));


    auto center1 = point3(400, 400, 200);
    auto center2 = center1 + vec3(30,0,0);
    auto moving_sphere_material = make_shared<lambertian>(color(0.7, 0.3, 0.1));

    objects.add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
    objects.add(make_shared<sphere>(
        point3(0, 150, 145), 50, make_shared<metal>(color(0.8, 0.8, 0.9), 1.0)
    ));

    auto boundary = make_shared<sphere>(point3(360,150,145), 70, make_shared<dielectric>(1.5));
    objects.add(boundary);
    boundary = make_shared<sphere>(point3(0, 0, 0), 5000, make_shared<dielectric>(1.5));

    auto emat = make_shared<lambertian>(make_shared<image_texture>("../data/earthmap.jpg"));
    objects.add(make_shared<sphere>(point3(400,200,400), 100, emat));
    auto pertext = make_shared<noise_texture>(0.1);
    objects.add(make_shared<sphere>(point3(220,280,300), 80, make_shared<lambertian>(pertext)));

    hittable_list boxes2;
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    int ns = 1000;
    for (int j = 0; j < ns; j++) {
        boxes2.add(make_shared<sphere>(point3::random(0,165), 10, white));
    }

    objects.add(make_shared<bvh_node>(boxes2, 0.0, 1.0));

    cam = camera(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);

}

#endif