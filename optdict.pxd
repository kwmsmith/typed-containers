cdef extern from "optdictgeneric.h":
    
    ctypedef struct _OptDict "OptDict":
        pass

    enum type_enum:
        INT
        LONG
        FLOAT
        DOUBLE

    _OptDict *OptDict_New(int, int)
    # int OptDict_SetItem(_OptDict *mp, void *key, long hash, void *value, void *oldvalue)
    long int_hash(int)
