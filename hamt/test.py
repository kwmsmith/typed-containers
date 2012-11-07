from hamtpy import hamtpy
# import gc

def _test_insert():
    h = hamtpy()
    # h[0] = 1
    for i in xrange(32**4 + 10):
        h[i] = 'a'

def test_search():
    h = hamtpy()
    for i in xrange(32**4):
        # ch = chr((i % 26) + ord('a'))
        h[i] = 'a'

    for i in xrange(32**4):
        assert h[i] == 'a'

def _test_search_dict():
    h = {}
    for i in xrange(32**4):
        # ch = chr((i % 26) + ord('a'))
        h[i] = 'a'

    for i in xrange(32**4):
        assert h[i] == 'a'

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
