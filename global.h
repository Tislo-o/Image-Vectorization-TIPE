#pragma once
#include <stdlib.h>
#include <math.h>
#include <stdint.h> 
#include <stdbool.h>
#include "vector.h"


typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

u32 WIDTH, HEIGHT;
u32 NB_PX; //WIDTH * HEIGHT

typedef struct  {
    s8 x, y;
}s8_tuple;

#define NO_PX UINT32_MAX
typedef u32 Pixel;
typedef struct{
    u16 x, y;
}Vertex; //sommet

//chaque composante est un flottant entre 0 et 1
typedef struct{
    float L, a, b;
}Color;


//image bitmap à 3 composantes (L, a, b) dans l'espace des couleurs CIELAB 
//L est entre 0 et 100, a et b sont entre -128 et 127
typedef struct{
    Color* px_colors;
}bitmap_img;

typedef struct{
    bool* data;
}pixel_set;


bitmap_img copy_bitmap_img(bitmap_img* img) {
    bitmap_img new;
    int bytes_needed = sizeof(Color)*NB_PX;
    new.px_colors = malloc(bytes_needed);
    memcpy(new.px_colors, img->px_colors, bytes_needed);
    return new;
}

//represents the difference between 2 colors
typedef struct {
    float L_dif, a_dif, b_dif;
}Color_dif;

//returns the color difference between c1 and c2
Color_dif color_dif_between(Color c1, Color c2) {
    return (Color_dif){(float)c1.L - c2.L, (float)c1.a - c2.a, (float)c1.b - c2.b};
}

//renvoie la distance au carré des couleurs c1 et c2 
float color_dif_magnitude_squared(Color_dif cd) {
    // return (2.f + cd.L_average/256.f)*cd.L_dif*cd.L_dif 
    //         + 4.f*cd.a_dif*cd.a_dif 
    //         + (2.f + (255.f - cd.L_average) / 256.f)*cd.b_dif*cd.b_dif;
     return (cd.b_dif*cd.b_dif + cd.L_dif*cd.L_dif + cd.a_dif*cd.a_dif);
} 


typedef struct {
    Color_dif x, y;
}gradient_on_px;

const gradient_on_px NULL_GRAD = (gradient_on_px){(Color_dif){0, 0, 0}, (Color_dif){0, 0, 0}};
//return the addition of 2 "gradient_on_px": g1 + g2
gradient_on_px add_g(gradient_on_px g1, gradient_on_px g2) {
    return (gradient_on_px) {(Color_dif){g1.x.L_dif  + g2.x.L_dif,  g1.x.a_dif   + g2.x.a_dif, 
                                         g1.x.b_dif + g2.x.b_dif},
                             (Color_dif){g1.y.L_dif  + g2.y.L_dif,  g1.y.a_dif   + g2.y.a_dif, 
                                         g1.y.b_dif + g2.y.b_dif}};                     
}
//return the substraction of 2 "gradient_on_px": g1 - g2
gradient_on_px substract_g(gradient_on_px g1, gradient_on_px g2) {
    return (gradient_on_px) {(Color_dif){g1.x.L_dif  - g2.x.L_dif,  g1.x.a_dif   - g2.x.a_dif, 
                                         g1.x.b_dif - g2.x.b_dif},
                             (Color_dif){g1.y.L_dif  - g2.y.L_dif,  g1.y.a_dif   - g2.y.a_dif, 
                                         g1.y.b_dif - g2.y.b_dif}};                     
}

//return the scalar multiplication of a with the gradient_on_px g: a.x
gradient_on_px multiply_g(float a, gradient_on_px g) {
    return (gradient_on_px) {(Color_dif){a*g.x.L_dif,  a*g.x.a_dif  , 
                                         a*g.x.b_dif},
                             (Color_dif){a*g.y.L_dif,  a*g.y.a_dif, 
                                         a*g.y.b_dif}};                     
}

float gradient_magnitude_squaL(gradient_on_px gpx) {
    return color_dif_magnitude_squared(gpx.x) + color_dif_magnitude_squared(gpx.y);
}

typedef struct {
    gradient_on_px* data;
}gradient_field;

//return the divergence of the pixel (i, j)
Color_dif get_divergence(gradient_field* g, int i, int j) {
    Color_dif res = (Color_dif){0, 0, 0};
    int idx = i * WIDTH + j;
    if (j >= 1) {
        res.L_dif     += g->data[idx].x.L_dif     - g->data[idx-1].x.L_dif;
        res.a_dif   += g->data[idx].x.a_dif   - g->data[idx-1].x.a_dif;
        res.b_dif    += g->data[idx].x.b_dif    - g->data[idx-1].x.b_dif;
    } else {
        res.L_dif     += g->data[idx].x.L_dif;
        res.a_dif   += g->data[idx].x.a_dif;
        res.b_dif    += g->data[idx].x.b_dif;
    }
    if (i >= 1) {
        res.L_dif     += g->data[idx].y.L_dif     - g->data[idx-WIDTH].y.L_dif;
        res.a_dif   += g->data[idx].y.a_dif   - g->data[idx-WIDTH].y.a_dif;
        res.b_dif    += g->data[idx].y.b_dif    - g->data[idx-WIDTH].y.b_dif;
    } else {
        res.L_dif     += g->data[idx].y.L_dif;
        res.a_dif   += g->data[idx].y.a_dif;
        res.b_dif    += g->data[idx].y.b_dif;
    }
    return res;
}

//returns in g (where g.data points to allocated memory) the gradient field associated with the bitmap_img* img
void get_gradient_field(bitmap_img* img, gradient_field* g) {
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            int idx = i * WIDTH + j;

            if (j + 1 < WIDTH) {
                g->data[idx].x =
                    color_dif_between(img->px_colors[idx + 1], img->px_colors[idx]);
            } else {
                g->data[idx].x = (Color_dif){0};
            }

            if (i + 1 < HEIGHT) {
                g->data[idx].y =
                    color_dif_between(img->px_colors[idx + WIDTH], img->px_colors[idx]);
            } else {
                g->data[idx].y = (Color_dif){0};
            }
        }
    }
}
//a and b must have NB_PX pixels
float image_distance(bitmap_img* a, bitmap_img* b) {
    float total = 0.f;
    for (int i = 0; i < NB_PX; ++i) {
        total += sqrtf(color_dif_magnitude_squared(color_dif_between(a->px_colors[i], b->px_colors[i])));
    }
    return total /= (float)NB_PX;
}



