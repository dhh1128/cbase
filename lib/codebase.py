import os
import ioutil
import re
import sys

VCS_FOLDER_PAT = re.compile('\.(hg|bzr|svn|git)$')
TEST_FOLDER_PAT = re.compile('tests?$', re.IGNORECASE)
C_EXTS_PAT = re.compile('.*\.(([hc](pp|xx)?)|cc)$', re.IGNORECASE)
HEADER_EXTS_PAT = re.compile('.*\.(([h](pp|xx)?)|cc)$', re.IGNORECASE)

_REQUIRED_FILES = ['Makefile', 'Makefile.am', 'configure', 'configure.ac']
def is_root(folder):
    found = False
    for f in _REQUIRED_FILES:
        if os.path.isfile(os.path.join(folder, f)):
            found = True
            break
    if found:
        items = os.listdir(folder)
        return ('src' in items or 'include' in items) and ('.git' in items or '.hg' in items or '.svn' in items)
    return False

def find_root(startingFrom):
    if not startingFrom:
        startingFrom = os.getcwd()
    head = os.path.abspath(startingFrom)
    while True:
        if is_root(head):
            return head
        if not head or head == '/':
            print("Couldn't find codebase root from %s." % startingFrom)
            sys.exit(1)
        print('couldnt find it at %s' % head)
        head, tail = os.path.split(head)
        
def only_header_files(fname):
    return bool(HEADER_EXTS_PAT.match(fname))

def only_cxx_files(fname):
    return bool(C_EXTS_PAT.match(fname))

def final_folder(folder):
    folder = ioutil.norm_seps(folder)
    if folder.endswith('/'):
        folder = folder[0:-1]
    return folder.split('/')[-1]

def skip_tests(folder):
    return not bool(TEST_FOLDER_PAT.match(final_folder(folder)))

def skip_vcs(folder):
    '''
    Return true for all folders except .hg, .git, .svn.
    '''
    folder = final_folder(folder)
    return not bool(VCS_FOLDER_PAT.match(folder))

def skip_tests_and_vcs(folder):
    folder = final_folder(folder)
    return not bool(TEST_FOLDER_PAT.match(folder)) and not bool(VCS_FOLDER_PAT.match(folder))

class Codebase:
    '''
    Make it easy to enumerate files in a particular codebase.
    '''
    def __init__(self, path, recurse_filter=skip_vcs, visit_filter=only_cxx_files):
        path = find_root(path)
        # Traverse codebase to enumerate files that need processing.
        self.root = ioutil.norm_folder(path)
        self.by_folder = {}
        self.by_ext = {}
        self._discover(recurse_filter, visit_filter)
        
    def abspath(self, fname):
        return self.root + fname
    
    @property
    def files(self):
        for folder in self.by_folder.iterkeys():
            for item in self.by_folder[folder]:
                if not item.endswith('/'):
                    yield item
    
    def _discover(self, recurse_filter, visit_filter):
        for root, dirs, files in os.walk(self.root):
            root = ioutil.norm_folder(root)
            relative_root = root[len(self.root):]
            items = []
            for d in dirs[:]:
                if recurse_filter and (not recurse_filter(relative_root + d)):
                    dirs.remove(d)
                else:
                    items.append(ioutil.norm_seps(relative_root + d, trailing=True))
            for f in files:
                if C_EXTS_PAT.match(f):
                    if (not visit_filter) or visit_filter(f):
                        fname, ext = os.path.splitext(f)
                        if not ext in self.by_ext:
                            self.by_ext[ext] = []
                        self.by_ext[ext].append(relative_root + f)
                        items.append(relative_root + f)
            self.by_folder[relative_root] = items
            

