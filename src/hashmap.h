#ifndef HASHMAP_H_
#define HASHMAP_H_
#include "linked_list.h"
#include <stdbool.h>

DEFSTRUCT(hash_entry);
DEFSTRUCT(hashmap);

#define HASHMAP_ENTRY_LENGTH 1024

struct hash_entry {
	int key;
	void *value;
};
struct hash_map {
	linked_list entries[HASHMAP_ENTRY_LENGTH];
};

hash_map *hm_new();
void hm_put(hash_map *map, int key, void *value);
void *hm_get(hash_map *map, int key);
bool hm_has(hash_map *map, int key);
#endif