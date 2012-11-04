from cpython.object cimport PyObject_Hash

cdef extern from "hamt.h":
    ctypedef struct HAMT:
        pass
    HAMT *HAMT_new()

    void * HAMT_insert(HAMT *hamt, void *key, 
        void *value, int *replace,
        unsigned long (*hash_func)(void *),
        int (*eq_func)(void *, void *),
        void (*deletefunc)(void *))

cdef class hamtpy:
    cdef HAMT *_thisptr

cdef unsigned long python_hash(void *)
