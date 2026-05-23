#ifndef etcp_hash_H
#define etcp_hash_H

#include <sys/queue.h>
#include "tcp_flow.h"

#define NUM_BINS_FLOWS 		(131072)     /* 132 K entries per thread*/
#define NUM_BINS_LISTENERS	(1024)	     /* assuming that chaining won't happen excessively */
#define TCP_AR_CNT 		(3)

typedef struct hash_bucket_head {
	tcp_flow *tqh_first;
	tcp_flow **tqh_last;
} hash_bucket_head;

typedef struct list_bucket_head {
	struct tcp_listener *tqh_first;
	struct tcp_listener **tqh_last;
} list_bucket_head;

/* hashtable structure */
struct hashtable {
	uint32_t bins;

	union {
		hash_bucket_head *ht_table;
		list_bucket_head *lt_table;
	};

	// functions
	unsigned int (*hashfn) (const void *);
	int (*eqfn) (const void *, const void *);
};

/*functions for hashtable*/
struct hashtable *CreateHT(unsigned int (*hashfn) (const void *), 
				  int (*eqfn) (const void *, 
					       const void *),
				  int bins);
void DestroyHT(struct hashtable *ht);


int InsertFlowHT(struct hashtable *ht, void *);
void* RemoveFlowHT(struct hashtable *ht, void *);
void *SearchFlowHT(struct hashtable *ht, const void *);
unsigned int GetHashListener(const void *hbo_port_ptr);
int GetEqualListner(const void *hbo_port_ptr1, const void *hbo_port_ptr2);
int InsertListnerHT(struct hashtable *ht, void *);
void *RemoveListnerHT(struct hashtable *ht, void *);
void *SearchListnerHT(struct hashtable *ht, const void *);

#endif /* etcp_hash_H */
