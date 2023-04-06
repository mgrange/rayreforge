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

// hittable_list random_scene() {
//     hittable_list world;

//     auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
//     world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(checker)));

//     auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
//     world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

//     for (int a = -11; a < 11; a++) {
//         for (int b = -11; b < 11; b++) {
//             auto choose_mat = random_double();
//             point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

//             if ((center - point3(4, 0.2, 0)).length() > 0.9) {
//                 shared_ptr<material> sphere_material;

//                 if (choose_mat < 0.8) {
//                     // diffuse
//                     auto albedo = color::random() * color::random();
//                     sphere_material = make_shared<lambertian>(albedo);
//                     world.add(make_shared<sphere>(center, 0.2, sphere_material));
//                 } else if (choose_mat < 0.95) {
//                     // metal
//                     auto albedo = color::random(0.5, 1);
//                     auto fuzz = random_double(0, 0.5);
//                     sphere_material = make_shared<metal>(albedo, fuzz);
//                     world.add(make_shared<sphere>(center, 0.2, sphere_material));
//                 } else {
//                     // glass
//                     sphere_material = make_shared<dielectric>(1.5);
//                     world.add(make_shared<sphere>(center, 0.2, sphere_material));
//                 }
//             }
//         }
//     }

//     auto material1 = make_shared<dielectric>(1.5);
//     world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

//     auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
//     world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

//     auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
//     world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

//     return world;
// }

// hittable_list two_spheres() {
//     hittable_list objects;

//     auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));

//     objects.add(make_shared<sphere>(point3(0,-10, 0), 10, make_shared<lambertian>(checker)));
//     objects.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

//     return objects;
// }

// hittable_list simple_light() {
//     hittable_list objects;

//     auto checker = make_shared<checker_texture>(color(0.2, 0.3, 0.1), color(0.9, 0.9, 0.9));
//     objects.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(checker)));
//     objects.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(checker)));

//     auto difflight = make_shared<diffuse_light>(color(4,4,4));
//     point3 a(3,1,-2), b(5,3,-2), c(5,1,-2), d(3,3,-2);
//     objects.add(make_shared<triangle>(a, b, c, difflight));
//     objects.add(make_shared<triangle>(a, b, d, difflight));

//     return objects;
// }

#endif