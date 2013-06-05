import os
import sys
import argparse

# Add my ./lib folder to search path, so next 3 imports work.
_MYDIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(_MYDIR, 'lib'))

import codebase
import analyze

def main(args):
    cb = codebase.Codebase(args.folder, args.novisit, args.norecurse)
    # Understand the code.
    gist = analyze.Gist(cb)
    for fname in cb.files:
        abspath = cb.root + fname
        parser = analyze.Parser(abspath, cb)
        gist.include(parser)
    # Describe what we learned.
    gist.summarize()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Analyze/fix logging in a codebase.')
    parser.add_argument('--skipd', dest="norecurse", metavar='regex', nargs='?', help="skip dirs (don't recurse) where names match this pattern; default is vcs and test folders", default=None)
    parser.add_argument('--skipf', dest="novisit", metavar='regex', nargs='?', help="skip files that match this pattern; default is anything that's not C/C++ code", default=None)
    parser.add_argument('folder', metavar='root', help="a folder that's the root of a codebase")
    args = parser.parse_args()
    main(args)
