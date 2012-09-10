cdef extern from "optdictbase.h":
    
    ctypedef struct OptDictEntry:
        pass

    ctypedef struct _OptDict "OptDict":
        pass

    _OptDict *OptDict_New()
