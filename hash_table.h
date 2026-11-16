#include <stdbool.h>

typedef struct Entry Entry;
typedef struct Entry{
    unsigned char* key;
    unsigned char* val;
    Entry* next;
}Entry;

typedef struct {
    int key_stride; //number of bytes per key, must be <= 8
    int val_stride; //number of bytes per value
    Entry** slots;
    int nb_slots;
    int count;
}Hash_table;

Hash_table* empty_ht(int key_stride, int val_stride);

Hash_table* copy_ht(Hash_table* ht);

void free_ht(Hash_table* ht);

unsigned long long ht_hash(Hash_table* ht, void* key);

void ht_print(Hash_table* ht);

void ht_resize(Hash_table* ht, int new_nb_slots);

void* ht_add(Hash_table* ht, void* key, void* val);

//returns a pointer to the val corresponding to that key in the hash table, NULL if "key" isn't in the table
void* ht_get_val(Hash_table* ht, void* key);

//if the key is in the table, remove it
void ht_remove(Hash_table* ht, void* key);

bool ht_contains(Hash_table* ht, void* key);


typedef struct {
    Entry* entry;
    int slot;
}Ht_Iterator;

//returns if there is a following element in the table (i.e it->entry isn't the last one)
//if so modifies "it" 
bool ht_next(Hash_table* ht, Ht_Iterator* it);

//returns a Ht_Iterator pointing to the first entry of ht
Ht_Iterator* ht_first(Hash_table* ht);



