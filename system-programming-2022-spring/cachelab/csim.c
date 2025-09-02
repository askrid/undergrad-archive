/* Simple cache simulator implementation by J. W. Choi */

#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cachelab.h"

/* Information of a cache line. */
typedef struct _CacheLine
{
    uint64_t valid : 1;    /* Set if the line is occupied by a valid data. */
    uint64_t priority : 8; /* For LRU replacement. Highest priority in a set would be evicted. */
    uint64_t tag : 55;     /* Portion of an address to identify a specific line within the set it belongs to. */
} CacheLine;

/* Result of a memory operation. */
typedef struct _Result
{
    uint8_t hit : 4;
    uint8_t miss : 2;
    uint8_t eviction : 2;
} Result;

/* Global Variables */
static CacheLine **cache; /* 2-dimensional array for virtual cache. */

static uint8_t n_setb;  /* Number of set index bits. */
static uint8_t n_assoc; /* Number of lines per set. */
static uint8_t n_blkb;  /* Number of block offset bits. */
static char *trace;     /* Path to a trace file. */

static int f_verbose; /* Verbose option flag. */
static int f_help;    /* Print help message flag. */

static int cnt_hit;      /* Cache hit count. */
static int cnt_miss;     /* Cache miss count. */
static int cnt_eviction; /* Cache eviction count. */

/* Helper Function Declarations */

/* Parse option arguments from the command line. */
void parse_args(int argc, char **argv);

/* Open the trace file and run cache simulation. */
void run_csim();

/* Find a cache line in the given set. Return 1 if found, else 0. */
int contains_line(CacheLine *cset, uint64_t tag);

/* Place a cache line in the given set. Return 1 if evicted, else 0. */
int place_line(CacheLine *cset, uint64_t tag);

/* Print result for verbose mode. */
void print_verbose(char op, uint64_t addr, int size, Result result);

/* Print help message. */
void print_help();

/* Main Routine */
int main(int argc, char **argv)
{
    int n_set;

    parse_args(argc, argv);

    if (f_help)
    {
        print_help();
        return 0;
    }

    /* Get the number of cache sets. */
    n_set = 1 << n_setb;

    /* Initialize virtual cache. */
    cache = (CacheLine **)calloc(n_set, sizeof(CacheLine *));
    for (int i = 0; i < n_set; i++)
        cache[i] = (CacheLine *)calloc(n_assoc, sizeof(CacheLine));

    run_csim();

    /* Free the allocated memory. */
    for (int i = 0; i < n_set; i++)
        free(cache[i]);
    free(cache);

    printSummary(cnt_hit, cnt_miss, cnt_eviction);

    return 0;
}

/* Helper Function Definitions */
void parse_args(int argc, char **argv)
{
    int opt;
    extern char *optarg;

    while ((opt = getopt(argc, argv, "s:E:b:t:vh")) != -1)
    {
        switch (opt)
        {
        case 's':
            n_setb = atoi(optarg);
            break;
        case 'E':
            n_assoc = atoi(optarg);
            break;
        case 'b':
            n_blkb = atoi(optarg);
            break;
        case 't':
            trace = optarg;
            break;
        case 'v':
            f_verbose = 1;
            break;
        case 'h':
            f_help = 1;
            break;
        default:
            break;
        }
    }
}

void run_csim()
{
    FILE *fp;
    char op;
    uint64_t addr, idx, tag;
    int size;
    CacheLine *cset;
    Result result;

    if ((fp = fopen(trace, "r")) == NULL)
        exit(0);

    /* Size field is ignored. */
    while (fscanf(fp, " %c %lx,%d", &op, &addr, &size) > 0)
    {
        /* Reset the result. */
        result = (Result){0};

        /* Don't care instruction load. */
        if (op == 'I')
            continue;

        /* Compute set index and tag of the cache. */
        idx = (addr >> n_blkb) & ((1 << n_setb) - 1);
        tag = addr >> (n_setb + n_blkb);

        /* Get the cache set. */
        cset = cache[idx];

        if (contains_line(cset, tag))
            result.hit += (op == 'M') ? 2 : 1;

        else
        {
            result.miss++;

            if (place_line(cset, tag))
                result.eviction++;

            if (op == 'M')
                result.hit++;
        }

        /* Update the counters. */
        cnt_hit += result.hit;
        cnt_miss += result.miss;
        cnt_eviction += result.eviction;

        /* Print verbose result. */
        if (f_verbose)
            print_verbose(op, addr, size, result);
    }

    fclose(fp);
}

int contains_line(CacheLine *cset, uint64_t tag)
{
    CacheLine cline;
    int idx_target;
    uint64_t old_priority;

    /* Find a cache line matching the given tag. */
    for (idx_target = 0; idx_target < n_assoc; idx_target++)
    {
        cline = cset[idx_target];

        if (cline.valid && cline.tag == tag)
        {
            old_priority = cline.priority;
            break;
        }
    }

    /* Update the priorities if cache line was found. */
    if (idx_target < n_assoc)
    {
        for (int i = 0; i < n_assoc; i++)
        {
            cline = cset[i];

            if (cline.valid && cline.priority < old_priority)
                cset[i].priority++;
        }

        cset[idx_target].priority = 0;

        return 1;
    }

    return 0;
}

int place_line(CacheLine *cset, uint64_t tag)
{
    CacheLine cline;
    int idx_target, eviction = 0;

    /* Find an empty line. */
    for (idx_target = 0; idx_target < n_assoc; idx_target++)
    {
        if (!cset[idx_target].valid)
            break;
    }

    /* If there is no empty line, choose a victim and unset its valid bit. */
    if (idx_target == n_assoc)
    {
        eviction = 1;
        idx_target = 0;

        for (int i = 0; i < n_assoc; i++)
        {
            if (cset[i].priority > cset[idx_target].priority)
                idx_target = i;
        }

        cset[idx_target].valid = 0;
    }

    /* Update the priorities. */
    for (int i = 0; i < n_assoc; i++)
    {
        cline = cset[i];

        if (cline.valid)
            cset[i].priority++;
    }

    /* Place a new cache line. */
    cset[idx_target].valid = 1;
    cset[idx_target].priority = 0;
    cset[idx_target].tag = tag;

    return eviction;
}

void print_verbose(char op, uint64_t addr, int size, Result result)
{
    printf("%c %lx,%d ", op, addr, size);

    for (int i = 0; i < result.miss; i++)
        printf("%s ", "miss");

    for (int i = 0; i < result.eviction; i++)
        printf("%s ", "eviction");

    for (int i = 0; i < result.hit; i++)
        printf("%s ", "hit");

    printf("\n");
}

void print_help()
{
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
           "Options:\n"
           "  -h         Print this help message.\n"
           "  -v         Optional verbose flag.\n"
           "  -s <num>   Number of set index bits.\n"
           "  -E <num>   Number of lines per set.\n"
           "  -b <num>   Number of block offset bits.\n"
           "  -t <file>  Trace file.\n"
           "\n"
           "Examples:\n"
           "  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
           "  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}