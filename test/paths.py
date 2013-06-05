import os
import sys

TESTDIR = os.path.dirname(os.path.normpath(__file__))
LIBDIR = os.path.normpath(os.path.join(TESTDIR, '..', 'lib'))
if LIBDIR not in sys.path:
    sys.path.append(LIBDIR)
