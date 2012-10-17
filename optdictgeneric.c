#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "optdictgeneric.h"

/* See large comment block below.  This must be >= 1. */
#define PERTURB_SHIFT 5

/*
   Major subtleties ahead:  Most hash schemes depend on having a "good" hash
   function, in the sense of simulating randomness.  Python doesn't:  its most
   important hash functions (for strings and ints) are very regular in common
cases:

>>> map(hash, (0, 1, 2, 3))
[0, 1, 2, 3]
>>> map(hash, ("namea", "nameb", "namec", "named"))
[-1658398457, -1658398460, -1658398459, -1658398462]
>>>

This isn't necessarily bad!  To the contrary, in a table of size 2**i, taking
the low-order i bits as the initial table index is extremely fast, and there
are no collisions at all for dicts indexed by a contiguous range of ints.
The same is approximately true when keys are "consecutive" strings.  So this
gives better-than-random behavior in common cases, and that's very desirable.

OTOH, when collisions occur, the tendency to fill contiguous slices of the
hash table makes a good collision resolution strategy crucial.  Taking only
the last i bits of the hash code is also vulnerable:  for example, consider
[i << 16 for i in range(20000)] as a set of keys.  Since ints are their own
hash codes, and this fits in a dict of size 2**15, the last 15 bits of every
hash code are all 0:  they *all* map to the same table index.

But catering to unusual cases should not slow the usual ones, so we just take
the last i bits anyway.  It's up to collision resolution to do the rest.  If
we *usually* find the key we're looking for on the first try (and, it turns
out, we usually do -- the table load factor is kept under 2/3, so the odds
are solidly in our favor), then it makes best sense to keep the initial index
computation dirt cheap.

The first half of collision resolution is to visit table indices via this
recurrence:

j = ((5*j) + 1) mod 2**i

For any initial j in range(2**i), repeating that 2**i times generates each
int in range(2**i) exactly once (see any text on random-number generation for
proof).  By itself, this doesn't help much:  like linear probing (setting
j += 1, or j -= 1, on each loop trip), it scans the table entries in a fixed
order.  This would be bad, except that's not the only thing we do, and it's
actually *good* in the common cases where hash keys are consecutive.  In an
example that's really too small to make this entirely clear, for a table of
size 2**3 the order of indices is:

0 -> 1 -> 6 -> 7 -> 4 -> 5 -> 2 -> 3 -> 0 [and here it's repeating]

If two things come in at index 5, the first place we look after is index 2,
not 6, so if another comes in at index 6 the collision at 5 didn't hurt it.
Linear probing is deadly in this case because there the fixed probe order
is the *same* as the order consecutive keys are likely to arrive.  But it's
extremely unlikely hash codes will follow a 5*j+1 recurrence by accident,
and certain that consecutive hash codes do not.

The other half of the strategy is to get the other bits of the hash code
into play.  This is done by initializing a (unsigned) vrbl "perturb" to the
full hash code, and changing the recurrence to:

j = (5*j) + 1 + perturb;
perturb >>= PERTURB_SHIFT;
use j % 2**i as the next table index;

Now the probe sequence depends (eventually) on every bit in the hash code,
and the pseudo-scrambling property of recurring on 5*j+1 is more valuable,
because it quickly magnifies small differences in the bits that didn't affect
the initial index.  Note that because perturb is unsigned, if the recurrence
is executed often enough perturb eventually becomes and remains 0.  At that
point (very rarely reached) the recurrence is on (just) 5*j+1 again, and
that's certain to find an empty slot eventually (since it generates every int
in range(2**i), and we make sure there's always at least one empty slot).

Selecting a good value for PERTURB_SHIFT is a balancing act.  You want it
small so that the high bits of the hash code continue to affect the probe
sequence across iterations; but you want it large so that in really bad cases
the high-order hash bits have an effect on early iterations.  5 was "the
best" in minimizing total collisions across experiments Tim Peters ran (on
both normal and pathological cases), but 4 and 6 weren't significantly worse.

Historical:  Reimer Behrends contributed the idea of using a polynomial-based
approach, using repeated multiplication by x in GF(2**n) where an irreducible
polynomial for each table size was chosen such that x was a primitive root.
Christian Tismer later extended that to use division by x instead, as an
efficient way to get the high bits of the hash code into play.  This scheme
also gave excellent collision statistics, but was more expensive:  two
if-tests were required inside the loop; computing "the next" index took about
the same number of operations but without as much potential parallelism
(e.g., computing 5*j can go on at the same time as computing 1+perturb in the
 above, and then shifting perturb can be done while the table index is being
 masked); and the OptDict struct required a member to hold the table's
polynomial.  In Tim's experiments the current scheme ran faster, produced
equally good collision statistics, needed less code & used less memory.

Theoretical Python 2.5 headache:  hash codes are only C "long", but
sizeof(Py_ssize_t) > sizeof(long) may be possible.  In that case, and if a
dict is genuinely huge, then only the slots directly reachable via indexing
by a C long can be the first slot in a probe sequence.  The probe sequence
will still eventually reach every slot in the table, but the collision rate
on initial probes may be much higher than this scheme was designed for.
Getting a hash code as fat as Py_ssize_t is the only real cure.  But in
practice, this probably won't make a lick of difference for many years (at
        which point everyone will have terabytes of RAM on 64-bit boxes).
*/

