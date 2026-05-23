#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include "memory_manager.h"
/*----------------------------------------------------------------------------*/
typedef struct tag_mem_chunk
{
	int mc_free_chunks;
	struct tag_mem_chunk *mc_next;
} mem_chunk;
/*----------------------------------------------------------------------------*/
typedef mem_chunk *mem_chunk_t;
/*----------------------------------------------------------------------------*/
typedef struct memory_pool
{
	u_char *mp_startptr;      /* start pointer */
	mem_chunk_t mp_freeptr;   /* pointer to the start memory chunk */
	int mp_free_chunks;       /* number of total free chunks */
	int mp_total_chunks;       /* number of total  chunks */
	int mp_chunk_size;        /* chunk size in bytes */
	int mp_type;
	
} memory_pool;
/*----------------------------------------------------------------------------*/
memory_pool_t 
CreateMemoryPool(int chunk_size, size_t total_size)
{	
	memory_pool_t mp;

	if (chunk_size < sizeof(mem_chunk)) {
		fprintf(stderr,"The chunk size should be larger than %lu. current: %d\n", 
				sizeof(mem_chunk), chunk_size);
		return NULL;
	}
	if (chunk_size % 4 != 0) {
		fprintf(stderr,"The chunk size should be multiply of 4!\n");
		return NULL;
	}

	if ((mp = calloc(1, sizeof(memory_pool))) == NULL) {
		perror("calloc failed");
		exit(0);
	}
	mp->mp_type = 0;
	mp->mp_chunk_size = chunk_size;
	mp->mp_free_chunks = ((total_size + (chunk_size -1))/chunk_size);
	mp->mp_total_chunks = mp->mp_free_chunks;
	total_size = chunk_size * ((size_t)mp->mp_free_chunks);

	/* allocate the big memory chunk */
	int res = posix_memalign((void **)&mp->mp_startptr, getpagesize(), total_size);
	if (res != 0) {
		fprintf(stderr,"posix_memalign failed, size=%ld\n", total_size);
		assert(0);
		free(mp);
		return (NULL);
	}

	/* try mlock only for superuser */
	if (geteuid() == 0) {
		if (mlock(mp->mp_startptr, total_size) < 0) 
			fprintf(stderr,"m_lock failed, size=%ld\n", total_size);
	}
	// mlock() --> locks a range of the calling process's virtual memory into RAM, preventing it from
	// being paged out ( == swapped to disk).
	mp->mp_freeptr = (mem_chunk_t)mp->mp_startptr;
	mp->mp_freeptr->mc_free_chunks = mp->mp_free_chunks;
	mp->mp_freeptr->mc_next = NULL;

	return mp;
}
/*----------------------------------------------------------------------------*/
void *
AllocChunkInMemoryPool(memory_pool_t mp)
{
	mem_chunk_t p = mp->mp_freeptr;
	
	if (mp->mp_free_chunks == 0) 
		return (NULL);
	assert(p->mc_free_chunks > 0 && p->mc_free_chunks <= p->mc_free_chunks);
	
	p->mc_free_chunks--;
	mp->mp_free_chunks--;
	if (p->mc_free_chunks) {
		/* move right by one chunk */
		mp->mp_freeptr = (mem_chunk_t)((u_char *)p + mp->mp_chunk_size);
		mp->mp_freeptr->mc_free_chunks = p->mc_free_chunks;
		mp->mp_freeptr->mc_next = p->mc_next;
	}
	else {
		mp->mp_freeptr = p->mc_next;
	}

	return p;
}
/*----------------------------------------------------------------------------*/
void
FreeChunkInMemoryPool(memory_pool_t mp, void *p)
{
	mem_chunk_t mcp = (mem_chunk_t)p;
	assert(((u_char *)p - mp->mp_startptr) % mp->mp_chunk_size == 0);
	mcp->mc_free_chunks = 1;
	mcp->mc_next = mp->mp_freeptr;
	mp->mp_freeptr = mcp;
	mp->mp_free_chunks++;
}
/*----------------------------------------------------------------------------*/
void
DestroyMemoryPool(memory_pool_t mp)
{
	free(mp->mp_startptr);
	free(mp);
}
/*----------------------------------------------------------------------------*/
