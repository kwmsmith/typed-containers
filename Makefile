all:
	python setup.py build_ext --inplace

clean:
	-rm -rf build optdict.c optdict.so