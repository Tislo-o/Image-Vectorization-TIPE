#include "external/stb_image_write.h"
#include "external/stb_image.h"
#include "external/color_conversions.h"

#include <time.h>
#include "segmentation.h"

//- returns the preprocessed image 
//- modifies s->segID_of_px such as
//      segID_of_px[x] == DISCONTINUITY_PX <=> x belongs to the discontinuity map, ie (grad u)[x] > lambda/alpha
//      otherwise segID_of_px[x] = i, where i is unique for each x and if segID_of_px[x] = i > 0, there exists 
//      y such as segID_of_px[y] = i-1
//      s->nb_segments = max{segID_of_px[x]+1, x a pixel}
//- modifies (and first allocates) *discontinuities in puts in it the discontinuity px
bitmap_img discrete_mumford_shah(bitmap_img* f, float alpha, float lambda, float epsilon, segmentation* s, vec** discontinuity_px) {
    float tau = 0.25f, sigma = 0.5f;

    bitmap_img u = copy_bitmap_img(f);
    bitmap_img u_bar = copy_bitmap_img(f);
    bitmap_img prev_u = copy_bitmap_img(&u);

    gradient_field grad_of_u_bar, p, p_tild;

    grad_of_u_bar.data = malloc(sizeof(gradient_on_px) * NB_PX);
    p.data = malloc(sizeof(gradient_on_px) * NB_PX);
    p_tild.data = malloc(sizeof(gradient_on_px) * NB_PX);

    for (int i = 0; i < NB_PX; ++i) {
        p.data[i] =      NULL_GRAD;
        p_tild.data[i] = NULL_GRAD;
    }
    int iteration = 0;
    while(true) {
        //prev_u takes the data of u, performed by swapping pointers
        Color* temp = u.px_colors;
        u.px_colors = prev_u.px_colors;
        prev_u.px_colors = temp;

        //Dual ascent in p
        get_gradient_field(&u_bar, &grad_of_u_bar);
        for (int i = 0; i < NB_PX; ++i) {
            p_tild.data[i] = add_g(p.data[i], multiply_g(sigma, grad_of_u_bar.data[i]));
        }
        for (int i = 0; i < NB_PX; ++i) {
            if (gradient_magnitude_squaL(p_tild.data[i]) <= lambda*sigma*(sigma + 2*alpha)/alpha) {
                p.data[i] = multiply_g((2*alpha)/(sigma + 2*alpha), p_tild.data[i]);
            } else {
                p.data[i] = NULL_GRAD;
            }   
        }

        //Primal descent in u
        for (int i = 0; i < HEIGHT; ++i) {
            for (int j = 0; j < WIDTH; ++j) {
                int idx = i * WIDTH + j;
                Color_dif div = get_divergence(&p, i, j);
                Color u_tild_on_p = (Color){prev_u.px_colors[idx].L  + tau*div.L_dif, 
                                            prev_u.px_colors[idx].a + tau*div.a_dif,
                                            prev_u.px_colors[idx].b + tau*div.b_dif };

                u.px_colors[idx].L =   (u_tild_on_p.L   + 2*tau*f->px_colors[idx].L)   / (1 + 2*tau);
                u.px_colors[idx].a = (u_tild_on_p.a + 2*tau*f->px_colors[idx].a) / (1 + 2*tau);
                u.px_colors[idx].b = (u_tild_on_p.b  + 2*tau*f->px_colors[idx].b) / (1 + 2*tau);
            }
        }

        //Extrapolation step
        float theta = 1.f / (sqrtf(1 + 4*tau));
        tau = theta*tau;
        sigma = sigma / theta;
        for (int i = 0; i < NB_PX; ++i) {
            u_bar.px_colors[i].L = u.px_colors[i].L + theta*(u.px_colors[i].L - prev_u.px_colors[i].L);
            u_bar.px_colors[i].a = u.px_colors[i].a + theta*(u.px_colors[i].a - prev_u.px_colors[i].a);
            u_bar.px_colors[i].b = u.px_colors[i].b + theta*(u.px_colors[i].b - prev_u.px_colors[i].b);
        }
        if (iteration % 8 == 0) {
            float dist = image_distance(&u, &prev_u);
            printf("iteration: %d dist: %f, tau: %f, sigma : %f", iteration, dist, tau, sigma);
            if (dist < epsilon) {
                printf("  stopping\n");

                free(grad_of_u_bar.data);
                free(u_bar.px_colors);
                free(prev_u.px_colors);
                free(p.data);
                free(p_tild.data);
                break;
            }
            printf("\n");
        }

        ++iteration;
    }
    //construire la segmentation s, initialement constituée d'un segment pour chaque pixel
    gradient_field grad_of_result;
    grad_of_result.data = malloc(NB_PX*sizeof(gradient_on_px));
    get_gradient_field(&u, &grad_of_result);

    u32 next_segment_ID = 0;
    s->color_of_segment = malloc(NB_PX * sizeof(Color));
    s->px_of_segment = malloc(NB_PX * sizeof(vec));
    *discontinuity_px = empty_vec(sizeof(Pixel), NULL);

    for (Pixel i = 0; i < NB_PX; ++i) {
        if (gradient_magnitude_squaL(grad_of_result.data[i]) > lambda / alpha) {
            s->segID_of_px[i] = DISCONTINUITY_PX;
            vec_add(*discontinuity_px, &i);
        } else {
            s->segID_of_px[i] = next_segment_ID;
            s->color_of_segment[next_segment_ID] = u.px_colors[i];
            s->px_of_segment[next_segment_ID] = empty_vec(sizeof(Pixel), NULL);
            vec_add(s->px_of_segment[next_segment_ID], &i);
            next_segment_ID++;
        }
    }
    s->nb_segments = next_segment_ID;
    free(grad_of_result.data);

    return u;
}

