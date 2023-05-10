#ifndef IOUTILITY_H
#define IOUTILITY_H

#include <cstdio>
#include <ctype.h>
#include <climits>
#include <memory>
#include <map>

#include <algorithm>

#include "material.hpp"
#include "camera.hpp"

#include "struct/vec3.hpp"
#include "struct/hittable_list.hpp"
#include "struct/sphere.hpp"
#include "struct/triangle.hpp"
#include "struct/bvh.hpp"

std::string pathname( const std::string& filename )
{
    std::string path= filename;
#ifndef WIN32
    std::replace(path.begin(), path.end(), '\\', '/');   // linux, macos : remplace les \ par /.
    size_t slash = path.find_last_of( '/' );
    if(slash != std::string::npos)
        return path.substr(0, slash +1); // inclus le slash
    else
        return "./";
#else
    std::replace(path.begin(), path.end(), '/', '\\');   // windows : remplace les / par \.
    size_t slash = path.find_last_of( '\\' );
    if(slash != std::string::npos)
        return path.substr(0, slash +1); // inclus le slash
    else
        return ".\\";
#endif
}

void write_image(std::vector<color> & pixel_list, const int image_width, const int image_height, 
    const int samples_per_pixel){
        
        int bytes_per_pixel = 3;
        unsigned char * data;
        float * datahdr;
        data = new unsigned char[pixel_list.size()*bytes_per_pixel];
        datahdr = new float[pixel_list.size()*bytes_per_pixel];

        for (int j = image_height-1; j >= 0; --j) {
            for (int i = image_width-1; i >= 0; --i) {
                double r = pixel_list[offset(i,j,image_height,image_width)].x;
                double g = pixel_list[offset(i,j,image_height,image_width)].y;
                double b = pixel_list[offset(i,j,image_height,image_width)].z;

                // Divide the color by the number of samples and gamma-correct for gamma=2.0.
                double scale = 1.0 / samples_per_pixel;
                r = sqrt(scale * r);
                g = sqrt(scale * g);
                b = sqrt(scale * b);
                
                // Write the translated [0,255] value of each color component.
                data[(i + (image_height-j-1) * image_width)*3] = static_cast<unsigned char>(256 * clamp(r, 0.0, 0.999));
                data[(i + (image_height-j-1) * image_width)*3+1] = static_cast<unsigned char>(256 * clamp(g, 0.0, 0.999));
                data[(i + (image_height-j-1) * image_width)*3+2] = static_cast<unsigned char>(256 * clamp(b, 0.0, 0.999));

                // Write the translated [0,255] value of each color component.
                datahdr[(i + (image_height-j-1) * image_width)*3] = float(r);
                datahdr[(i + (image_height-j-1) * image_width)*3+1] = float(g);
                datahdr[(i + (image_height-j-1) * image_width)*3+2] = float(b);
            }
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

std::map<std::string,shared_ptr<material>> read_materials( const char *filename )
{
    std::map<std::string,shared_ptr<material>> materials;
    std::vector<std::string> namematerial;
    std::vector<color> colormaterial;
    
    FILE *in= fopen(filename, "rt");
    if(in == NULL)
    {
        printf("[error] loading materials '%s'...\n", filename);
        materials.insert(std::pair<std::string, shared_ptr<material>>("white",make_shared<lambertian>(color(0.5,0.5,0.5))));
        return materials;
    }
    
    printf("loading materials '%s'...\n", filename);
    
    char tmp[1024];
    char line_buffer[1024];
    bool error= true;
    bool isColor = false;
    bool isMaterialLight = false;
    
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
        
        if(line[0] == 'n')
        {
            // add new material name
            if(sscanf(line, "newmtl %[^\r\n]", tmp) == 1)
                namematerial.push_back(tmp);
        }
        
        // if(material == NULL)
        //     continue;

        if(line[0] == 'K')
        {
            float r, g, b;
            // add new color material
            if(sscanf(line, "Kd %f %f %f", &r, &g, &b) == 3)
                colormaterial.push_back(color(r,g,b));
            else if(sscanf(line, "Ks %f %f %f", &r, &g, &b) == 3)
                colormaterial.push_back(color(r,g,b));
            else if(sscanf(line, "Ke %f %f %f", &r, &g, &b) == 3)
                colormaterial.push_back(color(r,g,b));
        }
        
        // else if(line[0] == 'N')
        // {
        //     float n;
        //     if(sscanf(line, "Ns %f", &n) == 1)          // Ns, puissance / concentration du reflet, modele blinn phong
        //         material->ns= n;
        // }
    }
    for(int i = 0; i < namematerial.size(); ++i){
        if(colormaterial[i*3+2].x != 0 || colormaterial[i*3+2].y != 0 || colormaterial[i*3+2].z != 0){
            materials.insert(std::pair<std::string, shared_ptr<material>>(namematerial[i],make_shared<diffuse_light>(colormaterial[i*3+2])));
        } else {
            materials.insert(std::pair<std::string, shared_ptr<material>>(namematerial[i],make_shared<lambertian>(colormaterial[i*3])));
        }
    }
    
    fclose(in);
    if(error)
        printf("[error] parsing line :\n%s\n", line_buffer);
    
    return materials;
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
    std::map<std::string,shared_ptr<material>> materials;
    std::string materialName;

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
                world.add(make_shared<triangle>(abc[0], abc[1], abc[2], materials[materialName]));
            }
        }
        
        else if(line[0] == 'm')
        {
           if(sscanf(line, "mtllib %[^\r\n]", tmp) == 1)
           {
               materials= read_materials( std::string(pathname(filename) + tmp).c_str() );
           }
        }
        
        else if(line[0] == 'u')
        {
           if(sscanf(line, "usemtl %[^\r\n]", tmp) == 1)
           {
                materialName = tmp;
           }
        }
    }
    
    fclose(in);
    
    if(error)
        std::cerr << "loading mesh "<< filename << "[error]" << line_buffer <<"...\n" << std::endl;
    
    std::cerr << world.objects.size() << " element load" << std::endl;
    return world;
}

