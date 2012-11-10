from cpython.object cimport PyObject_Hash

cdef extern from "hamt.h":
    ctypedef struct HAMT:
        pass

    ctypedef struct HAMT_entry:
        void *key
        void *value

    HAMT *HAMT_new()

    void * HAMT_insert(HAMT *hamt, void *key, void *value,
            unsigned long (*hash_func)(void *),
            int (*eq_func)(void *, void *),
            void (*deletefunc)(void *))

    void HAMT_delete(HAMT *, void (*deletefunc)(void *data))

    HAMT_entry *HAMT_search(HAMT *hamt, void *key, 
            unsigned long (*hash_func)(void *), 
            int (*eq_func)(void *, void *))

cdef class hamtpy:
    cdef HAMT *_thisptr

cdef unsigned long python_hash(void *)
