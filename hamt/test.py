from hamtpy import hamtpy
# import gc

from time import sleep

LIM = 32**4 + 10

def test_insert():
    h = hamtpy()
    # h[0] = 1
    for i in xrange(LIM):
        h[i] = i

def _test_insert_dict():
    # gc.disable()
    h = {}
    # h[0] = 1
    for i in xrange(LIM):
        h[i] = i
    # gc.enable()


def _test_hash_collision():
    h = hamtpy()
    assert hash(-1) == hash(-2)
    h[-1] = -1
    h[-2] = -2

def test_search():
    h = hamtpy()
    for i in xrange(LIM):
        h[i] = i

    for i in xrange(LIM):
        assert h[i] == i

def _test_search_dict():
    h = {}
    for i in xrange(LIM):
        h[i] = i

    for i in xrange(LIM):
        assert h[i] == i

def _test_insert_empty():
    gc.disable()
    for i in xrange(LIM):
        pass
