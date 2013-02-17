from hamtpy import hamtpy
from time import clock

def time_insert(lim, d):
    h = d()
    tic = clock()
    for i in xrange(lim):
        h[i] = i
    tic = clock() - tic
    return tic

def time_search(lim, d):
    h = d()
    for i in xrange(lim):
        h[i] = i

    tic = clock()
    for i in xrange(lim):
        assert h[i] == i
    return clock() - tic

if __name__ == '__main__':
    LIM = 32**4 + 10

    dict_insert = time_insert(LIM, dict)
    hamt_insert = time_insert(LIM, hamtpy)

    dict_search = time_search(LIM, dict)
    hamt_search = time_search(LIM, hamtpy)

    print "Size: {}".format(LIM)
    print "         {:10s}        {:10s}".format('dict(ms)',  'hamt(ms)')
    print "Insert:  {:^10.0f}       {:^10.0f}".format(1000 * dict_insert, 1000 * hamt_insert)
    print "Search:  {:^10.0f}       {:^10.0f}".format(1000 * dict_search, 1000 * hamt_search)
