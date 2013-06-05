'''
Formal parsing of C and C++ might be complicated, but doable -- if it weren't for all the
old and new language standards, and the non-standard compiler extensions, and the
preprocessor.

This class doesn't pretend to do perfect parsing, but it is relatively simply and robust.
It's good at identifying code blocks (classes, structs, function declarations and 
definitions). It handles comments (both C and C++) and string literals without getting
confused, even with some pretty gnarly corner cases.

It does not handle #ifdefs very well. It recognizes them, but it assumes that you will
use them in a way that doesn't cross block boundaries -- either wholly inside a function,
wholly outside a function, or wholly around a function, for example. If you put the end
of one function and the beginning of another inside a single #ifdef block, it will
become inaccurate. Certain unusual macro constracts may cause problems as well.

Basic usage:

Create an instance of ParsedFile(path, codebase), then access its properties to
understand the structure in the file.
'''

import re
import sys
import weakref

_DEFINE_PAT = re.compile(r'#\s*define\s+.*')
_ELSE_PAT = re.compile(r'#\s*else')
_ENDIF_PAT = re.compile(r'^#\s*endif', re.MULTILINE)
_ID = '[_a-zA-Z][_a-zA-Z0-9:]*'
_UDT_PAT = re.compile(r'(typedef\s+)?(struct|class)\s+(' + _ID + ')\s*(?::.*)?$', re.MULTILINE)
_FUNC_PAT = re.compile(r'((?:' + _ID + ')[^-()=+!<>/|^]*?(?:\s+|\*|\?))(' + _ID + ')\s*\(([^()]*?)\)(\s*const)?\s*$')
_CTOR_PAT = re.compile(r'(' + _ID + ')\s*\(([^()]*?)\)\s*(?::.*)$')
_DTOR_PAT = re.compile(r'(?:virtual\s+)?~(' + _ID + ')\s*\(\s*(?:void\s*)?\)\s*$')
_THROW_PAT = re.compile(r'\s+throw\s*\(.*\)\s*$', re.DOTALL)
_PUBPRIVPROT_PAT = re.compile(r'(public|private|protected)\s*:\s*')
_RUN_OF_WHITESPACE_PAT = re.compile('\s+')
_RESERVED_WORDS_PAT = re.compile(r'(for|do|while|if|else|switch|case|namespace)[^_a-zA-Z0-9]')

'''Turn this on to spit out verbose messages as code is parsed.'''
enable_debug = False

def debug(msg):
    if enable_debug:
        sys.stdout.write(msg + '\n')
        sys.stdout.flush()
        
def norm_param(param):
    '''Convert a parameter to a canonical format.'''
    param = param.replace('*', ' * ').replace('&', ' & ')
    param = _RUN_OF_WHITESPACE_PAT.sub(' ', param).strip()
    param = param.replace('* *', '**')
    return param

class BlockInfo:
    '''
    Hold info about a block of code -- a function, class/struct, conditional, namespace, etc.
    After parsing, the parser object contains various collections of these objects, or of
    objects that derive from this class.
    '''
    def __init__(self, parent, txt, line, begin, end, category='unnamed'):
        self.at_line_num = line
        self.txt = txt
        self.begin = begin
        self.end = end
        self.blocks = []
        self.category = category
        self._flc = -1
        # Is there a sibling block, immediately preceding this one, that documents
        # this block?
        self.block_comment = None
        if parent:
            self._parent = weakref.ref(parent)
            self.depth = 1 + parent.depth
        else:
            self._parent = None
            self.depth = 0
            
    @property
    def full_line_count(self):
        '''How many full lines (lines ending with \n') are contained in this block?'''
        if self._flc == -1:
            self._flc = self.txt.count('\n', self.begin, self.end)
        return self._flc
    
    @property
    def end_line_num(self):
        return self.at_line_num + self.full_line_count
            
    @property
    def parent(self):
        if self._parent:
            return self._parent()
        return None
    
    def blocks_by_category(self, category):
        '''Iterate all child blocks that have a particular category.'''
        for block in self.blocks:
            if block.category == category:
                yield block
    
    @property
    def most_distant_descendants(self):
        '''Which descendants beneath me are nested most deeply?'''
        mdd = []
        max_depth = 0
        for child in self.blocks:
            child_mdd = child.most_distant_descendants
            if child_mdd:
                this_max_depth = child_mdd[0].depth
                if this_max_depth > max_depth:
                    mdd = child_mdd
                    max_depth = this_max_depth
                elif this_max_depth == max_depth:
                    mdd.extend(child_mdd)
        return mdd
    
    def header(self):
        return '%s at %s, line %s.' % (self.category, self.path, self.full_line_count)
    
    def skeleton(self):
        indent = ' ' * (2 * self.depth)
        sk = self.header()
        if self.blocks:
            sk += ' {\n'
            for b in self.blocks:
                sk += b.skeleton()
            sk += '\n' + indent + '}'
        else:
            sk += ' {...}'
        sk = indent + sk
        return sk