//returns a bitmap_img* add modifies the global variables WIDTH, HEIGHT and NB_PX
bitmap_img* get_bitmap_img_from_file(char* filename) {
    bitmap_img* img = malloc(sizeof(bitmap_img));
    int channels;
    u8* stbi_output_data = stbi_load(filename, &WIDTH, &HEIGHT, &channels, 0);
    NB_PX = WIDTH * HEIGHT;
    if (stbi_output_data == NULL) {
        printf("Erreur de chargement.");
        return NULL;
    }
    if (NB_PX > (~((u32)0))) {
        printf("L'image contient trop de pixels.");
        return NULL;
    }
    img->px_colors = malloc(sizeof(Color) * NB_PX);
    for (int i = 0; i < NB_PX; ++i) {
        // img->px_colors[i].L = (float)stbi_output_data[i*channels];
        // img->px_colors[i].a = (float)stbi_output_data[i*channels+1];
        // img->px_colors[i].b = (float)stbi_output_data[i*channels+2];
        convertRGBtoLAB(stbi_output_data[i*channels], stbi_output_data[i*channels+1], stbi_output_data[i*channels+2],
                        &img->px_colors[i].L, &img->px_colors[i].a, &img->px_colors[i].b);
    }
    stbi_image_free(stbi_output_data);
    return img;
}

//L -> discontinuity pixels
//random color but not L for each segment
//puts in px_buf the colors
void visualize_segementation(segmentation* s, u8* px_buf) {
    u8* color_per_segment = malloc(s->nb_segments * 3 * sizeof(u8));
    for (int i = 0; i < s->nb_segments; ++i) {
        color_per_segment[i*3] = 255;
        color_per_segment[i*3+1] = 0;
        color_per_segment[i*3+2] = 0;
    }
    for (int i = 0; i < NB_PX; ++i) {
        u32 id = s->segID_of_px[i];
        if (id == DISCONTINUITY_PX) {
            px_buf[i*3] = 255;
            px_buf[i*3+1] = 0;
            px_buf[i*3+2] = 0;
        } else {
            if (color_per_segment[id*3] == 255) { //means the segmentation wasn't assigned a color yet
                //printf("NEW COLOR for id: %d i: %d|", id, i);
                if (id == 20) {
                    color_per_segment[id*3] = 0;
                    color_per_segment[id*3+1] = 0;
                    color_per_segment[id*3+2] = 0;
                } else if (id == 15) {
                    color_per_segment[id*3] = 255;
                    color_per_segment[id*3+1] = 255;
                    color_per_segment[id*3+2] = 255;
                } else {
                    color_per_segment[id*3] = rand() % 100;
                color_per_segment[id*3+1] = rand() % 100;
                color_per_segment[id*3+2] = rand() % 100;
                }
                
            } 
            px_buf[i*3] = color_per_segment[id*3];
            px_buf[i*3+1] = color_per_segment[id*3+1];
            px_buf[i*3+2] = color_per_segment[id*3+2];
        }
    }
    free(color_per_segment);
}

