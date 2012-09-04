Typed Containers
----------------

Python containers -- lists, tuples, sets, and, especially, dictionaries -- are
awesome.  They rank with Python's syntax, object model, and standard library
as one of the most compelling feature of the language, and they are extremely
well designed.  Because they're used everywhere in the language itself
(especially dicts), they have to be all things to all people, and they
accomplish their task with aplomb.

But...

What if you *know* that you need a dictionary of 32-bit unsigned int keys to
10-character strings, you know from the outset that it will have 512**2
key-value pairs, and you know that its primary use is for creation and a few
iterations, with no deletions?  Can we do better than the one-size-fits-all
option provided by the workhorse `dict()`?

Of course we can.

At the other end of the spectrum, if we have a tiny dictionary of just 8 key /
value pairs (chars to ints, for example) that will be used in an internal
loop, can we optimize it so that the entire dictionary, including the keys /
values, fits in two cache lines?

You bet.

Features and sketchy roadmap
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    * Typing of dictionary keys, values.  What NumPy does for typed arrays, we
      do for the Python dictionary.

    * User-tweakable parameters to control memory usage / sparsity, initial
      dictionary size, and many more,

    * A clean C interface, with no Python dependency.  You can use the
      underlying containers from your favorite systems language without having
      to think about Python refcounting.

    * Clean Python wrappers over the C foundation.  When you ask for something
      from a container, a new Python object is created for you, like Ctypes,
      or NumPy.

    * Optimized memory usage for the base fixed-size types.  Dictionaries of
      uint32 -> float64 will use just 16 bytes per key / value pair on a
      32-bit system.  For fixed-sized element types, key / values are stored
      in the dictionary itself; they are *not* stored in the dictionary as
      pointers.  This is to save pointer dereferencing, improve memory
      locality, and to reduce memory usage.

Some possible avenues to explore:

    * making a dictionary readonly, and the optimizations that a readonly dict
      makes possible.

    * Different collision behavoir to reduce cache misses (sparsity is a
      factor here as well).
