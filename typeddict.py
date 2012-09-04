def _typecheck(ob, type):
    if not isinstance(ob, type):
        raise TypeError("{} is not a {}".format(ob, type.__name__))
    return True

class TypedDict(object):

    def __init__(self, init=None, keytype=object, valtype=object):
        self.keytype = keytype
        self.valtype = valtype
        self._d = {}
        if init is not None:
            self.update(init)

    def update(self, other, **kwargs):
        try:
            for k in other.keys():
                v = other[k]
                _typecheck(k, self.keytype)
                _typecheck(v, self.valtype)
                self._d[k] = other[k]
        except AttributeError:
            pass
        for (k, v) in other:
            _typecheck(k, self.keytype)
            _typecheck(v, self.valtype)
            self._d[k] = v
        for k in kwargs:
            v = kwargs[k]
            _typecheck(k, self.keytype)
            _typecheck(v, self.valtype)
            self._d[k] = v

    def __repr__(self):
        return "TypedDict(keytype={}, valtype={}, {!r})".format(self.keytype.__name__,
                                                                 self.valtype.__name__,
                                                                 self._d)

    def __getitem__(self, item):
        return self._d[item]

    def __setitem__(self, item, val):
        _typecheck(item, self.keytype)
        _typecheck(val, self.valtype)
        self._d[item] = val

    def __contains__(self, ob):
        return ob in self._d

    def get(self, k, d=None):
        return self._d.get(k, d)

    def setdefault(self, k, d=None):
        if k not in self:
            _typecheck(k, self.keytype)
            _typecheck(d, self.valtype)
        return self._d.setdefault(k, d)

    def clear(self):
        self._d.clear()

    def copy(self):
        new_td = TypedDict(keytype=self.keytype, valtype=self.valtype)
        new_td._d = self._d.copy()
        return new_td

    def items(self):
        return self._d.items()

    def keys(self):
        return self._d.keys()

    def values(self):
        return self._d.values()

    def pop(self, k, d=None):
        return self._d.pop(k, d)

    def popitem(self):
        return self._d.popitem()
