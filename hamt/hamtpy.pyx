from cpython.ref cimport Py_INCREF

cdef int eq_func(void *o1, void *o2):
    if <object>o1 == <object>o2:
        return 1
    return 0

cdef void deletefunc(void *data):
    pass

cdef unsigned long python_hash(void *vv):
    # XXX: FIXME: exception?
    cdef object key = <object>vv
    return PyObject_Hash(key)

cdef class hamtpy:

    def __cinit__(self):
        self._thisptr = HAMT_new()
        if not self._thisptr:
            raise MemoryError("Unable to create a new HAMT object")

    def __setitem__(self, key, value):
        Py_INCREF(key)
        Py_INCREF(value)
        HAMT_insert(self._thisptr,
                <void*>key, <void*>value, 
                python_hash,
                eq_func,
                deletefunc)

    def __dealloc__(self):
        if self._thisptr:
            HAMT_delete(self._thisptr, deletefunc)
            self._thisptr = NULL
