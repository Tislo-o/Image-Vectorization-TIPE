#pragma once
#include "global.h"
#include "math.h"
#include "hash_table.h"
#include "queue.h"

typedef enum{
    TOP = 0,
    RIGHT = 1,
    BOTTOM = 2,
    LEFT = 3,
    ENUM_END
}Straight_Dir;
const s8_tuple straight_dir[] = { //décalages x/y pour passer de pixel à pixel ou de sommet à sommet
    {0,-1},{1,0},{0,1},{-1,0}
};

typedef enum{
    TOP_RIGHT = 0,
    BOTTOM_RIGHT = 1,
    BOTTOM_LEFT = 2,
    TOP_LEFT = 3,
    ERROR = 4
}Angled_Dir;
const s8_tuple dir_VToP[] = { //décalages x/y pour passer de sommet à pixel
    {0,-1},{0,0},{-1,0},{-1,-1}
};
const s8_tuple dir_PToV[] = { //décalages x/y pour passer de pixel à sommet
    {1,0},{1,1},{0,1},{0,0}
};
//convertit 2 directions "droites" d1 et d2 en une direction en biais valant straights_to_angled[d1*4 + d2]
const Angled_Dir straights_to_angled[16] = {
    ERROR, TOP_RIGHT, ERROR, TOP_LEFT,       //(TOP, TOP) (TOP, RIGHT) (TOP, BOTTOM) (TOP, LEFT)
    TOP_RIGHT, ERROR, BOTTOM_RIGHT, ERROR,   //(RIGHT, TOP) (RIGHT, RIGHT) (RIGHT, BOTTOM) (RIGHT, LEFT)
    ERROR, BOTTOM_RIGHT, ERROR, BOTTOM_LEFT, //(BOTTOM, TOP) (BOTTOM, RIGHT) (BOTTOM, BOTTOM) (BOTTOM, LEFT)
    TOP_LEFT,  ERROR, BOTTOM_LEFT, ERROR,     //(LEFT, TOP) (LEFT, RIGHT) (LEFT, BOTTOM) (LEFT, LEFT)
};

//Si le pixel voisin du pixel src est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le pixel correspondant
bool PtoP_neighbour(Pixel src, Pixel* dst, Straight_Dir d) {
    u16 x = src % WIDTH;
    u16 y = src / WIDTH;
    int nx = x + straight_dir[d].x;
    int ny = y + straight_dir[d].y;
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) return false;
    *dst = ny * WIDTH + nx;
    return true;
}
//Un sommet voisin d'un pixel est toujours dans l'image
//Met dans dst le sommet correspondant
void PtoV_neighbour(Pixel src, Vertex* dst, Angled_Dir d) {
    u16 x = src % WIDTH;
    u16 y = src / WIDTH;
    int nx = x + dir_PToV[d].x;
    int ny = y + dir_PToV[d].y;
    assert(nx >= 0 && nx < WIDTH+1 && ny >= 0 && ny < HEIGHT+1 && "Ce cas est impossible avec un pixel dans l'image");
    dst->x = nx;
    dst->y = ny;
}
//Si le pixel voisin du sommet src est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le pixel correspondant
bool VtoP_neighbour(Vertex src, Pixel* dst, Angled_Dir d) {
    int nx = src.x + dir_VToP[d].x;
    int ny = src.y + dir_VToP[d].y;
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) return false;
    *dst = ny * WIDTH + nx;
    return true;
}
//Si le sommet voisin du sommet src est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le sommet correspondant
bool VtoV_neighbour(Vertex src, Vertex* dst, Straight_Dir d) {
    int nx = src.x + straight_dir[d].x;
    int ny = src.y + straight_dir[d].y;
    if (nx < 0 || nx >= WIDTH+1 || ny < 0 || ny >= HEIGHT+1) return false;
    dst->x = nx;
    dst->y = ny;
    return true;
}
  

#define DISCONTINUITY_PX UINT32_MAX
typedef struct {
    u32* segID_of_px; //segment ID for each pixel
    int nb_segments; //number of segments
    Color* color_of_segment; //color of each segment
    vec** px_of_segment; //vec* of px for each segment, the first px of px_of_segment[i] should be min{x belongs to segment i}
}segmentation;


