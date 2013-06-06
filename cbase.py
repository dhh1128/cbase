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
    category = 'udt'
    if '(' in name:
        category = 'prototype'
        name = name[0:name.find('(')]
    cb = Codebase(path='../torque/4.2-EA', visit_filter=only_header_files, recurse_filter=skip_tests_and_vcs)
    found = []
    for f in cb.files:
        parsed = ParsedFile(cb.abspath(f), cb)
        #print('%s has %d blocks' % (f, len(parsed.blocks)))
        for block in parsed.blocks_by_category(category):
            if block.name == name:
                found.append(block)
    for block in found:
        if block.parent:
            print('parent = %s' % block.parent)
            relpath = block.parent.relpath
        else:
            relpath = 'unknown'
        print 'found %s for %s at %s line %d' % (block.category, block, relpath, block.at_line_num)

def test(args):
    find_decl('getpwnam_ext()')

if __name__ == '__main__':
    args = sys.argv
    func = globals()[args[1]]
    func(args)