void open_cornell(hittable_list & mesh, camera & cam, double aspect_ratio)
{
     // World
    color background(0,0,0);
    mesh = read_obj("../data/cornell.obj");

    point3 lookfrom(0,1,3.5);
    point3 lookat(0,1,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = (lookfrom-lookat).length();
    auto aperture = 2.0;

    cam = camera(lookfrom, lookat, vup, 45, aspect_ratio, aperture, dist_to_focus);
}

void open_sponza(hittable_list & world, camera & cam, double aspect_ratio)
{
    color background(0.1,0.1,0.1);
    hittable_list objects = read_obj("../data/sponza.obj");

    //add light outside the bvh
    for (int i = 0; i < objects.objects.size(); ++i){
        if(objects.objects[i]->have_material_light()){
            world.add(objects.objects[i]);
        }
    }

    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    world.add(make_shared<sphere>(point3(0, 25, 0), 5, difflight));

    world.add(make_shared<bvh_node>(objects, 0, 1));
    std::cerr << std::endl;

    point3 lookfrom(15,2,0);
    point3 lookat(0,2,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = (lookfrom-lookat).length();
    auto aperture = 2.0;

    cam = camera(lookfrom, lookat, vup, 45, aspect_ratio, aperture, dist_to_focus);
}

void open_spaceship(hittable_list & world, camera & cam, double aspect_ratio)
{
     // Worldcornell
    color background(0,0,0);
    // hittable_list objects;
    // hittable_list objects = read_obj("../data/starcruiser_military/Starcruiser_military.obj");
    world = read_obj("../data/ufo_plane_free.obj");

    // add earth
    auto emat = make_shared<lambertian>(make_shared<image_texture>("../data/earthmap.jpg"));
    world.add(make_shared<sphere>(point3(-100, -45, -61), 100, emat));

    // add light
    auto difflight = make_shared<diffuse_light>(color(4,7,7));
    world.add(make_shared<sphere>(point3(71, 11, 227), 50, difflight));

    // add mini light (star)
    int ns = 500;
    for (int j = 0; j < ns; j++) {
        point3 alea(point3::random(-500,500));
        alea.z = 300;
        auto random_color = make_shared<lambertian>(color(random_double(0,1),random_double(0,1),random_double(0,1)));
        world.add(make_shared<sphere>(alea, random_double(0.1,2), random_color));
    }

    point3 lookfrom(71,11,-227);
    point3 lookat(0,0,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = (lookfrom-lookat).length();
    auto aperture = 2.0;

    cam = camera(lookfrom, lookat, vup, 45, aspect_ratio, aperture, dist_to_focus);
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
    objects.add(make_shared<triangle>(a,d,c,difflight));
    objects.add(make_shared<triangle>(a,b,d,difflight));

    //create BVH 
    world.add(make_shared<bvh_node>(objects, 0, 1));
    std::cerr << std::endl;

    point3 lookfrom(20,5,50);
    point3 lookat(0,3,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = (lookfrom-lookat).length();
    auto aperture = 2.0;

    cam = camera(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);
}

void open_test(hittable_list & world, camera & cam, double aspect_ratio)
{
     // World
    color background(0,0,0);
    hittable_list objects = read_obj("../data/test.obj");

    //add light outside the bvh
    for (int i = 0; i < objects.objects.size(); ++i){
        if(objects.objects[i]->have_material_light()){
            world.add(objects.objects[i]);
        }
    }

    // adding light
    auto mirroir = make_shared<metal>(color(1,1,1),1);
    point3 a(-20,20,-20);
    point3 b(-20,-10,-20);
    point3 c(-10,20,20);
    point3 d(-10,-10,20);
    objects.add(make_shared<triangle>(a,d,c,mirroir));
    objects.add(make_shared<triangle>(a,b,d,mirroir));

    //create BVH 
    world.add(make_shared<bvh_node>(objects, 0, 1));

    point3 lookfrom(13,13,23);
    point3 lookat(0,5,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = (lookfrom-lookat).length();
    auto aperture = 2.0;

    cam = camera(lookfrom, lookat, vup, 70, aspect_ratio, aperture, dist_to_focus);
}

void open_sportCar(hittable_list & world, camera & cam, double aspect_ratio)
{
     // World
    color background(0.8,0.8,0.8);
    hittable_list objects = read_obj("../data/sportsCar.obj");

    //add light outside the bvh
    for (int i = 0; i < objects.objects.size(); ++i){
        if(objects.objects[i]->have_material_light()){
            world.add(objects.objects[i]);
        }
    }

    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    point3 a(-10,2,-10);
    point3 b(-10,2,10);
    point3 c(10,2,-10);
    point3 d(10,2,10);
    world.add(make_shared<triangle>(a,d,c,difflight));
    world.add(make_shared<triangle>(a,b,d,difflight));

    auto wall = make_shared<lambertian>(color(0.8,0.8,0.8));
    c = color(-10,0,-10);
    d = color(-10,0,10);

    objects.add(make_shared<triangle>(a,d,c,wall));
    objects.add(make_shared<triangle>(a,b,d,wall));

    //create BVH 
    world.add(make_shared<bvh_node>(objects, 0, 1));

    point3 lookfrom(50,1.8,50);
    point3 lookat(5,0.5,5);
    vec3 vup(0,1,0);
    auto dist_to_focus = (lookfrom-lookat).length();
    auto aperture = 2.0;

    cam = camera(lookfrom, lookat, vup, 45, aspect_ratio, aperture, dist_to_focus);
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