/* Object used as dummy key to fill deleted entries */
static void *dummy = NULL; /* Initialized by first call to optdict_new() */


int
init_dummy() 
{
    char *init = "<dummy key>";
    char *alias = NULL;
    if (dummy == NULL) {
        dummy = malloc(12 * sizeof(char));
        if (dummy == NULL)
            return 0;
    }
    alias = dummy;
    while(*init) {
        *alias = *init;
        ++alias;
        ++init;
    }
    *alias = '\0';
    return 1;
}

    static int
eqint(void *a, void *b)
{
    return *(int*)a == *(int*)b;
}

    static int
eqlong(void *a, void *b)
{
    return *(long*)a == *(long*)b;
}

    static int
eqfloat(void *a, void *b)
{
    return *(float*)a == *(float*)b;
}

    static int
eqdouble(void *a, void *b)
{
    return *(double*)a == *(double*)b;
}

#define ARR_LEN(a) (sizeof(a) / sizeof(a[0]))

/* Returns nonzero on failure.
 */
int
set_entry_size_and_ofs(enum type_enum key_type,
                       enum type_enum val_type,
                       register OptDict *mp)
{
    /* XXX: FIXME: This ignores alignment */
    size_t zero_ofs = 0;
    int i, this_size;
    enum type_enum all_types[] = {LONG, LONG, key_type, val_type};
    size_t *offsets[] = {&zero_ofs, &mp->flags_ofs, &mp->key_ofs, &mp->val_ofs};
    assert(ARR_LEN(all_types) == ARR_LEN(offsets));

    this_size = mp->entry_size = 0;
    for(i=0; i<ARR_LEN(all_types); i++) {
        *offsets[i] = mp->entry_size;
        switch(all_types[i]) {
            case INT:
                this_size = sizeof(int);
                break;
            case LONG:
                this_size = sizeof(long);
                break;
            case FLOAT:
                this_size = sizeof(float);
                break;
            case DOUBLE:
                this_size = sizeof(double);
                break;
            default:
                return 1;
        }
        mp->entry_size += this_size;
    }
    return 0;
}

/* forward declaration */
    static OptDictEntry *
lookdict(OptDict *mp, void *key, register long hash);

    OptDict *
OptDict_New(enum type_enum key_type, enum type_enum val_type)
{
    register OptDict *mp;
    size_t table_size;
    if (dummy == NULL) { /* Auto-initialize dummy */
        if (!init_dummy())
            return NULL;
    }
    mp = malloc(sizeof(OptDict));
    if (mp == NULL)
        return NULL;
    /* FIXME: check for zero return */
    if (set_entry_size_and_ofs(key_type, val_type, mp))
        goto fail;
    assert(mp->entry_size);
    table_size = mp->entry_size * optdict_MINSIZE;
    mp->ma_table = malloc(table_size);
    if (mp->ma_table == NULL)
        goto fail;
    mp->ma_mask = optdict_MINSIZE -1;
    memset(mp->ma_table, 0, table_size);
    mp->ma_used = mp->ma_fill = 0;
    mp->ma_lookup = lookdict;
    switch(key_type) {
        case INT:
            mp->eqfunc = eqint;
            break;
        case LONG:
            mp->eqfunc = eqlong;
            break;
        case FLOAT:
            mp->eqfunc = eqfloat;
            break;
        case DOUBLE:
            mp->eqfunc = eqdouble;
            break;
    }
    printf("key_type: %d\n", key_type);
    printf("mp->entry_size: %ld\n", mp->entry_size);
    printf("mp->flags_ofs: %ld\n", mp->flags_ofs);
    printf("mp->key_ofs: %ld\n", mp->key_ofs);
    printf("mp->val_ofs: %ld\n", mp->val_ofs);
    goto success;
fail:
    free(mp);
    mp = NULL;
success:
    return mp;
}

long
int_hash(int x)
{
    long y = (long) x;
    if (-1 == y)
        y = -2;
    return y;
}

    static OptDictEntry *
