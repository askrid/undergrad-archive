#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include "etcp_hash.h"

#define IS_FLOW_TABLE(x)	(x == CalcHash)
#define IS_LISTEN_TABLE(x)	(x == GetHashListener)
/*----------------------------------------------------------------------------*/
struct hashtable * 
CreateHT(unsigned int (*hashfn) (const void *), // key function
		int (*eqfn) (const void*, const void *),            // equality
		int bins) // no of bins
{
	int i;
	struct hashtable* ht = calloc(1, sizeof(struct hashtable));
	if (!ht){
		fprintf(stderr,"calloc: CreateHT");
		return 0;
	}

	ht->hashfn = hashfn;
	ht->eqfn = eqfn;
	ht->bins = bins;

	/* creating bins */

	if (IS_FLOW_TABLE(hashfn)) {
		ht->ht_table = calloc(bins, sizeof(hash_bucket_head));
		if (!ht->ht_table) {
			fprintf(stderr,"calloc: CreateHT bins!\n");
			free(ht);
			return 0;
		}
		/* init the tables */
		for (i = 0; i < bins; i++)
			TAILQ_INIT(&ht->ht_table[i]);
	} else if (IS_LISTEN_TABLE(hashfn)) {
		ht->lt_table = calloc(bins, sizeof(list_bucket_head));
		if (!ht->lt_table) {
			fprintf(stderr,"calloc: CreateHT bins!\n");
			free(ht);
			return 0;
		}
		/* init the tables */
		for (i = 0; i < bins; i++)
			TAILQ_INIT(&ht->lt_table[i]);
	}

	return ht;
}
/*----------------------------------------------------------------------------*/
void
DestroyHT(struct hashtable *ht)
{
	if (IS_FLOW_TABLE(ht->hashfn))
		free(ht->ht_table);
	else /* IS_LISTEN_TABLE(ht->hashfn) */
		free(ht->lt_table);
	free(ht);
}
/*----------------------------------------------------------------------------*/
int 
InsertFlowHT(struct hashtable *ht, void *it)
{
	/* create an entry*/ 
	int idx;
	tcp_flow *item = (tcp_flow *)it;

	assert(ht);

	idx = ht->hashfn(item);
	assert(idx >=0 && idx < NUM_BINS_FLOWS);

	TAILQ_INSERT_TAIL(&ht->ht_table[idx], item, rcvvar->he_link);

	item->ht_idx = TCP_AR_CNT;
	
	return 0;
}
/*----------------------------------------------------------------------------*/
void* 
RemoveFlowHT(struct hashtable *ht, void *it)
{
	hash_bucket_head *head;
	tcp_flow *item = (tcp_flow *)it;
	int idx = ht->hashfn(item);

	head = &ht->ht_table[idx];
	TAILQ_REMOVE(head, item, rcvvar->he_link);	

	return (item);
}	
/*----------------------------------------------------------------------------*/
void * 
SearchFlowHT(struct hashtable *ht, const void *it)
{
	int idx;
	const tcp_flow *item = (const tcp_flow *)it;
	tcp_flow *walk;
	hash_bucket_head *head;

	idx = ht->hashfn(item);

	head = &ht->ht_table[ht->hashfn(item)];
	TAILQ_FOREACH(walk, head, rcvvar->he_link) {
		if (ht->eqfn(walk, item)) 
			return walk;
	}

	UNUSED(idx);
	return NULL;
}
/*----------------------------------------------------------------------------*/
unsigned int
GetHashListener(const void *l)
{
	struct tcp_listener *listener = (struct tcp_listener *)l;

	return listener->socket->saddr.sin_port & (NUM_BINS_LISTENERS - 1);
}
/*----------------------------------------------------------------------------*/
int
GetEqualListner(const void *l1, const void *l2)
{
	struct tcp_listener *listener1 = (struct tcp_listener *)l1;
	struct tcp_listener *listener2 = (struct tcp_listener *)l2;

	return (listener1->socket->saddr.sin_port == listener2->socket->saddr.sin_port);
}
/*----------------------------------------------------------------------------*/
int 
InsertListnerHT(struct hashtable *ht, void *it)
{
	/* create an entry*/ 
	int idx;
	struct tcp_listener *item = (struct tcp_listener *)it;

	assert(ht);

	idx = ht->hashfn(item);
	assert(idx >=0 && idx < NUM_BINS_LISTENERS);

	TAILQ_INSERT_TAIL(&ht->lt_table[idx], item, he_link);
	
	return 0;
}
/*----------------------------------------------------------------------------*/
void * 
RemoveListnerHT(struct hashtable *ht, void *it)
{
	list_bucket_head *head;
	struct tcp_listener *item = (struct tcp_listener *)it;
	int idx = ht->hashfn(item);

	head = &ht->lt_table[idx];
	TAILQ_REMOVE(head, item, he_link);	

	return (item);
}	
/*----------------------------------------------------------------------------*/
void * 
SearchListnerHT(struct hashtable *ht, const void *it)
{
	int idx;
	struct tcp_listener item;
	uint16_t port = *((uint16_t *)it);
	struct tcp_listener *walk;
	list_bucket_head *head;
	struct socket_map s;

	s.saddr.sin_port = port;
	item.socket = &s;

	idx = ht->hashfn(&item);

	head = &ht->lt_table[idx];
	TAILQ_FOREACH(walk, head, he_link) {
		if (ht->eqfn(walk, &item)) 
			return walk;
	}

	return NULL;
}
/*----------------------------------------------------------------------------*/
