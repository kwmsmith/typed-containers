#include "hamt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <assert.h> */

#define assert(e)  \
    ((void) ((e) ? 0 : __assert (#e, __FILE__, __LINE__)))
#define __assert(e, file, line) \
    ((void)printf ("%s:%u: failed assertion `%s'\n", file, line, e), exit(1))

/* #define assert(e) ((void)0) */

/* #define CHATTY */

#define GET_SUB_TRIE_PTR(ep) ((HAMT_entry*)((intptr_t)(ep)->value & ~1))
#define SET_SUB_TRIE_BIT(ep) ((ep)->value = (void*)((intptr_t)(ep)->value | 1))
#define IS_SUB_TRIE_SLOT(ep) (((intptr_t)(ep)->value) & 1)
#define IS_KEY_VAL_SLOT(ep) (!IS_SUB_TRIE_SLOT(ep))

static int num_sub_trie = 0;

    int
popcount(uintptr_t a)
{
    assert(sizeof(uintptr_t) == sizeof(unsigned int));
    return __builtin_popcount(a);
}

typedef struct {
    void *key;
    void *value;
} HAMT_entry;

struct HAMT {
    HAMT_entry root[HASH_MINSIZE];
};

    HAMT *
HAMT_new()
{
    HAMT *hamt = NULL;
    if (!(hamt = malloc(sizeof(HAMT))))
        return NULL;
    memset(hamt->root, 0, sizeof(HAMT_entry) * HASH_MINSIZE);
    return hamt;
}

    static void
HAMT_insert_new_sub_trie(HAMT_entry *ep, int nbits_used, 
        unsigned long(*hash_func)(void*))
{
    /* Create a sub-trie. */
    HAMT_entry *sub_trie = NULL;
    unsigned int sub_index = 0;
    int i = 0;

    unsigned long existing_hash = (*hash_func)(ep->key);

    sub_trie = malloc(HASH_MINSIZE * sizeof(HAMT_entry));
    assert(sub_trie); /* TODO: proper NULL handling */
    assert(HASH_MINSIZE == 32); /* ??? */

#ifdef CHATTY
    printf("HAMT_insert_new_sub_trie:num_sub_trie: %d\n", ++num_sub_trie);
#endif

    /* initialize it */
    for (i=0; i<HASH_MINSIZE; i++) {
        sub_trie[i].key = sub_trie[i].value = NULL;
    }

    /* place existing key-val in new sub-trie */
    sub_trie[0].key = ep->key; sub_trie[0].value = ep->value;

    /* place new sub-trie in previous slot. */
    /* XXX: FIXME: what happens when nbits_used + BITS_PER_HASH_MINSIZE > BITS_PER_PTR??? */
    assert(nbits_used + BITS_PER_HASH_MINSIZE < BITS_PER_PTR);
    sub_index = (existing_hash >> (nbits_used + BITS_PER_HASH_MINSIZE)) & (HASH_MINSIZE-1);
    assert(sub_index >= 0 && sub_index < 32);
    ep->key = (void *)(intptr_t)(1<<sub_index);
    ep->value = sub_trie;
    SET_SUB_TRIE_BIT(ep);
    assert(IS_SUB_TRIE_SLOT(ep));
}


    static void
HAMT_insert_into_sub_trie(register HAMT_entry *ep, 
        int nbits_used, unsigned long hash,
        unsigned long (*hash_func)(void *),
        int (*eq_func)(void *, void *),
        void (*deletefunc)(void *),
        void *key, void *value)
{
    HAMT_entry *sub_trie = NULL;
    unsigned int sub_index = 0;
    int insert_index = 0;
    int num_inserted = 0;

    while (1) {
        /* XXX: FIXME: what happens when nbits_used > BITS_PER_PTR??? */
        assert(nbits_used < BITS_PER_PTR);
        if (IS_KEY_VAL_SLOT(ep)) {
            /* Case: collision */
            assert(ep->key);

            /* check for key equality */
            if (!(key == ep->key || (*eq_func)(key, ep->key))) {
                HAMT_insert_new_sub_trie(ep, nbits_used, hash_func);
                continue;
            }
            else {
                /* keys are the same, insert here */
                (*deletefunc)(ep->key);
                (*deletefunc)(ep->value);
                ep->key = key;
                ep->value = value;
                return;
            }
        }
        /* sub_trie_slot */

        assert(IS_SUB_TRIE_SLOT(ep));
        sub_trie = GET_SUB_TRIE_PTR(ep);
        sub_index = (hash >> nbits_used) & (HASH_MINSIZE-1);
        assert(sub_index >= 0 && sub_index < BITS_PER_PTR);

        /* find index at which to insert; zero-based. */
        insert_index = popcount((uintptr_t)ep->key & ((1<<sub_index)-1));

        if((uintptr_t)ep->key & (1<<sub_index)) {
            /* case: bitmask is full at sub_index, another collision. */
            ep = sub_trie + insert_index;
            nbits_used += BITS_PER_HASH_MINSIZE;
            continue;
        }   
        else {
            num_inserted = popcount((uintptr_t)ep->key);
            assert(num_inserted > 0);
            assert(num_inserted <= BITS_PER_PTR);
            assert(insert_index >= 0 && insert_index <= num_inserted);
            /* case: bitmask is empty at sub_index. */

            /* shift the larger values up one */
            memmove(sub_trie + (insert_index+1), sub_trie + insert_index,
                    sizeof(HAMT_entry) * (num_inserted - insert_index));

            /* insert the entry at the insert_index location */
            sub_trie[insert_index].key = key;
            sub_trie[insert_index].value = value;
            ep->key = (void*)((intptr_t)ep->key | (1<<insert_index));
            assert(popcount((uintptr_t)ep->key) == num_inserted + 1);
            return;
        }
    }
    assert(0); /* Should never get here */
}

    void *
HAMT_insert(HAMT *hamt, void *key, 
        void *value,
        unsigned long (*hash_func)(void *),
        int (*eq_func)(void *, void *),
        void (*deletefunc)(void *))
{
    unsigned long hash = 0;
    unsigned int index = 0;
    HAMT_entry *ep = NULL;

    assert(hamt); assert(hamt->root);
    assert(hash_func); assert(key); assert(value);

    hash = (unsigned long)(*hash_func)(key);

    /* get BITS_PER_HASH_MINSIZE lsb of hash */
    assert(BITS_PER_HASH_MINSIZE < BITS_PER_PTR);
    index = hash & (HASH_MINSIZE-1);

    ep = hamt->root + index;

    if (!ep->key) {
        /* Special case for top-level root hash table. */
        /* case: no collision, empty slot for root hash table. */
#               ifdef CHATTY
        printf("inserting at index: %d\n", index);
#               endif
        assert(!ep->value);
        ep->key = key;
        ep->value = value;
        return value;
    }

    HAMT_insert_into_sub_trie(ep, BITS_PER_HASH_MINSIZE, 
            hash, hash_func, eq_func, deletefunc, key, value);
    return NULL;
}

    static void *
HAMT_search_sub_trie(HAMT_entry *ep, int nbits_used, 
        unsigned long hash, unsigned long (*hash_func)(void *),
        int (*eq_func)(void*, void*), void *key)
{
    HAMT_entry *sub_trie = NULL;
    unsigned int sub_index = 0;
    int insert_index = 0;
    int num_inserted = 0;

    while (1) {
        /* XXX: FIXME: what happens when nbits_used > BITS_PER_PTR??? */
        assert(nbits_used < BITS_PER_PTR);
        if (IS_KEY_VAL_SLOT(ep)) {
            /* Case: collision */
            assert(ep->key);
            return (key == ep->key || (*eq_func)(key, ep->key)) ? ep->value : NULL;
        }
        /* sub_trie_slot */

        assert(IS_SUB_TRIE_SLOT(ep));
        sub_trie = GET_SUB_TRIE_PTR(ep);
        sub_index = (hash >> nbits_used) & (HASH_MINSIZE-1);
        assert(sub_index >= 0 && sub_index < BITS_PER_PTR);

        /* find index to search; zero-based. */
        insert_index = popcount((uintptr_t)ep->key & ((1<<sub_index)-1));

        if((uintptr_t)ep->key & (1<<sub_index)) {
            /* case: bitmask is full at sub_index, another collision. */
            ep = sub_trie + insert_index;
            nbits_used += BITS_PER_HASH_MINSIZE;
            continue;
        }   
        else 
            return NULL;
    }
    assert(0); /* Should never get here */
}

    void *
HAMT_search(HAMT *hamt, void *key, 
        unsigned long (*hash_func)(void *), 
        int (*eq_func)(void *, void *))
{
    unsigned long hash = 0;
    unsigned int index = 0;
    HAMT_entry *ep = NULL;

    assert(hamt); assert(hamt->root);
    assert(hash_func); assert(key);

    hash = (*hash_func)(key);

    /* get BITS_PER_HASH_MINSIZE lsb of hash */
    assert(BITS_PER_HASH_MINSIZE < BITS_PER_PTR);
    index = hash & (HASH_MINSIZE-1);

    ep = hamt->root + index;

    if (!ep->key) {
        assert(!ep->value);
        return ep->value;
    }
    return HAMT_search_sub_trie(ep, BITS_PER_HASH_MINSIZE, hash, hash_func, eq_func, key);
}

    void 
HAMT_delete(HAMT *hamt, void (*deletefunc)(void *data)) 
{
    return;
}
