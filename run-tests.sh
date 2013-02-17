#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
(cd ${DIR} && python setup.py build_ext --inplace && nosetests -s -v && python hamt/timing.py)
