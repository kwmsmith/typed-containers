#ifndef OPTDICT_H
#define OPTDICT_H
#ifdef __cplusplus
extern "C" {
#endif

#define optdict_MINSIZE 8
#define ERR_NO_MEM -1

enum type_enum {
    INT,
    LONG,
    FLOAT,
    DOUBLE
};

/* The OptDictEntry struct has the layout indicated in the comment block below,
 * although we cannot declare it as such.  The me_flags field determines
 * whether the element is FLAG_DUMMY or FLAG_EMPTY.  The key_t and val_t are
 * the types of the key and value, resp, and are not necessarily pointers;
 * typically key and value data are copied into the struct key / value fields.
 * This illustrates why it would be prohibitive to enumerate all possible
 * OptDictEntry structs for all combos of key_t and val_t.  In particular,
 * key_t and val_t can be composite types or structures.
 */

/*
 * typedef struct {
 *  long me_hash;
 *  unsigned long me_flags;
 *  key_t me_key;
 *  val_t me_value;
 *  } OptDictEntry;
 */

typedef char OptDictEntry;

typedef struct _optdict OptDict;
struct _optdict {
    size_t ma_fill;  /* # Active + # Dummy */
    size_t ma_used;  /* # Active */

    /* The table contains ma_mask + 1 slots, and that's a power of 2.
     * We store the mask instead of the size because the mask is more
     * frequently needed.
     */
    size_t ma_mask;

    /* ma_table points to ma_smalltable for small tables, else to
     * additional malloc'ed memory.  ma_table is never NULL!  This rule
     * saves repeated runtime null-tests in the workhorse getitem and
     * setitem calls.
     */
    size_t entry_size;
    size_t flags_ofs;
    size_t key_ofs;
    size_t val_ofs;
    OptDictEntry *ma_table;
    OptDictEntry *(*ma_lookup)(OptDict *mp, void *key, long hash);
    int (*eqfunc)(void *, void *);
};

OptDict *OptDict_New(enum type_enum, enum type_enum);

long int_hash(int);

/* [> PyAPI_DATA(PyTypeObject) PyDict_Type; <] */
/* [> PyAPI_DATA(PyTypeObject) PyDictIterKey_Type; <] */
/* [> PyAPI_DATA(PyTypeObject) PyDictIterValue_Type; <] */
/* [> PyAPI_DATA(PyTypeObject) PyDictIterItem_Type; <] */
/* [> PyAPI_DATA(PyTypeObject) PyDictKeys_Type; <] */
/* [> PyAPI_DATA(PyTypeObject) PyDictItems_Type; <] */
/* [> PyAPI_DATA(PyTypeObject) PyDictValues_Type; <] */

/* [> #define PyDict_Check(op) \ <] */
                 /* [> PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_DICT_SUBCLASS) <] */
/* [> #define PyDict_CheckExact(op) (Py_TYPE(op) == &PyDict_Type) <] */
/* [> #define PyDictKeys_Check(op) (Py_TYPE(op) == &PyDictKeys_Type) <] */
/* [> #define PyDictItems_Check(op) (Py_TYPE(op) == &PyDictItems_Type) <] */
/* [> #define PyDictValues_Check(op) (Py_TYPE(op) == &PyDictValues_Type) <] */
/* [> This excludes Values, since they are not sets. <] */
/* [> # define PyDictViewSet_Check(op) \ <] */
    /* [> (PyDictKeys_Check(op) || PyDictItems_Check(op)) <] */

/* PyObject *OptDict_GetItem(OptDictObject *mp, void *key); */
/* int OptDict_SetItem(OptDictObject *mp, void *key, void *item); */
/* int OptDict_DelItem(OptDictObject *mp, void *key); */
/* void OptDict_Clear(OptDictObject *mp); */
/* size_t OptDict_Size(OptDictObject *mp); */
/* int OptDict_Contains(OptDictObject *mp, void *key); */
/* int _OptDict_Contains(OptDictObject *mp, void *key, long hash); */
/* OptDictObject * _PyDict_NewPresized(Py_ssize_t minused); */

/* [> PyAPI_FUNC(int) PyDict_Next( <] */
    /* [> PyObject *mp, Py_ssize_t *pos, PyObject **key, PyObject **value); <] */
/* [> PyAPI_FUNC(int) _PyDict_Next( <] */
    /* [> PyObject *mp, Py_ssize_t *pos, PyObject **key, PyObject **value, long *hash); <] */
/* [> PyAPI_FUNC(PyObject *) PyDict_Keys(PyObject *mp); <] */
/* [> PyAPI_FUNC(PyObject *) PyDict_Values(PyObject *mp); <] */
/* [> PyAPI_FUNC(PyObject *) PyDict_Items(PyObject *mp); <] */
/* [> PyAPI_FUNC(PyObject *) PyDict_Copy(PyObject *mp); <] */
/* [> PyAPI_FUNC(void) _PyDict_MaybeUntrack(PyObject *mp); <] */

/* [> PyDict_Update(mp, other) is equivalent to PyDict_Merge(mp, other, 1). <] */
/* [> PyAPI_FUNC(int) PyDict_Update(PyObject *mp, PyObject *other); <] */

/* PyDict_Merge updates/merges from a mapping object (an object that
   supports PyMapping_Keys() and PyObject_GetItem()).  If override is true,
   the last occurrence of a key wins, else the first.  The Python
   dict.update(other) is equivalent to PyDict_Merge(dict, other, 1).
*/
/* [> PyAPI_FUNC(int) PyDict_Merge(PyObject *mp, <] */
                                   /* [> PyObject *other, <] */
                                   /* [> int override); <] */

/* PyDict_MergeFromSeq2 updates/merges from an iterable object producing
   iterable objects of length 2.  If override is true, the last occurrence
   of a key wins, else the first.  The Python dict constructor dict(seq2)
   is equivalent to dict={}; PyDict_MergeFromSeq(dict, seq2, 1).
*/
/* [> PyAPI_FUNC(int) PyDict_MergeFromSeq2(PyObject *d, <] */
                                           /* [> PyObject *seq2, <] */
                                           /* [> int override); <] */

/* [> PyAPI_FUNC(PyObject *) PyDict_GetItemString(PyObject *dp, const char *key); <] */
/* [> PyAPI_FUNC(int) PyDict_SetItemString(PyObject *dp, const char *key, PyObject *item); <] */
/* [> PyAPI_FUNC(int) PyDict_DelItemString(PyObject *dp, const char *key); <] */

#ifdef __cplusplus
}
#endif
#endif /* !Py_DICTOBJECT_H */

/* vim: set ft=c: */
