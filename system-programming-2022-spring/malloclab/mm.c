/*
 * Dynamic memory allocator implemented by J.W.Choi.
 *
 * - Segregated free list (LIFO)
 * - Boundary tag coalescing
 * - First fit
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/* Debug mode */
// #define DEBUG

/* Basic constants and macros */
#define WSIZE 4
#define DSIZE 8
#define MIN_BLK_SIZE 24 /* Header (4B) + Next pointer (8B) + Prev pointer (8B) + Footer(4B) */
#define CHUNKSIZE (1 << 10)
#define INITSIZE (1 << 8)

#define MAX(x, y) ((x) > (y)) ? (x) : (y)
#define MIN(x, y) ((x) < (y)) ? (x) : (y)

/* Double word alignment */
#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size | alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

/* Segregated free list macros */
#define NEXT_FREEP(bp) (*(char **)bp)
#define PREV_FREEP(bp) (*(char **)(bp + DSIZE))

#define NUM_SEG_LISTS 20
#define SEG_SIZE_OFFSET 4 /* Determined by the first seg list's byte size range */

/* Segregated free list */
static char *seglists[NUM_SEG_LISTS];

/*
 * Helper Functions Prototypes
 */

/* Extends the heap with a new free block */
static void *extend_heap(size_t words);

/* Merge freed block with any adjacent free blocks in constant time */
static void *coalesce(void *bp);

/* Find a free block that fits the given size */
static void *find_fit(size_t asize);

/* Place the block on the given free block */
static void place(void *bp, size_t asize);

/* Get index of segregated list that fits with the given size */
static int get_seglistn(size_t size);

/* Insert the given free block to the segregated list */
static void insert_seglist(void *bp);

/* Remove the given free block of the segregated list */
static void remove_seglist(void *bp);

/*
 * Debug Function Portotpyes & Global Variables
 */
#ifdef DEBUG
static int malloc_cnt = 0;
static int free_cnt = 0;
static int realloc_cnt = 0;

