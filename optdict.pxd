cdef extern from "optdictbase.h":
    
    ctypedef struct OptDictEntry:
        pass

    ctypedef struct _OptDict "OptDict":
        pass

    enum key_t:
        INT_KEY
        FLOAT_KEY
        DOUBLE_KEY

    _OptDict *OptDict_New(int)
