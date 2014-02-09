from libc.stdlib cimport malloc, free
from cpython.ref cimport Py_INCREF, Py_DECREF

cdef class OptDict:

    cdef _OptDict *od

    def __cinit__(self):
        self.od = OptDict_New(INT_KEY)

    def __setitem__(self, key, value):
        # XXX: BUG: if key is alreay in the dictionary, then we just replace
        # value, and do not modify key.  In this case, do not malloc new memory
        # and store a copy of the key's contents!  Should OptDict_SetItem() be
        # responsible for allocating new memory and storing the result?
        cdef int int_key = key
        cdef int *int_key_p = <int*>malloc(sizeof(int))
        cdef object oldvalue = None
        if not int_key_p:
            raise RuntimeError("")
        # XXX: BUG: *int_key_p != int_key (!)  How to do the copy in Cython
        # without a temp variable?
        cdef int err = OptDict_SetItem(self.od, int_key_p, int_hash(int_key),
                <void*>value, <void*>oldvalue)
        if err:
            raise RuntimeError("")
        Py_INCREF(value)
        if oldvalue:
            Py_DECREF(oldvalue)

        # err = OptDict_SetItem(self.od, )
    # int OptDict_SetItem(OptDict *mp, void *key, long hash, void *value, void *oldvalue)

    def __dealloc__(self):
        pass
