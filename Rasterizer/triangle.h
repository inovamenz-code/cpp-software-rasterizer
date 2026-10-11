#pragma once

#include "mesh.h"
#include "colour.h"
#include "renderer.h"
#include "light.h"
#include <iostream>
#include <algorithm>
#include <cmath>


#include "optimisation.h"
// Simple support class for a 2D vector
class vec2D {
public:
    float x, y;

    // Default constructor initializes both components to 0
    vec2D() { x = y = 0.f; };

    // Constructor initializes components with given values
    vec2D(float _x, float _y) : x(_x), y(_y) {}

    // Constructor initializes components from a vec4
    vec2D(vec4 v) {
        x = v[0];
        y = v[1];
    }

    // Display the vector components
    void display() { std::cout << x << '\t' << y << std::endl; }

    // Overloaded subtraction operator for vector subtraction
    vec2D operator- (vec2D& v) {
        vec2D q;
        q.x = x - v.x;
        q.y = y - v.y;
        return q;
    }
};

// Class representing a triangle for rendering purposes
class triangle {
    Vertex v[3];       // Vertices of the triangle
    float area;        // Area of the triangle
    colour col[3];     // Colors for each vertex of the triangle

public:
    // Constructor initializes the triangle with three vertices
    // Input Variables:
    // - v1, v2, v3: Vertices defining the triangle
    triangle(const Vertex& v1, const Vertex& v2, const Vertex& v3) {
        v[0] = v1;
        v[1] = v2;
        v[2] = v3;

        // Calculate the 2D area of the triangle
        vec2D e1 = vec2D(v[1].p - v[0].p);
        vec2D e2 = vec2D(v[2].p - v[0].p);
        area = std::fabs(e1.x * e2.y - e1.y * e2.x);
    }

    // Helper function to compute the cross product for barycentric coordinates
    // Input Variables:
    // - v1, v2: Edges defining the vector
    // - p: Point for which coordinates are being calculated
    float getC(vec2D v1, vec2D v2, vec2D p) {
        vec2D e = v2 - v1;
        vec2D q = p - v1;
        return q.y * e.x - q.x * e.y;
    }
    //OP1 

    inline float edgeC(const vec2D& a, const vec2D& b, float x, float y)
    {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        // exactly the same formula as getC(v1,v2,p)
        return (y - a.y) * dx - (x - a.x) * dy;
    }


    // Compute barycentric coordinates for a given point
    // Input Variables:
    // - p: Point to check within the triangle
    // Output Variables:
    // - alpha, beta, gamma: Barycentric coordinates of the point
    // Returns true if the point is inside the triangle, false otherwise
    bool getCoordinates(vec2D p, float& alpha, float& beta, float& gamma)
    {
        alpha = getC(vec2D(v[0].p), vec2D(v[1].p), p) / area;
        beta = getC(vec2D(v[1].p), vec2D(v[2].p), p) / area;
        gamma = getC(vec2D(v[2].p), vec2D(v[0].p), p) / area;

        if (alpha < 0.f || beta < 0.f || gamma < 0.f) return false;
        return true;
    }

    // Template function to interpolate values using barycentric coordinates
    // Input Variables:
    // - alpha, beta, gamma: Barycentric coordinates
    // - a1, a2, a3: Values to interpolate
    // Returns the interpolated value
    template <typename T>
    T interpolate(float alpha, float beta, float gamma, T a1, T a2, T a3) {
        return (a1 * alpha) + (a2 * beta) + (a3 * gamma);
    }