class ParsedBlockInfo(BlockInfo):
    '''
    A block of code that parses itself, instead of being wholly 
    created by the parent's parse.
    '''
    def __init__(self, name, parent, txt, line, begin, category):
        BlockInfo.__init__(self, parent, txt, line, begin, len(txt), category)
        self.name = name
        self.preceding_block_comment = None
        self._idx = begin
        self._line = line
        self._cc = 0
        self._cbytes = 0

    def complain(self, msg, severity='Error'):
        sys.stderr.write('%s, line %s: %s. %s\n' % (self.path, self._line, severity, msg))

    def multiline_comment(self):
        '''
        Consume till end of current string literal.
        @precondition ._idx points to first char after opening /*.
        @return True if comment ends normally, False if ends prematurely.
        '''
        debug('multiline_comment at %s (line %s)' % (self._idx, self._line))
        i = self.txt.find('*/', self._idx)
        if i == -1:
            self._idx = self.end
            self.complain('File ended before comment ended.')
            return False
        comment = BlockCommentInfo(parent=self, txt=self.txt, line=self._line, begin=self._idx - 2, end=i + 2)
        self.add_block(comment)
        self._idx = comment.end
        return True
    
    def string_literal(self):
        '''
        Consume till end of current string literal.
        @precondition ._idx points to first char after opening double quote.
        @return True if string literal ends normally, False if ends prematurely.
        '''
        assert self.txt[self._idx - 1] == '"'
        self._last_block_comment = None
        i = self._idx
        debug('string_literal at %s (line %s): <...%s...>' % (self._idx, self._line, self.txt[self._idx-3:self._idx+3]))
        debug('i = %d, self.end = %d' % (i, self.end))
        while i < self.end:
            c = self.txt[i]
            if c == '\\':
                i += 1
            elif c == '\r' or c == '\n':
                self.complain('Line ended before string literal ended.')
                self._idx = i
                return False
            elif c == '"':
                self.erase_string_literal(self._idx - 1, i + 1)
                self._idx = i + 1
                return True
            i += 1
        return False       
    
    def add_block(self, block):
        debug('adding block %s at %s (Im at line %s, block starts at line %s and ends at %s)' % (
            str(block), self._idx, self._line, block.at_line_num, block.end))
        if block.category == 'comment':
            self._last_block_comment = block
        else:
            if self._last_block_comment:
                if block.depth == self._last_block_comment.depth:
                    block.preceding_block_comment = weakref.ref(self._last_block_comment)
                self._last_block_comment = None
            self.blocks.append(block)
        self._idx = block.end
        if isinstance(block, ParsedBlockInfo):
            self._line = block._line
        else:
            self._line += block.full_line_count
               
    def finish_line(self):
        '''
        Advance past next newline, if found, or to end of string, if not.
        If newline found, increment line_count and remember that we're
        now at the begin of a line again.
        @return True if newline found.
        '''
        #debug('finish_line at %s (line %s)' % (self._idx, self._line))
        i = self.txt.find('\n', self._idx)
        if i == -1:
            self._idx = self.end
            return False
        self._idx = i + 1
        self._line += 1
        return True
        
    def preproc_directive(self):
        '''
        Advance past #include, #define, #pragma, #if, #else, #endif, and so forth.
        If newline found, increment line_count and remember that we're
        now at the beginning of a line again.
        @precondition ._idx points to #
        @return True if directive ended with newline
        '''
        #debug('preproc_directive at %s (line %s)' % (self._idx, self._line))
        try:
            fragment = self.txt[self._idx:self._idx + 20]
            if _DEFINE_PAT.match(fragment):
                # Look for line continuation (possibly followed by whitespace) at end of #define ...
                another = True
                while another:
                    i = self._idx
                    if not self.finish_line():
                        return False
                    # Position on char before \n
                    j = self._idx - 2
                    while j > i:
                        c = self.txt[j]
                        if c == '\\':
                            break
                        # Handle trailing spaces
                        elif not c.isspace():
                            another = False
                            break
                        j -= 1
                return True
            elif _ELSE_PAT.match(fragment):
                # This gets a bit tricky. It's possible to introduce uneven nesting of { ... } with #ifdef's.
                # For example, you could have code that looks like this:
                #     if (something)
                #     #ifdef FOO
                #        { ...
                #     #else
                #        { ...
                #     endif
                #     }
                # The *right* way to accomodate this problem is to analyze each possible expansion of the
                # code after running a preprocessor through it. This seems prohibitively complicated. A
                # simpler way is to eliminate the #else branch of any #if, on the theory that it won't
                # change our analysis of sub-blocks in any material way. We're doing that here.
                m = _ENDIF_PAT.search(self.txt, self._idx)
                if m:
                    self.erase_block(self._idx, m.end())
                else:
                    self.complain('#else without matching #endif')
                    self.finish_line()
            else:
                return self.finish_line()
        finally:
            # Technically, this is incorrect. It's entirely possible to have a #if ... #else ... #endif right
            # in the middle of an expression. However, this complicates parsing so much that I'm going to
            # ignore it for now.
            self._expr_begin = -1
        
    def anonymous_block(self):
        #debug('anonymous_block at %s (line %s)' % (self._idx, self._line))
        block = ParsedBlockInfo(name='', parent=self, txt=self.txt, line=self._line, begin=self._idx, category='unnamed')
        block.parse(self.txt, self._idx)
        self._last_block_comment = None
        self.add_block(block)
        
    def header(self):
        h = '%s "%s" at %s, line %s' % (self.category, self.name, self.path, self.at_line_num)
        if self.preceding_block_comment:
            h = '/* ... */\n' + h
        return h
    
    def __str__(self):
        return self.header()
    
    @property
    def comment_count(self):
        cc = self._cc
        for b in self.blocks:
            if hasattr(b, 'comment_count'):
                cc += b.comment_count
        return cc
    
    @property
    def comment_bytes(self):
        cb = self._cbytes
        for b in self.blocks:
            if hasattr(b, 'comment_bytes'):
                cb += b.comment_bytes
        return cb
    
    def erase_comment(self, begin, end):
        self._cc += 1
        debug('adding another comment; count is now %s' % self._cc)
        self.erase_block(begin, end)
    
    def erase_block(self, begin, end):
        '''Remove a block (e.g., comment, #else block) so we can parse with confidence.'''
        cbytes = end - begin
        self._cbytes += cbytes
        line_count = self.txt.count('\n', begin, end)
        spaces = (' ' * (cbytes - line_count)) + ('\n' * line_count)
        before_len = len(self.txt)
        x = self.txt[0:begin] + spaces + self.txt[end:]
        assert len(x) == before_len
        self.txt = x
        
    def erase_string_literal(self, begin, end):
        '''Remove comments so we can use regexes with confidence.'''
        before_len = len(self.txt)
        x = self.txt[0:begin] + ('x'*(end - begin)) + self.txt[end:]
        assert len(x) == before_len
        self.txt = x
        
    @property
    def path(self):
        p = self.parent
        if p:
            return p.path
        return self.name
    
    @property
    def relpath(self):
        p = self.parent
        if p:
            return p.relpath
    
    def block_header(self, header_line):
        try:
            header = self.txt[self._expr_begin:self._idx]
            if _RESERVED_WORDS_PAT.match(header):
                debug('found reserved word in "%s"; returning' % header[0:min(10, len(header))])
                return
            header = _THROW_PAT.sub('', header)
            header = _PUBPRIVPROT_PAT.sub('', header)
            debug('block_header "%s" from %s to %s (ending on line %s)' % (header, self._expr_begin, self._idx, self._line))
            i = header.find('(')
            if i > 0:
                m = _FUNC_PAT.match(header)
                postfix = ''
                if m:
                    returned = norm_param(m.group(1))
                    func_name = m.group(2)
                    params = [norm_param(p) for p in m.group(3).split(',')]
                    if m.group(4):
                        postfix = ' const'
                else:
                    m = _DTOR_PAT.match(header)
                    if m:
                        returned = '<dtor>'
                        func_name = '~' + m.group(1)
                        params = []
                    else:
                        m = _CTOR_PAT.match(header)
                        if m:
                            returned = '<ctor>'
                            func_name = m.group(1)
                            params = [p.strip() for p in m.group(2).split(',')]
                if m:
                    debug('%s %s(%s)%s' % (returned, func_name, ', '.join(params), postfix))
                    if len(params) == 1 and params[0] == 'void':
                        params = []
                    func = FuncInfo(returned, func_name, params, postfix, self, 
                                    self.txt, self._expr_begin, header_line, 
                                    self._line, self._idx + 1)
                    func.at_line_num = header_line
                    self.add_block(func)
                    return func_name
            else:
                m = _UDT_PAT.match(header)
                if m:
                    variant = m.group(2)
                    name = m.group(3)
                    debug('%s %s' % (variant, name))
                    udt = UdtInfo(variant, name, self, self.txt, 
                                  self._expr_begin, header_line, self._line,
                                  self._idx + 1)
                    self.add_block(udt)
                    return m.group(2)
        finally:
            self._expr_begin = -1
                    
    def parse(self, txt, idx=0, line=0):
        self.end = len(txt)
        self.txt = txt
        self._expr_begin = -1
        self._idx = idx
        self._last_block_comment = None
        if line > 0:
            self._line = line
        header_line = -1

        try:
            end = len(self.txt)
            while self._idx < end:
                i = self._idx
                c = self.txt[i]
                if c == '\n':
                    self.finish_line()
                else:
                    if c.isspace():
                        self._idx += 1
                    else:
                        if c == '/':
                            next_c = self.txt[i + 1]
                            if next_c == '/':
                                self._idx += 2
                                self.finish_line()
                                self.erase_comment(i, self._idx - 1)
                            elif next_c == '*':
                                self._idx += 2
                                self.multiline_comment()
                                self.erase_comment(i, self._idx)
                            else:
                                if self._expr_begin == -1:
                                    self.complain("Unexpected / char.")
                                self._idx += 1
                        elif c == ';':
                            self._idx = i + 1
                            debug('resetting _expr_begin at %s (line %s)' % (i, self._line))
                            self._expr_begin = -1
                        elif c == '{':
                            debug('calling block_header at %s with header_line = %s' % (self._idx, header_line))
                            name = self.block_header(header_line)
                            if not name:
                                self._idx += 1
                                self.anonymous_block()
                            debug('_idx is now %s' % self._idx)
                        elif c == '}':
                            self.end = i + 1
                            debug('ending block at %s (line %s; full lines = %s)' % (self.end, self._line, self.full_line_count))
                            return
                        elif c == '#':
                            self.preproc_directive()
                        elif c == '"':
                            self._idx += 1
                            self.string_literal()
                        elif c == "'":
                            next_c = self.txt[i + 1]
                            if next_c == '\\':
                                self._idx += 1
                            self._idx += 2
                        else:
                            if self._expr_begin == -1:
                                self._expr_begin = i
                                header_line = self._line
                                debug('expr_begin at %s (line %s)' % (self._idx, self._line))
                            self._idx += 1
        finally:
            # Remove clutter that was only needed during parse phase.
            # This is not especially necessary (eventually, gc will release
            # resources), but I want the properties on the object to be
            # pristine.
            del self._expr_begin
            del self._idx
            del self._last_block_comment        
    
