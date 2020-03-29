#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void slab_t;

slab_t *slab_create(uint32_t numelems, size_t elemsize);
void *slab_alloc(slab_t *slab);
void slab_free(slab_t *slab, void *ptr);
void slab_destroy(slab_t *slab);
uint32_t slab_get_avail_elems(slab_t *slab);
uint32_t slab_get_total_elems(slab_t *slab);
int32_t slab_get_slab_position(slab_t *slab, void *ptr);
void *slab_get_ptr_from_position(slab_t *slab, uint32_t pos);
bool slab_is_taken(slab_t *slab, uint32_t idx);
