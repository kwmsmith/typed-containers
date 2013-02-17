TODO
------------------------------------------------------------------------------

Declare a static array in the HAMT structure for the root hash table.

More intelligent memory management; reduce the number of calls to malloc.

Try using inner while loops during insert / search:

    * while descending sub-tries and bitmask is 0 -- common case for tries
      with many elements.

    * when there is a hash collision during insert and are creating new sub
      tries, inserting existing hash, and comparing again for a collision.

      Can remove extra ``IF_KEY_VAL_SLOT`` tests and extra hash computations.

Implement own hash function, with extra integer argument.  Constraints:

    * (Normal hash function constraint): If key1 == key2, then hash(key1, i)
      == hash(key2, i) for all integers i.

    * ("Infinite entropy" property) For key, i, j, if i != j, then hash(key,
      i) != hash(key, j).
