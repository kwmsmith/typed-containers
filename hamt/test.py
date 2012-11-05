from hamtpy import hamtpy
# import gc

def test_insert():
    h = hamtpy()
    # h[0] = 1
    for i in xrange(32**4 + 10):
        h[i] = 'a'

def _test_insert_dict():
    # gc.disable()
    h = {}
    # h[0] = 1
    for i in xrange(32**4 + 10):
        h[i] = 'a'
    # gc.enable()

def _test_insert_empty():
    gc.disable()
    for i in xrange(32**4 + 10):
        pass
