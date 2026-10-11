#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>

#include "GamesEngineeringBase.h" 
#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

#include "matrix.h"
#include "colour.h"
#include "mesh.h"
#include "zbuffer.h"
#include "renderer.h"
#include "RNG.h"
#include "light.h"
//#include "triangle.h"
#include "MuliLine_Triangle.h"//my new triangle headfile
#include "Tpool.h"


//button to control scene and thread
const int funcnum = 3;
int numThreads = 8;
ThreadPool gPool(numThreads);


void render(Renderer& renderer, Mesh* mesh, matrix& camera, Light& L) {
    // Combine perspective, camera, and world transformations for the mesh
    matrix p = renderer.perspective * camera * mesh->world;

    // Iterate through all triangles in the mesh
    for (triIndices& ind : mesh->triangles) {
        Vertex t[3]; // Temporary array to store transformed triangle vertices

        // Transform each vertex of the triangle
        for (unsigned int i = 0; i < 3; i++) {
            t[i].p = p * mesh->vertices[ind.v[i]].p; // Apply transformations
            t[i].p.divideW(); // Perspective division to normalize coordinates

            // Transform normals into world space for accurate lighting
            // no need for perspective correction as no shearing or non-uniform scaling
            t[i].normal = mesh->world * mesh->vertices[ind.v[i]].normal; 
            t[i].normal.normalise();

            // Map normalized device coordinates to screen space
            t[i].p[0] = (t[i].p[0] + 1.f) * 0.5f * static_cast<float>(renderer.canvas.getWidth());
            t[i].p[1] = (t[i].p[1] + 1.f) * 0.5f * static_cast<float>(renderer.canvas.getHeight());
            t[i].p[1] = renderer.canvas.getHeight() - t[i].p[1]; // Invert y-axis

            // Copy vertex colours
            t[i].rgb = mesh->vertices[ind.v[i]].rgb;
        }

        // Clip triangles with Z-values outside [-1, 1]
        if (fabs(t[0].p[2]) > 1.0f || fabs(t[1].p[2]) > 1.0f || fabs(t[2].p[2]) > 1.0f) continue;

        // Create a triangle object and render it
        triangle tri(t[0], t[1], t[2]);
        tri.draw(renderer, L, mesh->ka, mesh->kd);
    }
}

// each round only draw One y-axis Line ,avoid drawing repeatly on the same tile 
void renderMeshSlice(Renderer& renderer, Mesh* mesh, const matrix& camera, Light L,
    int y0, int y1)
{
    matrix p = renderer.perspective * camera * mesh->world;

    int W = renderer.canvas.getWidth();
    int H = renderer.canvas.getHeight();

    for (triIndices& ind : mesh->triangles) 
    {
        Vertex t[3];

        for (int i = 0; i < 3; i++) {
            t[i].p = p * mesh->vertices[ind.v[i]].p;
            t[i].p.divideW();

            t[i].normal = mesh->world * mesh->vertices[ind.v[i]].normal;
            t[i].normal.normalise();

            t[i].p[0] = (t[i].p[0] + 1.f) * 0.5f * (float)W;
            t[i].p[1] = (t[i].p[1] + 1.f) * 0.5f * (float)H;
            t[i].p[1] = (float)H - t[i].p[1];

            t[i].rgb = mesh->vertices[ind.v[i]].rgb;
        }

        if (fabs(t[0].p[2]) > 1.0f || fabs(t[1].p[2]) > 1.0f || fabs(t[2].p[2]) > 1.0f)
            continue;

        triangle tri(t[0], t[1], t[2]);
        tri.draw(renderer, L, mesh->ka, mesh->kd, y0, y1);
    }
}

// treadpool
 void renderSceneMT(Renderer& renderer, const std::vector<Mesh*>& scene,
    const matrix& camera, Light& L)
{
    int H = (int)renderer.canvas.getHeight();
    if (H <= 0) return;

    int threads = std::max(1, std::min(numThreads, 11)); 
    int slice = (H + threads - 1) / threads;

    for (int t = 0; t < threads; t++) {
        int y0 = t * slice;
        int y1 = std::min(H, y0 + slice);

        gPool.submit([&, y0, y1, Lcopy = L]() mutable {
            for (Mesh* m : scene) {
                renderMeshSlice(renderer, m, camera, Lcopy, y0, y1);
            }
            });
    }

    gPool.wait();
}

