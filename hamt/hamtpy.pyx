from cpython.ref cimport Py_INCREF, Py_DECREF

cdef int eq_func(void *o1, void *o2):
    if <object>o1 == <object>o2:
        return 1
    return 0

cdef inline void deletefunc(void *data):
    Py_DECREF(<object>data)

cdef unsigned long python_hash(void *vv):
    # XXX: FIXME: exception?
    return PyObject_Hash(<object>vv)

cdef unsigned long hash_int(void *vv):
    cdef object k = <object>vv
    cdef long i = k
    return <unsigned long>i

cdef class hamtpy:

    def __cinit__(self):
        self._thisptr = HAMT_new()
        if not self._thisptr:
            raise MemoryError("Unable to create a new HAMT object")

    def __getitem__(self, key):
        cdef HAMT_entry *res = NULL
        res = HAMT_search(self._thisptr,
                <void *>key, hash_int, eq_func)
        if <void*>res == NULL:
            raise KeyError()
        return <object>res.value

    def __setitem__(self, key, value):
        Py_INCREF(key)
        Py_INCREF(value)
        HAMT_insert(self._thisptr,
                <void*>key, <void*>value, 
                hash_int,
                eq_func,
                deletefunc)

    def __dealloc__(self):
        if self._thisptr:
            HAMT_delete(self._thisptr, deletefunc)
            self._thisptr = NULL