    // Draw the triangle on the canvas
    // Input Variables:
    // - renderer: Renderer object for drawing
    // - L: Light object for shading calculations
    // - ka, kd: Ambient and diffuse lighting coefficients
    void draw(Renderer& renderer, Light& L, float ka, float kd) {
        vec2D minV, maxV;

        // Get the screen-space bounds of the triangle
        getBoundsWindow(renderer.canvas, minV, maxV);

        // Skip very small triangles
        if (area < 1.f) return;

        //----------------------------------------------------------------------op2--ZU----
        // Iterate over the bounding box and check each pixel
#if OPTIMISATION_edgefunc

        int x0 = (int)(minV.x);
        int x1 = (int)ceil(maxV.x);
        int y0 = (int)(minV.y);
        int y1 = (int)ceil(maxV.y);

        // 2D positions
        vec2D p0(v[0].p), p1(v[1].p), p2(v[2].p);

        // Use the SAME area as your constructor (fabs)
        // so invArea is consistent with current behaviour
        float invArea = 1.0f / area;

        // Precompute edge deltas for incremental update
        // For edgeC(a,b,x,y) = (y-a.y)*dx - (x-a.x)*dy
        // stepping x+1 adds dE/dx = -dy
        // stepping y+1 adds dE/dy =  dx
        float dx01 = p1.x - p0.x, dy01 = p1.y - p0.y;
        float dx12 = p2.x - p1.x, dy12 = p2.y - p1.y;
        float dx20 = p0.x - p2.x, dy20 = p0.y - p2.y;

        float dE01_dx = -dy01, dE01_dy = dx01; // alpha edge (0->1)
        float dE12_dx = -dy12, dE12_dy = dx12; // beta  edge (1->2)
        float dE20_dx = -dy20, dE20_dy = dx20; // gamma edge (2->0)

        // IMPORTANT: keep pixel sample point consistent with your old code:
        // old code used vec2D((float)x,(float)y), so we use (x,y) not (x+0.5,y+0.5)
        float startX = (float)x0;
        float startY = (float)y0;

        // Edge values at row start
        float E01_row = edgeC(p0, p1, startX, startY); // alpha*area
        float E12_row = edgeC(p1, p2, startX, startY); // beta*area
        float E20_row = edgeC(p2, p0, startX, startY); // gamma*area

        //====================================OP3++ZU
#if OPTIMISATION_NormalizeOut
        vec4 wi = L.omega_i;
        wi.normalise();

#else
S
        L.omega_i.normalise();
#endif
        for (int y = y0; y < y1; ++y) {
            float E01 = E01_row;
            float E12 = E12_row;
            float E20 = E20_row;

            for (int x = x0; x < x1; ++x) {

                // inside test
                if (!(E01 < 0.f || E12 < 0.f || E20 < 0.f)) {

                    // barycentric weight
                    float w0 = E12 * invArea; // v0 weight
                    float w1 = E20 * invArea; // v1 weight
                    float w2 = E01 * invArea; // v2 weight

                    // first depth
                    float depth = (v[0].p[2] * w0) + (v[1].p[2] * w1) + (v[2].p[2] * w2);

                    // then Z-test（
#if OPTIMISATION_zTest
                    float& z = renderer.zbuffer(x, y); // once
                    bool pass = (z > depth && depth > 0.001f);
#else
                    bool pass = (renderer.zbuffer(x, y) > depth && depth > 0.001f);
#endif

                    if (pass) {

                       
#if OPTIMISATION_zTest
                        z = depth;
#else
                        renderer.zbuffer(x, y) = depth;
#endif
                        // colour interpolate + clamp
                        colour c = (v[0].rgb * w0) + (v[1].rgb * w1) + (v[2].rgb * w2);
                        c.clampColour();

                        // normal interpolate + normalise
                        vec4 normal = (v[0].normal * w0) + (v[1].normal * w1) + (v[2].normal * w2);
                        normal.normalise();

                        // lighting
#if OPTIMISATION_NormalizeOut
                        float dot = std::max(vec4::dot(wi, normal), 0.0f);
#else
                        float dot = std::max(vec4::dot(L.omega_i, normal), 0.0f);
#endif

                        colour out = (c * kd) * (L.L * dot) + (L.ambient * ka);

                        unsigned char r, g, b;
                        out.toRGB(r, g, b);
                        renderer.canvas.draw(x, y, r, g, b);

                       
                    }
                }

                // step x+1 (edge increment)
                E01 += dE01_dx;
                E12 += dE12_dx;
                E20 += dE20_dx;
            }


            // step y+1
            E01_row += dE01_dy;
            E12_row += dE12_dy;
            E20_row += dE20_dy;
        }
#else
        //Iterate over the bounding box and check each pixel
        L.omega_i.normalise();
            for (int y = (int)(minV.y); y < (int)ceil(maxV.y); y++) {
                for (int x = (int)(minV.x); x < (int)ceil(maxV.x); x++) {
                    float alpha, beta, gamma;

                    // Check if the pixel lies inside the triangle
                    if (getCoordinates(vec2D((float)x, (float)y), alpha, beta, gamma)) {
                        // Interpolate color, depth, and normals
                        colour c = interpolate(beta, gamma, alpha, v[0].rgb, v[1].rgb, v[2].rgb);
                        c.clampColour();
                        float depth = interpolate(beta, gamma, alpha, v[0].p[2], v[1].p[2], v[2].p[2]);
                        vec4 normal = interpolate(beta, gamma, alpha, v[0].normal, v[1].normal, v[2].normal);
                        normal.normalise();
                        // Perform Z-buffer test and apply shading
                        if (renderer.zbuffer(x, y) > depth && depth > 0.001f) {
                            // typical shader begin
                           
                            float dot = std::max(vec4::dot(L.omega_i, normal), 0.0f);
                            colour a = (c * kd) * (L.L * dot) + (L.ambient * ka); // using kd instead of ka for ambient
                            // typical shader end
                            unsigned char r, g, b;
                            a.toRGB(r, g, b);
                            renderer.canvas.draw(x, y, r, g, b);
                            renderer.zbuffer(x, y) = depth;
                        }
                    }
                }
            }
#endif
    }

