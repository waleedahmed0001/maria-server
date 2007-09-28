/******************************************************
Hash storage.
Provides a data structure that stores chunks of data in
its own storage, avoiding duplicates.

(c) 2007 Innobase Oy

Created September 22, 2007 Vasil Dimov
*******************************************************/

#include "univ.i"
#include "ha0storage.h"
#include "hash0hash.h"
#include "mem0mem.h"
#include "ut0rnd.h"

#ifdef UNIV_NONINL
#include "ha0storage.ic"
#endif

/***********************************************************************
Retrieves a data from a storage. If it is present, a pointer to the
stored copy of data is returned, otherwise NULL is returned. */
static
const void*
ha_storage_get(
/*===========*/
	ha_storage_t*	storage,	/* in: hash storage */
	const void*	data,		/* in: data to check for */
	ulint		data_len)	/* in: data length */
{
	ha_storage_node_t*	node;
	ulint			fold;

	/* avoid repetitive calls to ut_fold_binary() in the HASH_SEARCH
	macro */
	fold = ut_fold_binary(data, data_len);

#define IS_FOUND	\
	node->data_len == data_len && memcmp(node->data, data, data_len) == 0

	HASH_SEARCH(
		next,			/* node->"next" */
		storage->hash,		/* the hash table */
		fold,			/* key */
		ha_storage_node_t*,	/* type of node->next */
		node,			/* auxiliary variable */
		IS_FOUND);		/* search criteria */

	if (node == NULL) {

		return(NULL);
	}
	/* else */

	return(node->data);
}

/***********************************************************************
Copies data into the storage and returns a pointer to the copy. If the
same data chunk is already present, then pointer to it is returned.
Data chunks are considered to be equal if len1 == len2 and
memcmp(data1, data2, len1) == 0. */

const void*
ha_storage_put(
/*===========*/
	ha_storage_t*	storage,	/* in/out: hash storage */
	const void*	data,		/* in: data to store */
	ulint		data_len)	/* in: data length */
{
	void*			raw;
	ha_storage_node_t*	node;
	const void*		data_copy;
	ulint			fold;

	/* check if data chunk is already present */
	data_copy = ha_storage_get(storage, data, data_len);
	if (data_copy != NULL) {

		return(data_copy);
	}

	/* not present, add it */

	/* we put the auxiliary node struct and the data itself in one
	continuous block */
	raw = mem_heap_alloc(storage->heap,
			     sizeof(ha_storage_node_t) + data_len);

	node = (ha_storage_node_t*) raw;
	data_copy = (byte*) raw + sizeof(*node);

	memcpy((byte*) raw + sizeof(*node), data, data_len);

	node->data_len = data_len;
	node->data = data_copy;

	/* avoid repetitive calls to ut_fold_binary() in the HASH_INSERT
	macro */
	fold = ut_fold_binary(data, data_len);

	HASH_INSERT(
		ha_storage_node_t,	/* type used in the hash chain */
		next,			/* node->"next" */
		storage->hash,		/* the hash table */
		fold,			/* key */
		node);			/* add this data to the hash */

	/* the output should not be changed because it will spoil the
	hash table */
	return(data_copy);
}