// Test scene function to demonstrate rendering with user-controlled transformations
void sceneTest() {
    Renderer renderer;
    // create light source {direction, diffuse intensity, ambient intensity}
    Light L{ vec4(0.f, 1.f, 1.f, 0.f), colour(1.0f, 1.0f, 1.0f), colour(0.2f, 0.2f, 0.2f) };
    // camera is just a matrix
    matrix camera = matrix::makeIdentity(); // Initialize the camera with identity matrix

    bool running = true; // Main loop control variable

    std::vector<Mesh*> scene; // Vector to store scene objects

    // Create a sphere and a rectangle mesh
    Mesh mesh = Mesh::makeSphere(1.0f, 10, 20);
    //Mesh mesh2 = Mesh::makeRectangle(-2, -1, 2, 1);

    // add meshes to scene
    scene.push_back(&mesh);
   // scene.push_back(&mesh2); 

    float x = 0.0f, y = 0.0f, z = -4.0f; // Initial translation parameters
    mesh.world = matrix::makeTranslation(x, y, z);
    //mesh2.world = matrix::makeTranslation(x, y, z) * matrix::makeRotateX(0.01f);

    // Main rendering loop
    while (running) {
        renderer.canvas.checkInput(); // Handle user input
        renderer.clear(); // Clear the canvas for the next frame

        // Apply transformations to the meshes
     //   mesh2.world = matrix::makeTranslation(x, y, z) * matrix::makeRotateX(0.01f);
        mesh.world = matrix::makeTranslation(x, y, z);

        // Handle user inputs for transformations
        if (renderer.canvas.keyPressed(VK_ESCAPE)) break;
        if (renderer.canvas.keyPressed('A')) x += -0.1f;
        if (renderer.canvas.keyPressed('D')) x += 0.1f;
        if (renderer.canvas.keyPressed('W')) y += 0.1f;
        if (renderer.canvas.keyPressed('S')) y += -0.1f;
        if (renderer.canvas.keyPressed('Q')) z += 0.1f;
        if (renderer.canvas.keyPressed('E')) z += -0.1f;

        // Render each object in the scene
        for (auto& m : scene)
            render(renderer, m, camera, L);

        renderer.present(); // Display the rendered frame
    }
}

//generate a random rotation matrix
matrix makeRandomRotation() {
    RandomNumberGenerator& rng = RandomNumberGenerator::getInstance();
    unsigned int r = rng.getRandomInt(0, 3);

    switch (r) {
    case 0: return matrix::makeRotateX(rng.getRandomFloat(0.f, 2.0f * M_PI));
    case 1: return matrix::makeRotateY(rng.getRandomFloat(0.f, 2.0f * M_PI));
    case 2: return matrix::makeRotateZ(rng.getRandomFloat(0.f, 2.0f * M_PI));
    default: return matrix::makeIdentity();
    }
}

//render a scene with multiple objects and dynamic transformations

void scene1() {
    Renderer renderer;
    matrix camera;
    Light L{ vec4(0.f, 1.f, 1.f, 0.f), colour(1.0f, 1.0f, 1.0f), colour(0.2f, 0.2f, 0.2f) };

    bool running = true;

    std::vector<Mesh*> scene;

    // Create a scene of 40 cubes with random rotations
    for (unsigned int i = 0; i < 20; i++) {
        Mesh* m = new Mesh();
        *m = Mesh::makeCube(1.f);
        m->world = matrix::makeTranslation(-2.0f, 0.0f, (-3 * static_cast<float>(i))) * makeRandomRotation();
        scene.push_back(m);
        m = new Mesh();
        *m = Mesh::makeCube(1.f);
        m->world = matrix::makeTranslation(2.0f, 0.0f, (-3 * static_cast<float>(i))) * makeRandomRotation();
        scene.push_back(m);
    }

    float zoffset = 8.0f; // Initial camera Z-offset
    float step = -0.1f;  // Step size for camera movement

    auto start = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    int cycle = 0;

    // Main rendering loop
    while (running) {
        renderer.canvas.checkInput();
        renderer.clear();

        camera = matrix::makeTranslation(0, 0, -zoffset); // Update camera position

        // Rotate the first two cubes in the scene
        scene[0]->world = scene[0]->world * matrix::makeRotateXYZ(0.1f, 0.1f, 0.0f);
        scene[1]->world = scene[1]->world * matrix::makeRotateXYZ(0.0f, 0.1f, 0.2f);

        if (renderer.canvas.keyPressed(VK_ESCAPE)) break;

        zoffset += step;
        if (zoffset < -60.f || zoffset > 8.f) {
            step *= -1.f;
            if (++cycle % 2 == 0) {
                end = std::chrono::high_resolution_clock::now();
                std::cout << cycle / 2 << " :" << std::chrono::duration<double, std::milli>(end - start).count() << "ms\n";
                start = std::chrono::high_resolution_clock::now();
            }
        }

#if OPTIMISATION_Multitread
        renderSceneMT(renderer, scene, camera, L);

#else
        for (auto& m : scene) render(renderer, m, camera, L);
#endif

        renderer.present();
    }

    for (auto& m : scene)
        delete m;
}