void merge_by_color_dif(segmentation* s, float merging_threshold) {
    int old_nb_segments = s->nb_segments;
    bool merging_occured = true;

    while (merging_occured) {
        merging_occured = false;
        for (int i = 0; i < old_nb_segments; ++i) {
            if (s->px_of_segment[i] == NULL) continue; //si le segment i a été supprimé, on le saute

            float min_dist_squared = INFINITY;
            Pixel best_neighbour;

            Pixel starting_px = *(Pixel*)vec_get_element(s->px_of_segment[i], 0);
            Color seg_color = s->color_of_segment[i];

             //On ajoute le sommet en haut à gauche
            Vertex starting_vertex;
            PtoV_neighbour(starting_px, &starting_vertex, TOP_LEFT);

            Straight_Dir d_mov = RIGHT; //direction dans le sens du déplacement
            Straight_Dir d_cen = BOTTOM; //direction vers le centre de la forme
            Straight_Dir tmp;
            Vertex cur;                  //le sommet courant
            PtoV_neighbour(starting_px, &cur, TOP_RIGHT);
            while (starting_vertex.x != cur.x || starting_vertex.y != cur.y) {
                Angled_Dir d1 = straights_to_angled[d_mov*4+d_cen];              //direction résultante de d_mov et d_cen
                Angled_Dir d2 = straights_to_angled[d_mov*4+((d_cen + 2) & 3)]; //direction résultante de d_mov et l'opposé de d_cen
                Pixel p1, p2;      
                bool p1_in_image = VtoP_neighbour(cur, &p1, d1);
                bool p2_in_image = VtoP_neighbour(cur, &p2, d2);
                if (!p1_in_image || s->segID_of_px[p1] != i) { //si p1 est hors de la forme
                    tmp = d_mov;
                    d_mov = d_cen;            //direction du mouvement est maintenant celle du centre
                    d_cen = (tmp + 2) & 3;    //direction du centre prend la direction opposée du mouvement 
                    if (p1_in_image && s->segID_of_px[p1] != DISCONTINUITY_PX) {
                        u32 p1_segID = s->segID_of_px[p1];
                        float dist_squared = color_dif_magnitude_squared(color_dif_between(seg_color, s->color_of_segment[p1_segID]));
                        if (dist_squared < min_dist_squared) {
                            min_dist_squared = dist_squared;
                            best_neighbour = p1_segID;
                        }
                    }    
                } else if (p2_in_image && s->segID_of_px[p2] == i) { //si p2 est dans la forme
                        tmp = d_cen;
                        d_cen = d_mov;            //direction du centre est maintenant celle du mouvement
                        d_mov = (tmp + 2) & 3;    //direction du mouvement prend la direction opposée du centre
                } else {
                    if (p2_in_image && s->segID_of_px[p2] != DISCONTINUITY_PX) {
                        u32 p2_segID = s->segID_of_px[p2];
                        float dist_squared = color_dif_magnitude_squared(color_dif_between(seg_color, s->color_of_segment[p2_segID]));
                        if (dist_squared < min_dist_squared) {
                            min_dist_squared = dist_squared;
                            best_neighbour = p2_segID;
                        }
                    }  
                }
                VtoV_neighbour(cur, &cur, d_mov);
            }

            if (min_dist_squared < merging_threshold*merging_threshold) {
                merging_occured = true;
                u32 segmin, segmax;
                if (i < best_neighbour) {
                    segmin = i;
                    segmax = best_neighbour;
                } else {
                    segmin = best_neighbour;
                    segmax = i;
                }
                float t = (float)s->px_of_segment[i]->count / (float)(s->px_of_segment[i]->count + s->px_of_segment[best_neighbour]->count);
                s->color_of_segment[segmin] = 
                                  (Color){t*s->color_of_segment[i].L   + (1-t)*s->color_of_segment[best_neighbour].L,
                                          t*s->color_of_segment[i].a + (1-t)*s->color_of_segment[best_neighbour].a,
                                          t*s->color_of_segment[i].b  + (1-t)*s->color_of_segment[best_neighbour].b };

                vec_concatenate(s->px_of_segment[segmin], s->px_of_segment[segmax]);
                for (int i = 0; i < s->px_of_segment[segmax]->count; ++i) {
                    s->segID_of_px[*(Pixel*)vec_get_element(s->px_of_segment[segmax], i)] = segmin;
                }
                free_vec(s->px_of_segment[segmax]);
                s->px_of_segment[segmax] = NULL;
                --s->nb_segments;
            }
        }
    }       
    //--On comprime le tableau ie on ramène tous les identifiants des segments entre 0 et s->nb_segments-1
    u32 to_write = 0; 
    u32 to_read = 0;
    while (to_write != s->nb_segments) { //invarient: pour tout i dans [0, to_write-1], s->px_of_segment[i] != NULL
        if (s->px_of_segment[to_read] != NULL) {
            s->px_of_segment[to_write] = s->px_of_segment[to_read];
            s->color_of_segment[to_write] = s->color_of_segment[to_read];
            for (int i = 0; i < s->px_of_segment[to_read]->count; ++i) {
                s->segID_of_px[*(Pixel*)vec_get_element(s->px_of_segment[to_read], i)] = to_write;
            }
            ++to_write;
        }
        ++to_read;
    }
    s->px_of_segment = realloc(s->px_of_segment, sizeof(vec*) * s->nb_segments);
    s->color_of_segment = realloc(s->color_of_segment, sizeof(Color) * s->nb_segments);
}



