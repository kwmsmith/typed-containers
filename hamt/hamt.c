#include "hamt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <assert.h> */

#define assert(e)  \
    ((void) ((e) ? 0 : __assert (#e, __FILE__, __LINE__)))
#define __assert(e, file, line) \
    ((void)printf ("%s:%u: failed assertion `%s'\n", file, line, e), exit(1))

#define CHATTY

#define GET_SUB_TRIE_PTR(ep) ((HAMT_entry*)((intptr_t)(ep)->value & ~1))
#define SET_SUB_TRIE_BIT(ep) ((ep)->value = (void*)((intptr_t)(ep)->value | 1))
#define IS_SUB_TRIE_SLOT(ep) (((intptr_t)(ep)->value) & 1)
#define IS_KEY_VAL_SLOT(ep) (!IS_SUB_TRIE_SLOT(ep))

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
    HAMT_entry *root;
};

    HAMT *
HAMT_new()
{
    int i;
    HAMT *hamt = NULL;
    hamt = malloc(sizeof(HAMT));
    if (!hamt)
        return NULL;
    hamt->root = malloc(HASH_MINSIZE * sizeof(HAMT_entry));
    if (!hamt->root)
        goto fail;
    for (i=0; i<HASH_MINSIZE; i++) {
        hamt->root[i].key = hamt->root[i].value = NULL;
    }
    goto success;
fail:
    if (hamt)
        free(hamt);
success:
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
HAMT_insert_into_sub_trie(HAMT_entry *ep, 
        int nbits_used, 
        unsigned long hash,
        unsigned long (*hash_func)(void *),
        int (*eq_func)(void *, void *),
        void (*deletefunc)(void *),
        void *key, void *value)
{
    /* assert(0); [> XXX: Cut and paste -- needs refactoring!!! <] */
    unsigned int sub_index = 0;
    HAMT_entry *sub_trie = GET_SUB_TRIE_PTR(ep);
    /* XXX: FIXME: what happens when nbits_used > BITS_PER_PTR??? */
    assert(nbits_used < BITS_PER_PTR);
    sub_index = (hash >> nbits_used) & (HASH_MINSIZE-1);
    assert(sub_index >= 0 && sub_index < BITS_PER_PTR);

    /* find index at which to insert; zero-based. */
    int insert_index = popcount((uintptr_t)ep->key & ((1<<sub_index)-1));
    int num_inserted = popcount((uintptr_t)ep->key);

    assert(num_inserted > 0 && num_inserted < BITS_PER_PTR);
    assert(insert_index >= 0 && insert_index <= num_inserted);

    if((uintptr_t)ep->key & (1<<sub_index)) {
        /* case: bitmask is full at sub_index, another collision. */
        ep = sub_trie + insert_index;
        if (IS_KEY_VAL_SLOT(ep)) {
            /* Case: collision */
            assert(ep->key);

            /* check for key equality */
            if (key == ep->key || (*eq_func)(key, ep->key)) {
                /* keys are the same, insert here */
                (*deletefunc)(ep->key);
                (*deletefunc)(ep->value);
#ifdef CHATTY
                printf("replacing at index: %d\n", sub_index);
#endif
                ep->key = key;
                ep->value = value;
            }
            else {
                HAMT_insert_new_sub_trie(ep, nbits_used + BITS_PER_HASH_MINSIZE, hash_func);
                HAMT_insert_into_sub_trie(ep, nbits_used + BITS_PER_HASH_MINSIZE, hash, hash_func, 
                        eq_func, deletefunc, key, value);
            }
        } 
        else {
            /* case: ep is a sub trie, descend into it */
            assert(IS_SUB_TRIE_SLOT(ep));
            HAMT_insert_into_sub_trie(ep, nbits_used + BITS_PER_HASH_MINSIZE, hash, hash_func, eq_func, deletefunc, key, value);
        }
    }
    else {
        /* case: bitmask is empty at sub_index. */

        /* shift the larger values up one */
        memmove(sub_trie + (insert_index+1), sub_trie + insert_index,
                sizeof(HAMT_entry) * (num_inserted - insert_index));

        /* insert the entry at the insert_index location */
#ifdef CHATTY
                printf("replacing at index: %d\n", insert_index);
#endif
        sub_trie[insert_index].key = key;
        sub_trie[insert_index].value = value;
    }
}

    void *
HAMT_insert(HAMT *hamt, void *key, 
        void *value, int *replace,
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

    if (IS_KEY_VAL_SLOT(ep)) {
        if (!ep->key) {
            /* case: no collision, empty slot. */
#               ifdef CHATTY
            printf("inserting at index: %d\n", index);
#               endif
            assert(!ep->value);
            ep->key = key;
            ep->value = value;
            *replace = 1;
            return value;
        }
        else {
            /* Case: collision */

            /* check for key equality */
            if (key == ep->key || (*eq_func)(key, ep->key)) {
                /* keys are the same, insert here */
                (*deletefunc)(ep->key);
                (*deletefunc)(ep->value);
#               ifdef CHATTY
            printf("replacing at index: %d\n", index);
#               endif
                ep->key = key;
                ep->value = value;
            }
            else {
                HAMT_insert_new_sub_trie(ep, BITS_PER_HASH_MINSIZE, hash_func);
                /* assert(0); [> XXX: Implement this function!!! <] */
                HAMT_insert_into_sub_trie(ep, BITS_PER_HASH_MINSIZE, hash, hash_func, eq_func, deletefunc, key, value);
            }
        }
    } 
    else {
        /* case: ep is a sub trie, descend into it */
        assert(IS_SUB_TRIE_SLOT(ep));
        HAMT_insert_into_sub_trie(ep, BITS_PER_HASH_MINSIZE, hash, hash_func, eq_func, deletefunc, key, value);
    }
    return NULL;
}

/* int */
/* HAMT_search() */
/* { */
/* } */

/* void */
/* HAMT_delete(HAMT_entry *root) */
/* { */
    /* if (root) { */
        /* free(root); */
        /* root = NULL; */
    /* } */
/* } */