// Scene with a grid of cubes and a moving sphere
// No input variables
void scene2() {
    Renderer renderer;
    matrix camera = matrix::makeIdentity();
    Light L{ vec4(0.f, 1.f, 1.f, 0.f), colour(1.0f, 1.0f, 1.0f), colour(0.2f, 0.2f, 0.2f) };

    std::vector<Mesh*> scene;

    struct rRot { float x; float y; float z; }; // Structure to store random rotation parameters
    std::vector<rRot> rotations;

    RandomNumberGenerator& rng = RandomNumberGenerator::getInstance();

    // Create a grid of cubes with random rotations
    for (unsigned int y = 0; y < 6; y++) {
        for (unsigned int x = 0; x < 8; x++) {
            Mesh* m = new Mesh();
            *m = Mesh::makeCube(1.f);
            scene.push_back(m);
            m->world = matrix::makeTranslation(-7.0f + (static_cast<float>(x) * 2.f), 5.0f - (static_cast<float>(y) * 2.f), -8.f);
            rRot r{ rng.getRandomFloat(-.1f, .1f), rng.getRandomFloat(-.1f, .1f), rng.getRandomFloat(-.1f, .1f) };
            rotations.push_back(r);
        }
    }

    // Create a sphere and add it to the scene
    Mesh* sphere = new Mesh();
    *sphere = Mesh::makeSphere(1.0f, 10, 20);
    scene.push_back(sphere);
    float sphereOffset = -6.f;
    float sphereStep = 0.1f;
    sphere->world = matrix::makeTranslation(sphereOffset, 0.f, -6.f);

    auto start = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    int cycle = 0;

    bool running = true;
    while (running) {
        renderer.canvas.checkInput();
        renderer.clear();

        // Rotate each cube in the grid
        for (unsigned int i = 0; i < rotations.size(); i++)
            scene[i]->world = scene[i]->world * matrix::makeRotateXYZ(rotations[i].x, rotations[i].y, rotations[i].z);

        // Move the sphere back and forth
        sphereOffset += sphereStep;
        sphere->world = matrix::makeTranslation(sphereOffset, 0.f, -6.f);
        if (sphereOffset > 6.0f || sphereOffset < -6.0f) {
            sphereStep *= -1.f;
            if (++cycle % 2 == 0) {
                end = std::chrono::high_resolution_clock::now();
                std::cout << cycle / 2 << " :" << std::chrono::duration<double, std::milli>(end - start).count() << "ms\n";
                start = std::chrono::high_resolution_clock::now();
            }
        }

        if (renderer.canvas.keyPressed(VK_ESCAPE)) break;

        
#if OPTIMISATION_Multitread
        renderSceneMT(renderer, scene, camera, L);
#else
        for (auto& m : scene) render(renderer, m, camera, L);
#endif

           
        renderer.present();
    }

    for (auto& m : scene)
        delete m;
}

