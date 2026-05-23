#ifndef memory_manager_H
#define memory_manager_H
/*----------------------------------------------------------------------------*/
struct memory_pool;
typedef struct memory_pool* memory_pool_t;
/* create a memory pool with a chunk size and total size
   an return the pointer to the memory pool */
memory_pool_t CreateMemoryPool(int chunk_size, size_t total_size);
/*----------------------------------------------------------------------------*/
/* allocate one chunk */
void *
AllocChunkInMemoryPool(memory_pool_t mp);

/* free one chunk */
void
FreeChunkInMemoryPool(memory_pool_t mp, void *p);

/* destroy the memory pool */
void
DestroyMemoryPool(memory_pool_t mp);

/*----------------------------------------------------------------------------*/

#endif /* memory_manager_H */