int main(int argc, char** argv) {
    if (argc < 3 || argc > 3){
        printf("Donnez (uniquement) en argument le nom de l'image source et le nom de l'image à générer.\n");
        return 1;
    }
    bitmap_img* src = get_bitmap_img_from_file(argv[1]);
    if (src == NULL) return 1;
     
    float alpha, lambda, merging_threshold;
    printf("Donner alpha, lambda et le taux de fusion: ");
    scanf("%f %f %f", &alpha, &lambda, &merging_threshold);
    clock_t start = clock();

    segmentation s;
    s.segID_of_px = malloc(NB_PX * sizeof(u32));
    vec* discontinuity_px;
    bitmap_img preprocessed = discrete_mumford_shah(src, alpha, lambda, 0.01f, &s, &discontinuity_px);

    clock_t end = clock();
    float ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("Calcul de mumford_shah, %d pixels non de discontinuité: %fms\n", s.nb_segments, ms);


    start = clock();
    merge_by_color_dif(&s, merging_threshold);
    end = clock();
    ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("Fusion en %d régions: %fms\n", s.nb_segments, ms);


    start = clock();
    segmentation* S = final_segmentation(&s, discontinuity_px);
    end = clock();
    ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("Partition finale de %d régions: %fms\n", S->nb_segments, s.nb_segments, ms);
    // printf("------relation de discontinuité------:\n");
    // for (int i = 0; i < s.nb_segments; ++i) {
    //     printf("%d: ", i);
    //     vec_print_data(gc->constraints[i]);
    // }

    start = clock();
    u8* px_buf = malloc(HEIGHT * WIDTH * sizeof(u8) * 3);
    for (int i = 0; i < HEIGHT * WIDTH; ++i) {
        // px_buf[i*3] = (unsigned char)preprocessed.px_colors[i].L;
        // px_buf[i*3+1] = (unsigned char)preprocessed.px_colors[i].a;
        // px_buf[i*3+2] = (unsigned char)preprocessed.px_colors[i].b;
        convertLabtoRGB(preprocessed.px_colors[i].L, preprocessed.px_colors[i].a, preprocessed.px_colors[i].b,
                        &px_buf[i*3], &px_buf[i*3+1], &px_buf[i*3+2]);
    }
    stbi_write_jpg(argv[2], WIDTH, HEIGHT, 3, px_buf, 100);
    
    char name_buf[100];
    strcpy(name_buf, argv[2]);
    strcat(name_buf, "_S0.jpg");
    visualize_segementation(&s, px_buf);
    stbi_write_jpg(name_buf, WIDTH, HEIGHT, 3, px_buf, 100);

    strcpy(name_buf, argv[2]);
    strcat(name_buf, "_S.jpg");
    visualize_segementation(S, px_buf);
    stbi_write_jpg(name_buf, WIDTH, HEIGHT, 3, px_buf, 100);

    free(px_buf);
    end = clock();
    ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("Ecriture de %s: %fms\n", argv[2], ms);




    //Free the segmentations s, S and discontinuity_px
    free(s.segID_of_px);
    free(s.color_of_segment);
    for (int i = 0; i < s.nb_segments; ++i) {
        free_vec(s.px_of_segment[i]);
    }
    free(s.px_of_segment);

    free(S->segID_of_px);
    free(S->color_of_segment);
    for (int i = 0; i < S->nb_segments; ++i) {
        free_vec(S->px_of_segment[i]);
    }
    free(S->px_of_segment);
    free(S);

    free_vec(discontinuity_px);


    //Free the images
    free(preprocessed.px_colors);
    free(src->px_colors);
    free(src);
    return 0;
}