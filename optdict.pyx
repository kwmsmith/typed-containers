
cdef class OptDict:

    cdef _OptDict *od

    def __cinit__(self):
        self.od = OptDict_New(INT_KEY)

    def __dealloc__(self):
        pass
