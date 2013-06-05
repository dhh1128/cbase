import os
import unittest

import paths
import codebase

SAMPLE_DIR = os.path.join(paths.TESTDIR, 'sample_code')

class CodebaseTest(unittest.TestCase):
    
    def test_file_enum(self):
        cb = codebase.Codebase(SAMPLE_DIR)
        self.assertEqual(69, len(cb.by_ext['.h']))
        for item in cb.by_ext['.h']:
            self.assertTrue(item in cb.by_folder['include/'])
        self.assertEqual(2, len(cb.by_folder['']))
        
    def test_path_norm(self):
        cb = codebase.Codebase(SAMPLE_DIR)
        self.assertTrue('/test/sample_code/' in cb.root)
        self.assertTrue(cb.root.endswith('/'))
        for folder in cb.by_folder['']:
            self.assertTrue(folder.endswith('/'))
            
    def test_vcs_folders_ignored(self):
        cb = codebase.Codebase(SAMPLE_DIR)
        self.assertFalse('include/.svn' in cb.by_folder)
        self.assertTrue('include' in cb.by_folder)
        
    def test_novisit(self):
        def ignore_some(fname):
            return not fname.startswith('M')
        cb = codebase.Codebase(SAMPLE_DIR, visit_filter=ignore_some)
        self.assertFalse('include/MBSON.h' in cb.by_ext['.h'])
        self.assertTrue('include/PluginNodeAllocate.h' in cb.by_ext['.h'])
        
    def test_iteration(self):
        cb = codebase.Codebase(SAMPLE_DIR)
        hits = [f for f in cb.files if f == 'moab/MUIVPC.c' or f == 'include/moab-const.h']
        self.assertEqual(2, len(hits))
        self.assertFalse('lib/' in cb.files)
        
