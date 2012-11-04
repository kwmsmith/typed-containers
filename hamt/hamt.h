#ifndef _HAMT_H_
#define _HAMT_H_

#include <limits.h> 
#include <inttypes.h>

#define CT_ASSERT(cond) extern char dummy_assert_array[(2*(cond))-1]

/* We depend on 8 bytes per char; make sure it holds. */
CT_ASSERT(CHAR_BIT == 8);
#define BITS_PER_PTR (sizeof(void*) * CHAR_BIT)

#define BITS_PER_HASH_MINSIZE 5
#define HASH_MINSIZE (1<<BITS_PER_HASH_MINSIZE)

typedef struct HAMT HAMT;

/* Creates new, empty, HAMT.
 */
HAMT *HAMT_new();

/* Deletes HAMT and all data associated with it using deletefunc() on each data
 * item.
 */
/* void HAMT_delete(HAMT *, void (*deletefunc)(void *data)); */

/*
 * Inserts key into HAMT, associating it with data.
 *
 * If the key is not present in the HAMT, inserts it, sets *replace to 1, and
 * returns the data passed in.
 *
 * If the key is already present and *replace is 0, deletes the data passed in
 * using deletefunc() and returns the data currently associated with the key.
 *
 * If the key is already present and *replace is 1, deletes the data currently
 * associated with the key using deletefunc() and replaces it with the data
 * passed in.
 */
    void *
HAMT_insert(HAMT *hamt, void *key, 
        void *value, int *replace,
        unsigned long (*hash_func)(void *),
        int (*eq_func)(void *, void *),
        void (*deletefunc)(void *));
// void *HAMT_insert(HAMT *hamt,  
        // void *key, long (*hash_func)(void *key),
        // void *data, int *replace,
        // void (*deletefunc) (void *data));

/* Searches for the data associated with a key in the HAMT. If the key is not
 * present, returns NULL.
 */
/* void *HAMT_search(HAMT *hamt, const void *key, long (*hashfunc)(const void *key)); */

/* Traverse over all keys in HAMT, calling func() for each data item.  Stops
 * early if func returns 0.
 */
/* int HAMT_traverse(HAMT *hamt,  void *d, */
        /* int (*func)(void *node, void *d)); */

/* :vim: set ft=c */

#endif
