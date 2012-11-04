top = '.'
out = 'build'

def options(ctx):
    ctx.load('compiler_c')
    ctx.load('python')
    ctx.load('cython')

def configure(ctx):
    ctx.load('compiler_c')
    ctx.load('python')
    ctx.check_python_headers()
    ctx.load('cython')

    ctx.env.CFLAGS += '-Wall -O3 -pedantic -std=c99'.split()

def build(ctx):
    # ctx(features = 'c cshlib',
        # source = 'optdictbase.c',
        # target = 'optdictbase',
        # includes = 'optdictbase')

    ctx(features = 'c cshlib pyext',
        source = 'optdictgeneric.c optdict.pyx',
        target = 'optdict',
        includes = '. ..',
        )

    ctx.recurse('hamt')

def test(ctx):
    print ctx, dir(ctx)

# vim:ft=python
