from hamtpy import hamtpy

def test_insert():
    h = hamtpy()
    # h[0] = 1
    for i in range(32**4):
        h[i] = 'a'
