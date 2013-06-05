import os
import sys
import argparse

# Add my ./lib folder to search path, so next 3 imports work.
_MYDIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(_MYDIR, 'lib'))

from codebase import *
from cparser import *
import analyze

def find_decl(name):
    collection = 'udt'
    if '(' in name:
        collection = 'function'
        name = name[0:name.find('(')]
    cb = Codebase(visit_filter=only_header_files, recurse_filter=skip_tests_and_vcs)
    found = []
    for f in cb.files:
        parsed = ParsedFile(f, cb)
        if name in parsed[collection]:
            print 'found name'

def test(args):
    find_decl('SetAccountAttrOnJob()')

if __name__ == '__main__':
    args = sys.argv
    func = globals()[args[1]]
    func(args)
