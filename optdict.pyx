
cdef class OptDict:

    cdef _OptDict *od

    def __cinit__(self):
        self.od = OptDict_New()

    def __dealloc__(self):
        pass