class BlockCommentInfo(BlockInfo):
    '''
    Hold info about a block comment.
    '''
    def __init__(self, parent, txt, line, begin, end):
        BlockInfo.__init__(self, parent, txt, line, begin, end, 'comment')
    
class UdtInfo(ParsedBlockInfo):    
    '''
    Hold info about a user-defined type -- a class or struct.
    '''
    def __init__(self, variant, name, parent, txt, header_begin, at_line_num, parse_line_num, parse_begin):
        ParsedBlockInfo.__init__(self, name, parent, txt, at_line_num, header_begin, 'udt')
        self.variant = variant
        self.parse(self.txt, parse_begin, parse_line_num)
        
    def header(self):
        h = '%s %s' % (self.variant, self.name)
        if self.preceding_block_comment:
            h = '/* ... */\n' + h
        return h

class FuncInfo(ParsedBlockInfo):
    def __init__(self, returned, name, params, postfix, parent, txt, header_begin, at_line_num, parse_line_num, parse_begin):
        ParsedBlockInfo.__init__(self, name, parent, txt, at_line_num, header_begin, 'function')
        self.returned = returned
        self.params = params
        self.postfix = postfix
        self.parse(self.txt, parse_begin, parse_line_num)

    def header(self):
        h = '%s %s(%s)%s' % (self.returned, self.name, ', '.join(self.params), self.postfix)
        if self.preceding_block_comment:
            h = '/* ... */\n' + h
        return h
    
class ParsedFile(ParsedBlockInfo):
    def __init__(self, path=None, codebase=None):
        ParsedBlockInfo.__init__(self, name=path, parent=None, txt='', line=0, begin=0, category='file')
        if path:
            f = open(path, 'r')
            txt = f.read()
            f.close()
            self.parse(txt)
        self.codebase = codebase
        
    @property
    def relpath(self):
        if self.codebase:
            return self.name[len(self.codebase.root):]
            
    def header(self):
        h = '%s "%s"' % (self.category, self.name)
        if self.preceding_block_comment:
            h = '/* ... */\n' + h
        return h
                
    def skeleton(self):
        sk = self.header() + '\n'
        if self.blocks:
            for b in self.blocks:
                sk += b.skeleton()
        return sk
    
    def parse(self, txt, idx=0):
        self._line = 1
        if idx:
            self._line += txt.count(0, idx)
        ParsedBlockInfo.parse(self, txt, idx)