typedef struct {
    u32 u; 
    u32 v;
}segment_pair;

typedef struct {
    segment_pair pair;
    float score;
}pair_and_score;


typedef struct {
    u32 to;
    float w;
    int rev; //index of the reverse arc in r->adj[to]
}arc;


//undirected weighted graph of segments
typedef struct {
    int size;
    vec** adj;    //adj[u] is a vec* of arc
}segment_graph; 
             

typedef struct {
    int nb_discontinuity; //f_A(u, v): number of discontinuity pixels separating u from v
    Pixel last_discontinuity_px; //used for not incrementing over than 1 time nb_discontinuity for a given discontinuity px
}pair_discontinuity;

#define SIGMA 5     //range of a discontinuity px for computing f_A(u, v)
#define TAU_A 0.25f //A = (u,v) such as nb_discontinuity[u,v] > TAU_A*nb_shared_boundaries[u,v]
#define FLOAT_MARGIN 0.00001f

void register_boundary(segment_graph* G, Hash_table* shared_boundaries, segmentation* s, u32 u, u32 v) {
    if (u == v || u == DISCONTINUITY_PX || v == DISCONTINUITY_PX) return;
    segment_pair sp = u < v ? (segment_pair){u, v} : (segment_pair){v, u};
    int* nb = ht_get_val(shared_boundaries, &sp);
    if (nb == NULL) {
        int initial = 1;
        ht_add(shared_boundaries, &sp, &initial);
        float w = expf(-0.01f * color_dif_magnitude_squared(
                        color_dif_between(s->color_of_segment[u], s->color_of_segment[v])));
        if (w <= FLOAT_MARGIN){
            printf("{%f, %f, %f}u: %d, {%f, %f, %f}v: %d, w == %f <= FLOAT_MARGIN\n",s->color_of_segment[u].a,s->color_of_segment[u].b,s->color_of_segment[u].L, u, s->color_of_segment[v].a,s->color_of_segment[v].b,s->color_of_segment[v].L, v, w);
        }    
        arc a = (arc){.to = v, .w = w, .rev = G->adj[v]->count};
        vec_add(G->adj[u], &a);

        a.to = u;
        a.rev = G->adj[u]->count - 1;
        vec_add(G->adj[v], &a);
    } else {
        *nb = *nb + 1;
    }
}
Straight_Dir opp(Straight_Dir d) { return d == ENUM_END ? ENUM_END : (d + 2) & 3; }
int collect_segmentIDs(segmentation* s, Pixel px, Straight_Dir d1, Straight_Dir d2, u32 out[SIGMA]) {
    int n = 0;
    Pixel cur = px;
    for (int i = 0; i < SIGMA; ++i) {
        if (!PtoP_neighbour(cur, &cur, d1)) break;
        if (d2 != ENUM_END && !PtoP_neighbour(cur, &cur, d2)) break;
        u32 id = s->segID_of_px[cur];
        if (id == DISCONTINUITY_PX) continue;
        bool found = false;
        for (int j = 0; j < n && !found; ++j) found = (out[j] == id);
        if (!found) out[n++] = id;
    }
    return n;
}
void increment_nb_discontinuity(segmentation* s, Pixel px, Hash_table* discontinuities, Straight_Dir d1, Straight_Dir d2) {
    u32 plus[SIGMA], minus[SIGMA];
    int np = collect_segmentIDs(s, px, d1, d2, plus);
    int nm = collect_segmentIDs(s, px, opp(d1), opp(d2), minus);
    for (int k = 0; k < np; ++k) {
        for (int p = 0; p < nm; ++p) {
            if (plus[k] != minus[p]) {
                segment_pair sp = plus[k] < minus[p] ? (segment_pair){plus[k], minus[p]} : (segment_pair){minus[p], plus[k]};
                //printf("sp.u: %d, sp.v: %d\n", sp.u, sp.v);
                if (sp.u == 15 && sp.v == 20) {
                    printf("Culprit: (%d, %d), d1: %d, d2: %d", px % WIDTH, px / WIDTH, d1, d2);
                }
                pair_discontinuity* disc = ht_get_val(discontinuities, &sp);
                if (disc == NULL) {
                    pair_discontinuity initial = (pair_discontinuity) {.nb_discontinuity = 1, .last_discontinuity_px = px};
                    ht_add(discontinuities, &sp, &initial);
                } else if (disc->last_discontinuity_px != px){
                    disc->nb_discontinuity++;
                    disc->last_discontinuity_px = px;
                }
            }
        }
    }
}