lookdict(OptDict *mp, void *key, register long hash)
{
    register size_t i;
    register size_t perturb;
    register OptDictEntry *freeslot;
    register size_t mask = (size_t)mp->ma_mask;
    OptDictEntry *ep0 = mp->ma_table;
    register OptDictEntry *ep;
    register long me_hash;
    register long me_flags;

    i = hash & mask;
    ep = GET_ENTRY(mp, ep0, i);
    me_flags = GET_FLAGS(mp, ep);
    if (!(me_flags & FLAG_USED))
        return ep;
    me_hash = GET_HASH(mp, ep);
    if (me_flags & FLAG_DUMMY)
        freeslot = ep;
    else {
        if (me_hash == hash && mp->eqfunc(GET_KEYP(mp, ep), key))
            return ep;
        freeslot = NULL;
    }

    /* In the loop, me_key == dummy is by far (factor of 100s) the
     * least likely outcome, so test for that last.
     */
    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;
        ep = GET_ENTRY(mp, ep0, (i & mask));
        me_flags = GET_FLAGS(mp, ep);
        if (!(me_flags & FLAG_USED))
            return freeslot == NULL ? ep : freeslot;
        me_hash = GET_HASH(mp, ep);
        if (me_hash == hash
                && !(me_flags & FLAG_DUMMY)
                && mp->eqfunc(GET_KEYP(mp, ep), key))
            return ep;
        if ((me_flags & FLAG_DUMMY) && freeslot == NULL)
            freeslot = ep;
    }
    assert(0);          /* NOT REACHED */
    return 0;
}

/*
   Internal routine to insert a new item into the table.
   Used both by the internal resize routine and by the public insert routine.
   Eats a reference to key and one to value.
   Returns -1 if an error occurred, or 0 on success.
   */
    /* static int */
/* insertdict(register OptDict *mp, void *key, long hash, void *value, void *oldvalue) */
/* { */
    /* register OptDictEntry *ep; */
    /* typedef OptDictEntry *(*lookupfunc)(OptDict *, void *, long); */

    /* oldvalue = NULL; */

    /* assert(mp->ma_lookup != NULL); */
    /* ep = mp->ma_lookup(mp, key, hash); */
    /* if (ep == NULL) { */
        /* return -1; */
    /* } */
    /* [> For Python garbage collection tracking <] */
    /* [> MAINTAIN_TRACKING(mp, key, value); <] */
    /* if (ep->me_value != NULL) { */
        /* oldvalue = ep->me_value; */
        /* ep->me_value = value; */
    /* } */
    /* else { */
        /* if (ep->me_key == NULL) */
            /* mp->ma_fill++; */
        /* else { */
            /* assert(ep->me_key == dummy); */
        /* } */
        /* ep->me_key = key; */
        /* ep->me_hash = hash; */
        /* ep->me_value = value; */
        /* mp->ma_used++; */
    /* } */
    /* return 0; */
/* } */

/* Internal routine used by dictresize() to insert an item which is known to be
 * absent from the dict.  This routine also assumes that the dict contains no
 * deleted entries.  Besides the performance benefit, using insertdict() in
 * dictresize() is dangerous (SF bug #1456209).  Note that no refcounts are
 * changed by this routine; if needed, the caller is responsible for incref'ing
 * `key` and `value`. 
 */
    /* static void */
/* insertdict_clean(register OptDict *mp, void *key, long hash, */
        /* void *value) */
/* { */
    /* register size_t i; */
    /* register size_t perturb; */
    /* register size_t mask = (size_t)mp->ma_mask; */
    /* OptDictEntry *ep0 = mp->ma_table; */
    /* register OptDictEntry *ep; */

    /* [> XXX: FIXME: Not Implemented!!! <] */
    /* assert(0); */

    /* [> MAINTAIN_TRACKING(mp, key, value); <] */
    /* i = hash & mask; */
    /* ep = &ep0[i]; */
    /* for (perturb = hash; ep->me_key != NULL; perturb >>= PERTURB_SHIFT) { */
        /* i = (i << 2) + i + perturb + 1; */
        /* ep = &ep0[i & mask]; */
    /* } */
    /* assert(ep->me_value == NULL); */
    /* mp->ma_fill++; */
    /* ep->me_key = key; */
    /* ep->me_hash = hash; */
    /* ep->me_value = value; */
    /* mp->ma_used++; */
/* } */

/* Restructure the table by allocating a new table and reinserting all items
 * again.  When entries have been deleted, the new table may actually be
 * smaller than the old one.  
 */
    /* static int */
