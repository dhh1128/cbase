import os
import unittest
import StringIO
import sys

import paths
import cparser

SAMPLE_DIR = os.path.join(paths.TESTDIR, 'sample_code')

def dump(txt):
    n = 0
    line = 1
    for c in txt:
        if c == '\n':
            print str(n).rjust(8), '\\n'
            line += 1
        else:
            print str(line).rjust(4), str(n).rjust(3), c
        n += 1
    print

def mock(txt_or_parser, idx):
    '''
    Artifically configure internal object state in such a way that it
    looks like it's in the middle of a parse of the specified text, at
    the specified offset.
    '''
    if type(txt_or_parser) == type(''):
        parser = cparser.ParsedFile()
        parser.txt = txt_or_parser
    else:
        parser = txt_or_parser
    parser.end = len(parser.txt)
    parser._idx = idx
    return parser

class ParserTest(unittest.TestCase):
    
    def setUp(self):
        self.tmpstderr = StringIO.StringIO()
        self.oldstderr = sys.stderr
        sys.stderr = self.tmpstderr
        
    def tearDown(self):
        sys.stderr = self.oldstderr
        self.tmpstderr = None
        self.oldstderr = None
                
    def test_finish_line_normal(self):
        p = mock("line 1\nline 2", 0)
        self.assertTrue(p.finish_line())
        self.assertEqual(7, p._idx)
        
    def test_finish_line_no_eol(self):
        p = mock("line 1\nline 2", 7)
        self.assertFalse(p.finish_line())
        self.assertEqual(13, p._idx)
        
    def test_string_literal_normal(self):
        p = mock('"hello\\"" "abc"\nline 2', 1)
        self.assertTrue(p.string_literal())
        self.assertEqual(9, p._idx)
        self.assertEqual(' ', p.txt[p._idx])
        mock(p, 11)
        self.assertTrue(p.string_literal())
        self.assertEqual(15, p._idx)
        
    def test_string_literal_unterminated(self):
        p = mock(' "hello\nline 2"', 2)
        self.assertFalse(p.string_literal())
        self.assertEqual(7, p._idx)
        self.tmpstderr.seek(0, 0)
        self.assertTrue('Error. Line ended before string literal ended.' in self.tmpstderr.read())
        
    def test_multiline_comment_normal(self):
        p = mock('/*hello*//**//*/...*/\nline 2', 2)
        self.assertTrue(p.multiline_comment())
        self.assertEqual(9, p._idx)
        mock(p, 11)
        self.assertTrue(p.multiline_comment())
        self.assertEqual(13, p._idx)
        mock(p, 15)
        self.assertTrue(p.multiline_comment())
        self.assertEqual(21, p._idx)        
        
    def test_multiline_comment_unterminated(self):
        p = mock('/*hello\nline 2', 2)
        self.assertFalse(p.multiline_comment())
        self.assertEqual(14, p._idx)
        
    def test_preproc_directive_normal(self):
        for word in ['include', 'pragma', 'if', 'ifdef', 'endif', 'define']:
            word = word.ljust(7)
            p = mock('#%s <x.h>\nhello' % word, 0)
            self.assertTrue(p.preproc_directive())
            self.assertEqual(15, p._idx)
            
    def test_multiline_define(self):
        p = mock('# define abc(x, y) \\ \n  something\\\n  end\nhello', 0)
        self.assertTrue(p.preproc_directive())
        self.assertEqual(41, p._idx)
        
    def test_define_unterminated(self):
        p = mock('# define abc(x, y) \\ \n  something\\\n  end', 0)
        self.assertFalse(p.preproc_directive())
        self.assertEqual(40, p._idx)
        
    def test_udt(self):
        p = cparser.ParsedFile()
        p.name = 'fake.c'
        txt = '\ntypedef struct foo\n  { \nint x;\nconst char * y;\n}\nhello'
        p.parse(txt)
        self.assertEqual(1, len(p.blocks))
        udt = p.blocks[0]
        self.assertEqual('struct', udt.variant)
        self.assertEqual('foo', udt.name)
        self.assertEqual(4, udt.full_line_count)
        self.assertEqual(2, udt.at_line_num)

    def test_func(self):
        p = cparser.ParsedFile()
        p.name = 'fake.c'
        txt = '//a comment\nint doSomething(char * buf, int n);\nvoid doSomethingElse(double f) {\n  printf("hello");\n}'
        p.parse(txt)
        self.assertEqual(1, len(p.blocks))
        func = p.blocks[0]
        self.assertEqual('void', func.returned)
        self.assertEqual('doSomethingElse', func.name)
        self.assertEqual('double f', func.params[0])
        self.assertEqual(2, func.full_line_count)
        self.assertEqual(3, func.at_line_num)
        
    def test_easy_file1(self):
        c = cparser.ParsedFile(os.path.join(SAMPLE_DIR, 'include', 'MUURL.h'))
        # First 2 block comments should be counted; last one after #endif ignored
        self.assertEqual(2, c.comment_count)
        self.assertEqual(73, c.comment_bytes)
        # Function decls aren't blocks...
        self.assertFalse(bool(c.blocks))

    def test_easy_file2(self):
        c = cparser.ParsedFile(os.path.join(SAMPLE_DIR, 'include', 'PluginBase.h'))
        self.assertEqual(12, c.comment_count)
        self.assertEqual(360, c.comment_bytes)
        descrip = ';'.join([str(b) for b in c.blocks])
        self.assertEqual('/* ... */\nclass PluginException;/* ... */\nclass PluginBase', descrip)
        
    def test_hard_string_literal2(self):
        # This particular string used to cause a hang.
        txt = '''int MVMShow() {
  MStringAppendF(Buffer,"%sVM[%s]  State: %s  JobID: %s  ActiveOS: %s\\n",'''
        #dump(txt)
        p = cparser.ParsedFile()
        p.parse(txt)

    def test_for_loop(self):
        txt = '''    MStringAppendF(Buffer,"    Triggers:\n");

    for (tindex = 0;tindex < V->T->NumItems;tindex++)
      {   
      }   

/* END MVMShow.c */'''
        p = cparser.ParsedFile()
        p.parse(txt)
        self.assertEqual(1, p.comment_count)
        self.assertEqual(19, p.comment_bytes)
        descrip = ';'.join([str((b.category,b.at_line_num)) for b in p.blocks])
        self.assertEqual("('unnamed', 5)", descrip)

    def test_hard_file1(self):
        c = cparser.ParsedFile(os.path.join(SAMPLE_DIR, 'moab', 'MVMShow.c'))
        self.assertEqual(25, c.comment_count)
        self.assertEqual(1122, c.comment_bytes)
        descrip = ';'.join([str((b.category,b.at_line_num)) for b in c.blocks])
        self.assertEqual("('function', 28);('function', 67)", descrip)
        
    def test_hard_string_literal(self):
        p = mock('"    Procs: %d/%d (CPULoad: %.2f/%.2f)  Mem: %d/%d MB Location: %d:%d\\n" ', 1)
        self.assertTrue(p.string_literal())
        self.assertEqual(72, p._idx)
        self.assertEqual(' ', p.txt[p._idx])
        
    def test_uneven_braces_in_ifdef(self):
        txt = '''void tcl_init(void)
  {

#if TCLX
#if TCL_MINOR_VERSION < 5  && TCL_MAJOR_VERSION < 8
  if (TclX_Init(interp) == TCL_ERROR)
    {
#else

  if (Tclx_Init(interp) == TCL_ERROR)
    {
#endif
    fprintf(stderr, "Tclx_Init error: %s",
            interp->result);
    }

#endif /* TCLX */
  return;
  }

void nothing() { }
'''
        p = cparser.ParsedFile()
        p.parse(txt)
        descrip = ';'.join([str((b.category,b.at_line_num)) for b in p.blocks])
        self.assertEqual("('function', 1);('function', 21)", descrip)

if __name__ == '__main__':
    print('This module is intended to be tested with nose.')