# I'd like to use subprocess.check_output, even in python 2.6. However, it wasn't
# introduced till python 2.7. This patches the problem on old revs.

import subprocess

if not hasattr(subprocess, 'check_output'):
    def check_output(*popenargs, **kwargs):
        kwargs['stdout'] = subprocess.PIPE
        process = subprocess.Popen(*popenargs, **kwargs)
        err = process.wait()
        if err:
            raise Exception('Process returned %s.' % str(err))
        return process.stdout.read()
    subprocess.check_output = check_output