/* dictresize(OptDict *mp, size_t minused) */
/* { */
    /* size_t newsize; */
    /* OptDictEntry *oldtable, *newtable, *ep; */
    /* size_t i; */
    /* int is_oldtable_malloced; */
    /* OptDictEntry small_copy[optdict_MINSIZE]; */

    /* [> XXX: FIXME: Not Implemented!!! <] */
    /* assert(0); */

    /* assert(minused >= 0); */

    /* [> Find the smallest table size > minused. <] */
    /* for (newsize = optdict_MINSIZE; */
            /* newsize <= minused && newsize > 0; */
            /* newsize <<= 1) */
        /* ; */
    /* if (newsize <= 0) { */
        /* return ERR_NO_MEM; */
    /* } */

    /* [> Get space for a new table. <] */
    /* oldtable = mp->ma_table; */
    /* assert(oldtable != NULL); */
    /* is_oldtable_malloced = oldtable != mp->ma_smalltable; */

    /* if (newsize == optdict_MINSIZE) { */
        /* [> A large table is shrinking, or we can't get any smaller. <] */
        /* newtable = mp->ma_smalltable; */
        /* if (newtable == oldtable) { */
            /* if (mp->ma_fill == mp->ma_used) { */
                /* [> No dummies, so no point doing anything. <] */
                /* return 0; */
            /* } */
            /* We're not going to resize it, but rebuild the table anyway to
             * purge old dummy entries.  
             * Subtle:  This is *necessary* if fill==size, as lookdict needs at
             * least one virgin slot to terminate failing searches.  If fill <
             * size, it's merely desirable, as dummies slow searches.
             */
            /* assert(mp->ma_fill > mp->ma_used); */
            /* memcpy(small_copy, oldtable, sizeof(small_copy)); */
            /* oldtable = small_copy; */
        /* } */
    /* } */
    /* else { */
        /* newtable = malloc(newsize * sizeof(OptDictEntry)); */
        /* if (newtable == NULL) { */
            /* return ERR_NO_MEM; */
        /* } */
    /* } */

    /* [> Make the dict empty, using the new table. <] */
    /* assert(newtable != oldtable); */
    /* mp->ma_table = newtable; */
    /* mp->ma_mask = newsize - 1; */
    /* memset(newtable, 0, sizeof(OptDictEntry) * newsize); */
    /* mp->ma_used = 0; */
    /* i = mp->ma_fill; */
    /* mp->ma_fill = 0; */

    /* Copy the data over; this is refcount-neutral for active entries;
     * dummy entries aren't copied over, of course 
     */
    /* for (ep = oldtable; i > 0; ep++) { */
        /* if (ep->me_value != NULL) {             [> active entry <] */
            /* --i; */
            /* insertdict_clean(mp, ep->me_key, ep->me_hash, */
                    /* ep->me_value); */
        /* } */
        /* else if (ep->me_key != NULL) {          [> dummy entry <] */
            /* --i; */
            /* assert(ep->me_key == dummy); */
        /* } */
        /* [> else key == value == NULL:  nothing to do <] */
    /* } */

    /* if (is_oldtable_malloced) */
        /* free(oldtable); */
    /* return 0; */
/* } */

/* CAUTION: OptDict_SetItem() must guarantee that it won't resize the
 * dictionary if it's merely replacing the value for an existing key.  This
 * means that it's safe to loop over a dictionary with OptDict_Next() and
 * occasionally replace a value -- but you can't insert new keys or remove
 * them.
 */
    /* int */
/* OptDict_SetItem(register OptDict *mp, void *key, register long hash, void *value, void *oldvalue) */
/* { */
    /* register size_t n_used; */

    /* assert(key); */
    /* assert(value); */
    /* if (hash == -1) */
        /* return -1; */
    /* assert(mp->ma_fill <= mp->ma_mask);  [> at least one empty slot <] */
    /* n_used = mp->ma_used; */
    /* [> XXX: Finish me!!! <] */
    /* if (insertdict(mp, key, hash, value, oldvalue) != 0) */
        /* return -1; */
    /* If we added a key, we can safely resize.  Otherwise just return!
     * If fill >= 2/3 size, adjust size.  Normally, this doubles or
     * quaduples the size, but it's also possible for the dict to shrink
     * (if ma_fill is much larger than ma_used, meaning a lot of dict
     * keys have been deleted).
     *
     * Quadrupling the size improves average dictionary sparseness
     * (reducing collisions) at the cost of some memory and iteration
     * speed (which loops over every possible entry).  It also halves
     * the number of expensive resize operations in a growing dictionary.
     *
     * Very large dictionaries (over 50K items) use doubling instead.
     * This may help applications with severe memory constraints.
     */
    /* if (!(mp->ma_used > n_used && mp->ma_fill*3 >= (mp->ma_mask+1)*2)) */
        /* return 0; */
    /* return dictresize(mp, (mp->ma_used > 50000 ? 2 : 4) * mp->ma_used); */
/* } */