//sorts the elements sp[a...b] (a and b are indices) in ascending order
void merge_sort(pair_and_score* sp, pair_and_score* aux, int a, int b) {
    if (a >= b) return;
    int mid = (a+b)/2;
    merge_sort(sp, aux, a, mid);
    merge_sort(sp, aux, mid+1, b);

    int l = a, r = mid+1;
    for (int i = a; i <= b; ++i) {
        if (r == b+1 || (l != mid+1 && sp[r].score > sp[l].score)) {
            aux[i] = sp[l];
            ++l;
        } else {
            aux[i] = sp[r];
            ++r;
        }
    }
    memcpy(sp + a, aux + a, sizeof(pair_and_score) * (b - a + 1)); //copy aux into sp
}
//returns if there is a path from u to v in r by doing DFS, stack and seen points to allocated memory of r->size*sizeof(u32) bytes
bool is_reachable(segment_graph* G, u32 u, u32 v, u32* stack, bool* seen) {
    for (int i = 0; i < G->size; ++i) {
        seen[i] = false;
    }
    int size = 1;
    stack[0] = u;
    while(size > 0) {
        u32 s = stack[--size];
        if (s == v) return true;
        for (int i = 0; i < G->adj[s]->count; ++i) {
            arc a = *(arc*)vec_get_element(G->adj[s], i);
            if (seen[a.to]) continue;
            seen[a.to] = true;
            stack[size++] = a.to;
        }
    }

    return false;
}