    // Compute the 2D bounds of the triangle
    // Output Variables:
    // - minV, maxV: Minimum and maximum bounds in 2D space
    void getBounds(vec2D& minV, vec2D& maxV) {
        minV = vec2D(v[0].p);
        maxV = vec2D(v[0].p);
        for (unsigned int i = 1; i < 3; i++) {
            minV.x = std::min(minV.x, v[i].p[0]);
            minV.y = std::min(minV.y, v[i].p[1]);
            maxV.x = std::max(maxV.x, v[i].p[0]);
            maxV.y = std::max(maxV.y, v[i].p[1]);
        }
    }

    // Compute the 2D bounds of the triangle, clipped to the canvas
    // Input Variables:
    // - canvas: Reference to the rendering canvas
    // Output Variables:
    // - minV, maxV: Clipped minimum and maximum bounds
    void getBoundsWindow(GamesEngineeringBase::Window& canvas, vec2D& minV, vec2D& maxV) {
        getBounds(minV, maxV);
        minV.x = std::max(minV.x, static_cast<float>(0));
        minV.y = std::max(minV.y, static_cast<float>(0));
        maxV.x = std::min(maxV.x, static_cast<float>(canvas.getWidth()));
        maxV.y = std::min(maxV.y, static_cast<float>(canvas.getHeight()));
    }

    // Debugging utility to display the triangle bounds on the canvas
    // Input Variables:
    // - canvas: Reference to the rendering canvas
    void drawBounds(GamesEngineeringBase::Window& canvas) {
        vec2D minV, maxV;
        getBounds(minV, maxV);

        for (int y = (int)minV.y; y < (int)maxV.y; y++) {
            for (int x = (int)minV.x; x < (int)maxV.x; x++) {
                canvas.draw(x, y, 255, 0, 0);
            }
        }
    }

    // Debugging utility to display the coordinates of the triangle vertices
    void display() {
        for (unsigned int i = 0; i < 3; i++) {
            v[i].p.display();
        }
        std::cout << std::endl;
    }
};
