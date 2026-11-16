#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "hash_table.h"


Hash_table* empty_ht(int key_stride, int val_stride) {
    Hash_table* d = malloc(sizeof(Hash_table));
    d->key_stride = key_stride;
    d->val_stride = val_stride;
    d->nb_slots = 8;
    d->count = 0;
    d->slots = malloc(sizeof(Entry*) * d->nb_slots);
    for (int i = 0; i < d->nb_slots; ++i) {
        d->slots[i] = NULL;
    }
    return d;
}

Hash_table* copy_ht(Hash_table* ht) {
    Hash_table* res = malloc(sizeof(Hash_table));
    res->count = ht->count;
    res->key_stride = ht->key_stride;
    res->val_stride = ht->val_stride;
    res->nb_slots = ht->nb_slots;
    res->slots = malloc(sizeof(Entry*) * res->nb_slots);
    for (int i = 0; i < res->nb_slots; ++i) {
        Entry* cur_ht = ht->slots[i];
        Entry** cur_res = &res->slots[i];
        *cur_res = NULL;
        while (cur_ht != NULL) {
            *cur_res = malloc(sizeof(Entry));
            (*cur_res)->key = malloc(res->key_stride);
            (*cur_res)->val = malloc(res->val_stride);
            memcpy((*cur_res)->key, cur_ht->key, res->key_stride);
            memcpy((*cur_res)->val, cur_ht->val, res->val_stride);
            (*cur_res)->next = NULL;

            cur_ht = cur_ht->next;
            cur_res = &((*cur_res)->next);
        }
    }
    return res;
}

void free_ht(Hash_table* ht) {
    for (int i = 0; i < ht->nb_slots; ++i) {
        Entry* cur = ht->slots[i];
        while (cur != NULL) {
            Entry* tmp = cur;
            cur = cur->next;

            free(tmp->key);
            free(tmp->val);
            free(tmp);
        }
    }
    free(ht->slots);
    free(ht);
}

unsigned long long ht_hash(Hash_table* ht, void* key) {
    unsigned long long h = 0;
    memcpy(&h, key, ht->key_stride);

    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53L;
    h ^= h >> 33;
    return h;
}

void ht_print(Hash_table* ht) {
    for (int i = 0; i < ht->nb_slots; ++i) {
        printf("slot #%d: ", i);
        Entry* cur = ht->slots[i];
        while (cur != NULL) {
            printf("(");
            for (int b = 0; b < ht->key_stride; ++b) {
                printf("%02X", *(cur->key + b));
            }
            printf(", ");
            for (int b = 0; b < ht->val_stride; ++b) {
                printf("%02X", *(cur->val + b));
            }
            printf(")");
            if (cur->next != NULL) printf("->");
            cur = cur->next;
                fflush(stdout);

        }
        printf("\n");
    }
}

void ht_resize(Hash_table* ht, int new_nb_slots) {
    Entry** new_slots = malloc(sizeof(Entry*) * new_nb_slots);
    for (int i = 0; i < new_nb_slots; ++i) {
        new_slots[i] = NULL;
    }
    for (int i = 0; i < ht->nb_slots; ++i) {
        Entry* cur = ht->slots[i];
        while (cur != NULL) {
            Entry* tmp = cur->next;
            //----- Move cur to new_slots -----//
            Entry** to_add = &new_slots[ht_hash(ht, cur->key) % new_nb_slots];
            while (*to_add != NULL) {
                to_add = &((*to_add)->next);
            }
            cur->next = NULL;
            *to_add = cur;

            cur = tmp;
        }
    }
    free(ht->slots);
    ht->slots = new_slots;
    ht->nb_slots = new_nb_slots;
}
//returns a pointer to the value just added
void* ht_add(Hash_table* ht, void* key, void* val) {
    Entry** to_add = &ht->slots[ht_hash(ht, key) % ht->nb_slots];
    while (*to_add != NULL) {
        if (!memcmp(key, (*to_add)->key, ht->key_stride)) { //if key is already present, just replace its value
            memcpy((*to_add)->val, val, ht->val_stride);
            return (*to_add)->val;
        }
        to_add = &((*to_add)->next);
    }
    *to_add = malloc(sizeof(Entry));
    (*to_add)->next = NULL;
    (*to_add)->key = malloc(ht->key_stride);
    (*to_add)->val = malloc(ht->val_stride);
    memcpy((*to_add)->key, key, ht->key_stride);
    memcpy((*to_add)->val, val, ht->val_stride);

    void* res = (*to_add)->val;
    if (++ht->count > ht->nb_slots) ht_resize(ht, 2 * ht->nb_slots);
    return res;
}

//returns a pointer to the val corresponding to that key in the hash table, NULL if "key" isn't in the table
void* ht_get_val(Hash_table* ht, void* key) {
    Entry* cur = ht->slots[ht_hash(ht, key) % ht->nb_slots];
    while (cur != NULL) {
        if (!memcmp(key, cur->key, ht->key_stride)) { 
            return cur->val;
        }
        cur = cur->next;
    }
    return NULL;
}

//if the key is in the table, remove it
void ht_remove(Hash_table* ht, void* key) {
    Entry** to_remove = &ht->slots[ht_hash(ht, key) % ht->nb_slots];
    while (*to_remove != NULL) {
        if (!memcmp(key, (*to_remove)->key, ht->key_stride)) { 
            Entry* next = (*to_remove)->next;

            free((*to_remove)->key);
            free((*to_remove)->val);
            free(*to_remove);
            *to_remove = next;

            if (--ht->count < ht->nb_slots / 4 && ht->nb_slots > 8) ht_resize(ht, ht->nb_slots / 2);
            return;
        }
        to_remove = &((*to_remove)->next);
    }
}


bool ht_contains(Hash_table* ht, void* key) {
    Entry* cur = ht->slots[ht_hash(ht, key) % ht->nb_slots];
    while (cur != NULL) {
        if (!memcmp(key, cur->key, ht->key_stride)) { 
            return true;
        }
        cur = cur->next;
    }
    return false;
}


bool ht_next(Hash_table* ht, Ht_Iterator* it) {
    if (it->entry->next == NULL) {
        for (int i = it->slot + 1; i < ht->nb_slots; ++i) {
            if (ht->slots[i] != NULL) {
                it->entry = ht->slots[i];
                it->slot = i;
                return true;
            }
        }
        return false;
    } else {
        it->entry = it->entry->next;
        return true;
    }
}

Ht_Iterator* ht_first(Hash_table* ht) {
    for (int i = 0; i < ht->nb_slots; ++i) {
        if (ht->slots[i] != NULL) {
            Ht_Iterator* it = malloc(sizeof(Ht_Iterator));
            it->entry = ht->slots[i];
            it->slot = i;
            return it;
        }
    }
    return NULL;
}