float Dinic_dfs(int u, int t, float pushed, int* level, vec** adj, int* ptr) {
    if (u == t) return pushed;
    if (pushed <= FLOAT_MARGIN) return 0.f;
    float total = 0.f;
    for (int* cid = &ptr[u]; *cid < adj[u]->count; (*cid)++) {
        arc* e = (arc*)vec_get_element(adj[u], *cid);
        if (level[e->to] != level[u] + 1 || e->w <= FLOAT_MARGIN) continue;
        float tr = Dinic_dfs(e->to, t, (pushed - total < e->w ? pushed - total: e->w), level, adj, ptr);
        if (tr <= FLOAT_MARGIN) continue;
        e->w -= tr;
        ((arc*)vec_get_element(adj[e->to], e->rev))->w += tr;
        total += tr;
        if (pushed - total <= FLOAT_MARGIN) return total;
    }
    return total;

}
//level and ptr must point towards allocated memory of G->size * sizeof(int)
//modifies the segment_graph G such as s and t are disconnected and the sum 
//of removed edge weights is minimal
//s and t must be distinct
void apply_Dinic(segment_graph* G, u32 s, u32 t, int* level, int* ptr) {
    //Copy G into adj
    vec** adj = malloc(sizeof(vec*) * G->size);
    for (int i = 0; i < G->size; ++i) {
        adj[i] = empty_vec_choose_capacity(sizeof(arc), NULL, G->adj[i]->count);
        for (int j = 0; j < G->adj[i]->count; ++j) {
            vec_add(adj[i], (arc*)vec_get_element(G->adj[i], j));
        }
    }

    while(true) {
        //1.Fills level by BFS 
        queue* q = empty_queue();
        queue_element(q, s);
        for (int i = 0; i < G->size; ++i) level[i] = -1;
        level[s] = 0;
        while (!is_queue_empty(q)) {
            u32 u = dequeue_element(q);
 
            for (int i = 0; i < adj[u]->count; ++i) {
                arc a = *(arc*)vec_get_element(adj[u], i);
                if (level[a.to] != -1) continue; //if it was already seen
                if (a.w <= FLOAT_MARGIN) continue; 
                level[a.to] = level[u] + 1;
                queue_element(q, a.to);
            }
        }
        free_queue(q);
        if (level[t] == -1) break; //t wasn't reached

        //2.DFS
        for (int i = 0; i < G->size; ++i) ptr[i] = 0;
        float pushed = 0;
        do {
            pushed = Dinic_dfs(s, t, INFINITY, level, adj, ptr);
        } while(pushed > FLOAT_MARGIN);
    }
    
    //v wasn't reached, we cut all the edges (u, v) such as level[u] == -1 and level[v] != -1 or the contrary
    for (u32 u = 0; u < G->size; ++u) {
        for (int j = 0; j < G->adj[u]->count; ++j) {
            arc e = *(arc*)vec_get_element(G->adj[u], j);
            if ((level[u] == -1 && level[e.to] != -1) || (level[u] != -1 && level[e.to] == -1)) { 
                vec_remove_by_index(G->adj[u], j);
                if (j < G->adj[u]->count) {
                    arc new = *(arc*)vec_get_element(G->adj[u], j);
                    ((arc*)vec_get_element(G->adj[new.to], new.rev))->rev = j; 
                } 
                vec_remove_by_index(G->adj[e.to], e.rev);
                if (e.rev < G->adj[e.to]->count) {
                    arc new = *(arc*)vec_get_element(G->adj[e.to], e.rev);
                    ((arc*)vec_get_element(G->adj[new.to], new.rev))->rev = e.rev;  
                }
                --j;
            } 
        }
    }
    for (int i = 0; i < G->size; ++i) {
        free_vec(adj[i]);
    }
    free(adj);
}

typedef struct {
    vec* component;
}connected_components;
void free_component(void* c) {
    vec* p = *((vec**)c);
    free_vec(p);
}
//the segment of minimum ID in connected component is guaranteed to be at index 0 in its vec
connected_components* get_connected_components(segment_graph* G, bool* was_stacked, u32* stack) {
    connected_components* cc = malloc(sizeof(connected_components));
    cc->component = empty_vec(sizeof(vec*), free_component);
    
    for (int i = 0; i < G->size; ++i) was_stacked[i] = false;

    for (u32 s = 0; s < G->size; ++s) {
        if (was_stacked[s]) continue;
        int size = 1;
        stack[0] = s;
        was_stacked[s] = true;

        vec** ve = vec_push(cc->component);
        *ve = empty_vec(sizeof(u32), NULL);
        vec_add(*ve, &s);

        while (size > 0) {
            u32 u = stack[--size];
            for (int i = 0; i < G->adj[u]->count; ++i) {
                arc a = *(arc*)vec_get_element(G->adj[u], i);
                if (was_stacked[a.to]) continue;
                stack[size++] = a.to;
                vec_add(*ve, &a.to);
                was_stacked[a.to] = true;
            }
        }
    }
    return cc;
}