static void print_stat(char *context, int count);
static void print_heap();
static void print_flist();
#endif
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    char *heap_listp;

    /* Initialize seg lists with NULL */
    for (int i = 0; i < NUM_SEG_LISTS; i++)
        seglists[i] = NULL;

    if ((heap_listp = (char *)mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */

    /* Extend the empty heap with a free block of INITSIZE bytes */
    if (extend_heap(INITSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

/*
 * mm_malloc
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs */
    asize = MAX(ALIGN(size + DSIZE), MIN_BLK_SIZE);

    /* Search a fit block in the free list */
    if ((bp = find_fit(asize)) == NULL)
    {
        /* No fit found. Get more memory and place the block on it */
        extendsize = MAX(asize, CHUNKSIZE);
        if (!(bp = extend_heap(extendsize / WSIZE)))
            return NULL;
    }

    place(bp, asize);

#ifdef DEBUG
    print_stat("malloc", malloc_cnt++);
#endif

    return bp;
}

/*
 * mm_free
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);

#ifdef DEBUG
    print_stat("free", free_cnt++);
#endif
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    size_t oldsize; /* Payload of the current block */
    void *newbp;    /* New block pointer */

    /* Equivalent to mm_malloc(size) if bp is NULL */
    if (!bp)
        return mm_malloc(size);

    /* Equivalent to mm_free(bp) if size is equal to zero */
    if (!size)
    {
        mm_free(bp);
        return NULL;
    }

    /* Allocate a new block */
    if (!(newbp = mm_malloc(size)))
        return NULL;

    /* Copy the words from the previous block to the newly allocated block */
    oldsize = GET_SIZE(HDRP(bp)) - DSIZE;
    memcpy(newbp, bp, MIN(oldsize, size));

    /* Free the old block */
    mm_free(bp);

#ifdef DEBUG
    print_stat("realloc", realloc_cnt++);
#endif

    return newbp;
}

/*
 * Helper Function Implementations
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    /* Size is at least 24 bytes */
    size = MAX(size, MIN_BLK_SIZE);

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header; replaces existing epilogue block's header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* Case 1 - There is no adjacent free block */
    if (prev_alloc && next_alloc)
        ;

    /* Case 2 - Next block is free block */
    else if (prev_alloc && !next_alloc)
    {
        remove_seglist(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    /* Case 3 - Previous block is free block */
    else if (!prev_alloc && next_alloc)
    {
        remove_seglist(PREV_BLKP(bp));

        bp = PREV_BLKP(bp);
        size += GET_SIZE(HDRP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    /* Case 4 - Both the next and previous blocks are free blocks */
    else
    {
        remove_seglist(PREV_BLKP(bp));
        remove_seglist(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    /* Insert new free block to the segregation list */
    insert_seglist(bp);

    return bp;
}

static void *find_fit(size_t asize)
{
    char *seglist;
    int seglistn;
    void *bp;

    /* Search fit block in the free list */
    for (seglistn = get_seglistn(asize); seglistn < NUM_SEG_LISTS; seglistn++)
    {
        seglist = seglists[seglistn];

        for (bp = seglist; bp; bp = NEXT_FREEP(bp))
        {
            if (asize <= GET_SIZE(HDRP(bp)))
                return bp;
        }
    }

    /* No fit */
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    remove_seglist(bp);

    /* Split the block if the remainder is greater than or equal to overhead of free block */
    if ((csize - asize) >= MIN_BLK_SIZE)
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK((csize - asize), 0));
        PUT(FTRP(bp), PACK((csize - asize), 0));

        insert_seglist(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

static int get_seglistn(size_t size)
{
    int seglistn = 0;
    size >>= (SEG_SIZE_OFFSET + 1);

    while (size > 0 && seglistn < NUM_SEG_LISTS - 1)
    {
        seglistn++;
        size >>= 1;
    }

    return seglistn;
}

static void insert_seglist(void *bp)
{
    int seglistn = get_seglistn(GET_SIZE(HDRP(bp)));
    char *seglist = seglists[seglistn];

    if (seglist != NULL)
        PREV_FREEP(seglist) = bp;

    PREV_FREEP(bp) = NULL;
    NEXT_FREEP(bp) = seglist;

    seglists[seglistn] = bp;
}

static void remove_seglist(void *bp)
{
    int seglistn = get_seglistn(GET_SIZE(HDRP(bp)));

    char *prev = PREV_FREEP(bp);
    char *next = NEXT_FREEP(bp);

    if (prev != NULL)
        NEXT_FREEP(prev) = next;
    else
        seglists[seglistn] = next;

    if (next != NULL)
        PREV_FREEP(next) = prev;
}

/*
 * Debug Functions
 */
#ifdef DEBUG
static void print_stat(char *context, int count)
{
    printf("\n");
    printf("%*s*** %s [%d] STATUS ***%*s\n", 16, "", context, count, 16, "");

    printf("\n");
    printf("*** HEAP ***\n");
    print_heap();

    printf("\n");
    printf("*** FREE LISTS ***\n");
    print_flist();

    printf("\n");
}

static void print_heap()
{
    int idx = 0;
    char *bp;
    size_t size = -1;

    for (bp = (char *)mem_heap_lo() + (2 * WSIZE); size != 0; bp = NEXT_BLKP(bp), idx++)
    {
        size = GET_SIZE(HDRP(bp));

        if (idx == 0)
            printf("[PROLOGUE] ");
        else if (size == 0)
            printf("[EPILOGUE] ");
        else
            printf("[%8d] ", idx);

        printf("( %p ) | SIZE = %6uB", bp, size);

        if (GET_ALLOC(HDRP(bp)))
            printf(" | ALLOCATED\n");
        else
            printf(" | FREE\n");
    }
}

static void print_flist()
{
    char *seglist, *bp;
    int count;

    for (int i = 0; i < NUM_SEG_LISTS; i++)
    {
        seglist = seglists[i];
        count = 0;

        if (seglist == NULL)
        {
            printf("[%3d] EMPTY\n", i);
        }
        else
        {

            for (bp = seglist; bp != NULL; bp = NEXT_FREEP(bp))
                count++;

            printf("[%3d] HEAD ( %p ) FREE? %c | #BLOCKS = %3d\n", i, seglist, (GET_ALLOC(HDRP(seglist)) ? 'N' : 'Y'), count);
        }
    }
}
#endif