void scene3() {
    Renderer renderer;
    matrix camera = matrix::makeIdentity();
    Light L{ vec4(0.f, 1.f, 1.f, 0.f),
             colour(1.0f, 1.0f, 1.0f),
             colour(0.2f, 0.2f, 0.2f) };

    std::vector<Mesh*> scene;

    struct rRot { float x; float y; float z; };
    std::vector<rRot> rotations;

    RandomNumberGenerator& rng = RandomNumberGenerator::getInstance();


    // 1 CUbes like scene2 on the middle level

    const unsigned int gridH = 5;
    const unsigned int gridW = 7;
    const unsigned int gridCount = gridH * gridW;

    for (unsigned int y = 0; y < gridH; y++) {
        for (unsigned int x = 0; x < gridW; x++) {
            Mesh* m = new Mesh();
            *m = Mesh::makeCube(1.5f);
            scene.push_back(m);

            m->world = matrix::makeTranslation(-7.0f + (static_cast<float>(x) * 2.f),
                5.0f - (static_cast<float>(y) * 2.f),
                -8.f);

            rRot r{ rng.getRandomFloat(-.1f, .1f),
                    rng.getRandomFloat(-.1f, .1f),
                    rng.getRandomFloat(-.1f, .1f) };
            rotations.push_back(r);
        }
    }

   
    // 2 HUGE cubes no rotation just for covering other objects test

    {
        Mesh* m = new Mesh();
        *m = Mesh::makeCube(16.f);
        scene.push_back(m);
        m->world = matrix::makeTranslation(0.0f, 0.0f, -24.f);
    }
    {
        Mesh* m = new Mesh();
        *m = Mesh::makeCube(24.f);
        scene.push_back(m);
        m->world = matrix::makeTranslation(0.0f, 0.0f, -32.f);
    }
    {
        Mesh* m = new Mesh();
        *m = Mesh::makeCube(30.f);
        scene.push_back(m);
        m->world = matrix::makeTranslation(0.0f, 0.0f, -40.f);
    }

    // 3 a bigger closer ball in the very front 
   
    Mesh* sphere = new Mesh();
    *sphere = Mesh::makeSphere(3.0f, 10, 10);  
    scene.push_back(sphere);

    float sphereOffset = -6.f;
    float sphereStep = 0.1f;
    sphere->world = matrix::makeTranslation(sphereOffset, 0.f, -5.f); // ✅ B：z 从 -6 -> -5

    
    auto start = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock> end;
    int cycle = 0;

    bool running = true;
    while (running) {


        renderer.canvas.checkInput();
        renderer.clear();
       
        for (unsigned int i = 0; i < gridCount; i++) {
            scene[i]->world = scene[i]->world * matrix::makeRotateXYZ(rotations[i].x, rotations[i].y, rotations[i].z);
        }

        sphereOffset += sphereStep;
        sphere->world = matrix::makeTranslation(sphereOffset, 0.f, -5.f);

        if (sphereOffset > 6.0f || sphereOffset < -6.0f) {
            sphereStep *= -1.f;
            if (++cycle % 2 == 0) {
                end = std::chrono::high_resolution_clock::now();
                std::cout << cycle / 2 << " :"
                    << std::chrono::duration<double, std::milli>(end - start).count()
                    << "ms\n";
                start = std::chrono::high_resolution_clock::now();
            }
        }

        if (renderer.canvas.keyPressed(VK_ESCAPE)) break;

#if OPTIMISATION_Multitread
        renderSceneMT(renderer, scene, camera, L);

#else
        for (auto& m : scene) render(renderer, m, camera, L);
#endif

        renderer.present();
    }

    for (auto& m : scene)
        delete m;
}


int main() {


    //These are for time reord
    //for (int t = 1; t <= 11; t++) {
    //    numThreads = t;
    //    const int warmup = 5;
    //    const int frames = 25;
    //    double sum = 0.0;
    //    int count = 0;

    //    for (int f = 0; f < frames; f++) {
    //        auto start = std::chrono::high_resolution_clock::now();
    //        // Uncomment the desired scene function to run
    //        if (funcnum == 1)
    //            scene1();
    //        else if (funcnum == 2)
    //            scene2();
    //        else if (funcnum == 3)
    //            scene3();
    //        //sceneTest(); 

    //        auto end = std::chrono::high_resolution_clock::now();
    //        double ms = std::chrono::duration<double, std::milli>(end - start).count();

    //        if (f >= warmup) {
    //            sum += ms;
    //            count++;
    //        }
    //   }
    //    double avg = sum / count;
    //    std::cout << "scene1," << t << "," << avg << "ms\n";
    //}
   
    // Uncomment the desired scene function to run
    if (funcnum == 1)
        scene1();
    else if (funcnum == 2)
        scene2();
    else if (funcnum == 3)
        scene3();
    //sceneTest();
    

    return 0;
}