//constructs the adjacency weighted graph r and the discontinuity relation between segments of S0
//solves the min cut problem then return the connected components, forming S the final segmentation
segmentation* final_segmentation(segmentation* S0, vec* discontinuity_px) {
    segment_graph* G = malloc(sizeof(segment_graph));
    G->adj = malloc(sizeof(vec*) * S0->nb_segments);
    G->size = S0->nb_segments;

    for (u32 i = 0; i < S0->nb_segments; ++i) {
        G->adj[i] = empty_vec(sizeof(arc), NULL);
    }
    //for both, key is segment_pair (u, v) such as u < v
    Hash_table* discontinuities = empty_ht(sizeof(segment_pair), sizeof(pair_discontinuity));
    Hash_table* shared_boundaries = empty_ht(sizeof(segment_pair), sizeof(int)); //the number of shared_boundaires for any p1 in u with any p2 in v


    //----1.On construit G et shared_boundaries et itérant sur tous les pixels
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            u32 a = S0->segID_of_px[y*WIDTH + x];
            if (x + 1 < WIDTH) register_boundary(G, shared_boundaries, S0, a, S0->segID_of_px[y*WIDTH + x+1]);
            if (y + 1 < HEIGHT) register_boundary(G, shared_boundaries, S0, a, S0->segID_of_px[(y+1)*WIDTH + x]);
        }
    }
    printf("Etape 1 terminée. \n");

    //----2.On construit discontinuities en itérant sur tous les pixels de discontinuité
    for (int i = 0; i < discontinuity_px->count; ++i) {
        Pixel px = *(Pixel*)vec_get_element(discontinuity_px, i);
        increment_nb_discontinuity(S0, px, discontinuities, TOP, ENUM_END);
        increment_nb_discontinuity(S0, px, discontinuities, TOP, RIGHT);
        increment_nb_discontinuity(S0, px, discontinuities, RIGHT, ENUM_END);
        increment_nb_discontinuity(S0, px, discontinuities, BOTTOM, RIGHT);
    }
    printf("Etape 2 terminée. discontinuities->count: %d\n", discontinuities->count);


    //----3.On construit la relation de discontinuité sous forme de liste de couples associé à un score: la différence de couleur
    pair_and_score* discontinuity_relation = malloc(sizeof(pair_and_score)*discontinuities->count); //discontinuities->count majore le cardinal de la relation
    Ht_Iterator* it = ht_first(discontinuities);
    int idx = 0; //indice du prochain pair_and_score à initialiser
    if (it != NULL) {
        do {
            pair_discontinuity* disc = (pair_discontinuity*)it->entry->val;
            int* nb_shared_boundaries = ht_get_val(shared_boundaries, it->entry->key);
            if (nb_shared_boundaries == NULL || (float)disc->nb_discontinuity > TAU_A * ((float)*nb_shared_boundaries)) {
                segment_pair* p = (segment_pair*)it->entry->key;
                //printf("u: %d, v: %d\n", p->u, p->v);
                if (p->u == 15 && p->v == 20) {
                    printf("FOUND disc->nb_discontinuity: %d, ", disc->nb_discontinuity);
                    if (nb_shared_boundaries == NULL) {
                        printf("nb_shared_boundaries == NULL\n");
                    } else {
                        printf("nb_shared_boundaries: %d\n", nb_shared_boundaries);
                    }
                }
                discontinuity_relation[idx].pair = *p;
                discontinuity_relation[idx].score = color_dif_magnitude_squared(color_dif_between(S0->color_of_segment[p->u], S0->color_of_segment[p->v]));
                ++idx;

                // if(nb_shared_boundaries != NULL) { //si c'est une arête du graphe, on la retire directement
                //     for (int i = 0; i < G->adj[p->u]->count; ++i) {
                //         if (((arc*)vec_get_element(G->adj[p->u], i))->to == p->v) {
                //             vec_remove_by_index(G->adj[p->u], i);
                //             if (i < G->adj[p->u]->count) {
                //                 arc new = *(arc*)vec_get_element(G->adj[p->u], i);
                //                 ((arc*)vec_get_element(G->adj[new.to], new.rev))->rev = i; 
                //             } 
                //             break;
                //         }
                //     }
                //     for (int i = 0; i < G->adj[p->v]->count; ++i) {
                //         if (((arc*)vec_get_element(G->adj[p->v], i))->to == p->u) {
                //             vec_remove_by_index(G->adj[p->v], i);
                //             if (i < G->adj[p->v]->count) {
                //                 arc new = *(arc*)vec_get_element(G->adj[p->v], i);
                //                 ((arc*)vec_get_element(G->adj[new.to], new.rev))->rev = i; 
                //             } 
                //             break;
                //         }
                //     }
                // }   
            }
        }while(ht_next(discontinuities, it));
    }
    free(it);
    free_ht(discontinuities);
    free_ht(shared_boundaries);
    int discontinuity_relation_size = idx;
    printf("Etape 3 terminée. discontinuity_relation_size: %d\n", discontinuity_relation_size);;
    
    
    //----4.On trie la relation de discontinuité par score croissants
    pair_and_score* aux = malloc(sizeof(pair_and_score)*discontinuity_relation_size);
    merge_sort(discontinuity_relation, aux, 0, discontinuity_relation_size - 1);
    free(aux);
    printf("Etape 4 (tri) terminée\n");


    //----5.On applique l'algorithme Dinic pour successivement trouver un min_cut pour chaque paire
    int* arr1 = malloc(sizeof(int) * G->size);
    int* arr2 = malloc(sizeof(int) * G->size);
    //printf("\n\n");
    for (int i = 0; i < discontinuity_relation_size; ++i) {
        //printf("i: %d score: %f\n", i, discontinuity_relation[i].score);
        // printf("u: %d, v: %d", discontinuity_relation[i].pair.u, discontinuity_relation[i].pair.v);
        // fflush(stdout);
        if (is_reachable(G, discontinuity_relation[i].pair.u, discontinuity_relation[i].pair.v, (u32*)arr1, (bool*)arr2)) {
            // printf(", x_u: %d, x_v: %d",
            //             *(Pixel*)vec_get_element(S0->px_of_segment[discontinuity_relation[i].pair.u], 0) % WIDTH,
            //             *(Pixel*)vec_get_element(S0->px_of_segment[discontinuity_relation[i].pair.v], 0) % WIDTH);
            // printf(", y_u: %d, y_v: %d\n",
            //             *(Pixel*)vec_get_element(S0->px_of_segment[discontinuity_relation[i].pair.u], 0) / WIDTH,
            //             *(Pixel*)vec_get_element(S0->px_of_segment[discontinuity_relation[i].pair.v], 0) / WIDTH);
            // fflush(stdout);
            apply_Dinic(G, discontinuity_relation[i].pair.u, discontinuity_relation[i].pair.v, arr1, arr2);
        } else {
            // printf("\n");
            // fflush(stdout);
        }
        
    }   
    printf("Etape 5 (multi_cut) terminée.\n");


    connected_components* cc = get_connected_components(G, (bool*)arr1, (u32*)arr2);
    free(arr1);
    free(arr2);

    //----6.Après avoir récupérer les composantes connexes de G, on construit la partition finale
    segmentation* S = malloc(sizeof(segmentation));
    S->nb_segments = cc->component->count;
    S->segID_of_px = malloc(sizeof(u32) * NB_PX);
    S->px_of_segment = malloc(sizeof(vec*) * S->nb_segments);
    S->color_of_segment = malloc(sizeof(Color) * S->nb_segments);
    
    for (int i = 0; i < cc->component->count; ++i) {
        S->px_of_segment[i] = empty_vec(sizeof(Pixel), NULL);
        vec* c = *(vec**)vec_get_element(cc->component, i);

        S->color_of_segment[i] = (Color){0.f, 0.f, 0.f};

        for (int s = 0; s < c->count; ++s) {
            u32 seg = *(u32*)vec_get_element(c, s);
            vec_concatenate(S->px_of_segment[i], S0->px_of_segment[seg]);

            for (int k = 0; k < S0->px_of_segment[seg]->count; ++k) {
                S->segID_of_px[*(Pixel*)vec_get_element(S0->px_of_segment[seg], k)] = i;

                float multiplier = (float)S0->px_of_segment[seg]->count;
                S->color_of_segment[i].L += S0->color_of_segment[seg].L * multiplier;
                S->color_of_segment[i].a += S0->color_of_segment[seg].a * multiplier;
                S->color_of_segment[i].b += S0->color_of_segment[seg].b * multiplier;
            }
        }
        S->color_of_segment[i].L /= (float)S->px_of_segment[i]->count;
        S->color_of_segment[i].a /= (float)S->px_of_segment[i]->count;
        S->color_of_segment[i].b /= (float)S->px_of_segment[i]->count;
    }
    for (int i = 0; i < discontinuity_px->count; ++i) {
        S->segID_of_px[*(Pixel*)vec_get_element(discontinuity_px, i)] = DISCONTINUITY_PX;
    }

    free_vec(cc->component);
    free(cc);

    free(discontinuity_relation);
    for (int i = 0; i < S0->nb_segments; ++i) {
        free_vec(G->adj[i]);
    }
    free(G->adj);
    free(G);

    return S;
}



