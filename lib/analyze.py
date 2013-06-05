import os
import operator
import re

# This pattern only finds "MLog(", optionally preceded by "MDB(...)". However, the MDB
# part is only included if the two calls are only separated by whitespace. If there
# is an MDB() separated from MLog by other statements, or by a curly brace, the MLog
# call will be found, but it will be treated as if it had no MDB(...) prefix.
_LOG_PAT = re.compile(r'((\bMDB[EO]?\s*\(([^\)]*?)\))[ \r\n]*|\b)MLog\s*\(', re.DOTALL)
_LBL_PAT = re.compile(r'\(\s*(.*?)[%\t, :]')
_CANONICAL_LBL_PAT = re.compile('[A-Z]+$')

class Gist:
    '''Understand source code as a whole. Support transforms or reports.'''
    def __init__(self, codebase):
        self.codebase = codebase
        self.file_count = 0
        self.files = {}
        
    def include(self, parser):
        '''Add another file to the scope of our analysis.'''
        self.file_count += 1
        if parser.logcalls:
            self.files[parser.path] = parser
            
    def summarize(self):
        print
        print('%s: %d' % (_get_label('# files'), self.file_count))
        print('%s: %d' % (_get_label('# files calling MLog'), len(self.files)))
        
        # Compile some data.
        parsers = sorted([self.files[p] for p in self.files], key=lambda x:len(x.logcalls), reverse=True)
        by_dc = {}
        by_fac = {}
        by_lvl = {}
        by_ff = {}
        for parser in parsers:
            _count_dicts(parser, by_dc, by_fac, by_lvl, by_ff)
            
        _print_desc(by_dc, '# calls to log %s')
        _print_desc(by_fac, '# calls to facility %s')
        _print_desc(by_lvl, '# calls with level %s')
        _print_desc(by_ff, '# calls filtered by %s')
        
        print
        print("Top 10 files:")
        root = parsers[0].codebase.root
        rootlen = len(root)
        pcount = 0
        for f in parsers:
            pcount += 1
            if pcount > 10:
                break
            relpath = f.path[rootlen:]
            print('  %s: %d' % (relpath, len(f.logcalls)))
            for cat in f.by_described_category:
                n = len(f.by_described_category[cat])
                print('    %s: %d' % (cat, n))
        
class LogCall:
    '''
    Record info about a call to MLog()
    '''
    def __init__(self, txt, match):
        self.txt = txt
        self.begin = match.start()
        # Find all args, and the end of the MLog statement.
        self._parse()
        self.level = None
        self.facility = None
        self.filter_func = None
        self.mdb = match.group(2)
        if self.mdb:
            #print self.mdb
            i = self.mdb.find('(')
            self.filter_func = self.mdb[0:i].strip()
            mdbargs = match.group(3).strip()
            if mdbargs:
                mdbargs = [x.strip() for x in mdbargs.split(',')]
                assert(len(mdbargs) > 1)
                self.level = mdbargs[0]
                self.facility = mdbargs[-1]
            else:
                print('No args found')
        else:
            pass #print('No MDB: <<<%s>>>' % self.txt[self.begin:self.end])
        self._dc = None
        
    def _parse(self):
        args = []
        begin = self.txt.find('MLog', self.begin)
        begin = self.txt.find('(', begin) + 1
        inQuote = False
        parenCount = 1
        for i in xrange(begin, len(self.txt)):
            c = self.txt[i]
            if c == '"':
                inQuote = not inQuote
            elif c == '\\':
                if inQuote:
                    i += 1
            elif c == '(':
                if not inQuote:
                    parenCount += 1
            elif c == ')':
                if not inQuote:
                    parenCount -= 1
                    if parenCount == 0:
                        self.end = self.txt.find(';', i + 1)
                        break
            elif c == ',':
                if not inQuote:
                    if parenCount == 1:
                        args.append((begin,i))
                        begin = i + 1
                        if len(args) > 20:
                            print('Suspicious parse of MLog; falling back to dumb parse')
                            print('First 200 chars: %s' % self.txt[self.begin:self.begin + 200])
                            self.end = self.txt.find(';', self.begin)
                            break
        self.args = args
        
    @property
    def described_category(self):
        if self._dc is None:
            self._dc = '?'
            m = _LBL_PAT.search(self.txt, self.txt.find('MLog', self.begin))
            if m and m.start() < self.end:
                lbl = m.group(1)
                if lbl.startswith('"'):
                    lbl = lbl[1:]
                    if lbl and _CANONICAL_LBL_PAT.match(lbl):
                        self._dc = lbl
        return self._dc
            
class Parser:
    '''
    Parse a complete file and extract any calls to MLog.
    '''
    def __init__(self, path, codebase=None):
        self.codebase = codebase
        self.path = path
        f = open(path, 'r')
        txt = f.read()
        f.close()
        self._parse(txt)

    def _parse(self, txt):
        self.logcalls = []
        by_fac = {}
        by_dc = {}
        by_lvl = {}
        by_ff = {}
        for match in _LOG_PAT.finditer(txt):
            lc = LogCall(txt, match)
            if not by_dc.has_key(lc.described_category):
                by_dc[lc.described_category] = []
            by_dc[lc.described_category].append(lc)
            if not by_fac.has_key(lc.facility):
                by_fac[lc.facility] = []
            by_fac[lc.facility].append(lc)
            if not by_lvl.has_key(lc.level):
                by_lvl[lc.level] = []
            by_lvl[lc.level].append(lc)
            if not by_ff.has_key(lc.filter_func):
                by_ff[lc.filter_func] = []
            by_ff[lc.filter_func].append(lc)
            self.logcalls.append(lc)
        self.by_described_category = by_dc
        self.by_facility = by_fac
        self.by_level = by_lvl
        self.by_filter_func = by_ff

def _get_label(lbl):
    return lbl.ljust(30)

def _count_dict(src_dict, count_dict):
    for key in src_dict:
        if not count_dict.has_key(key):
            count_dict[key] = 0
        n = len(src_dict[key])
        count_dict[key] += n
        
def _count_dicts(parser, by_dc, by_fac, by_lvl, by_ff):
    _count_dict(parser.by_described_category, by_dc)
    _count_dict(parser.by_facility, by_fac)
    _count_dict(parser.by_level, by_lvl)
    _count_dict(parser.by_filter_func, by_ff)
    
def _print_desc(unsorted, lbl):
    print
    x = sorted(unsorted.iteritems(), key=operator.itemgetter(1), reverse=True)
    for tuple in x:
        print('%s: %d' % (_get_label(lbl % tuple[0]), tuple[1]))

