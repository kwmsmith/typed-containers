from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

exts = [Extension("optdict", ["optdict.pyx", "optdictbase.c"]),
        Extension("hamt.hamtpy", ["hamt/hamtpy.pyx", "hamt/hamt.c"])]

setup(
    cmdclass = {'build_ext': build_ext},
    ext_modules = exts,
)
