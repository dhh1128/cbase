import subprocess
import re
import time
import os

HG_CMD = 'hg'
GIT_CMD = 'git --no-pager'
LOG_DATE_PAT = re.compile(r'[dD]ate:\s+((Sun|Mon|Tue|Wed|Thu|Fri|Sat) (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) \d+ \d+:\d\d:\d\d \d\d\d\d( ?(Z|[A-Z][A-Z][A-Z]|[-+]\d\d\d\d))?).*')
FILE_DATE_PAT = re.compile(r'\s+(Sun|Mon|Tue|Wed|Thu|Fri|Sat) (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) \d+ \d+:\d\d:\d\d \d\d\d\d.*')
HUNK_PAT = re.compile('@@ ([-+]\d+),(\d+) ([-+]\d+),(\d+) @@.*')

def choose_vcs_cmd(folder):
    if os.path.isdir(os.path.join(folder, '.hg')):
        return HG_CMD
    return GIT_CMD

def get_most_recent_commit_date(folder):
    cmd = choose_vcs_cmd(folder)
    if cmd == HG_CMD:
        cmd = cmd + ' log -l 1'
    else:
        cmd = cmd + ' log -1'
    output = subprocess.check_output(cmd, shell=True, universal_newlines=True)
    match = LOG_DATE_PAT.search(output)
    if match:
        when = match.group(1)
        if match.group(4):
            when = when.replace(match.group(4), '').rstrip()
            
        print('converting %s' % when)
        return time.strptime(when)

def get_change_spec(folder):
    cmd = choose_vcs_cmd(folder)
    if cmd == HG_CMD:
        cmd += ' diff -U 0'
    else:
        cmd += ' diff -U0'
    change_spec = {}
    old_folder = os.getcwd()
    os.chdir(folder)
    try:
        try:
            stdout = subprocess.check_output(cmd, shell=True, universal_newlines=True)
        except:
            return {}
    finally:
        os.chdir(old_folder)
        
    lines = stdout.split('\n')
    for l in lines:
        if l.startswith('+++'):
            fname = l[l.find(' ')+ 1:].strip()
            match = FILE_DATE_PAT.search(fname)
            if match:
                fname = fname[0:match.start()].rstrip()
            if fname.startswith('a/') or fname.startswith('b/'):
                fname = fname[2:]
            change_spec[fname] = []
        else:
            match = HUNK_PAT.match(l)
            if match:
                n = 3
                if match.group(1).startswith('+'):
                    n = 1
                change_spec[fname].append((int(match.group(n)[1:]), int(match.group(n+1))))
    return change_spec

if __name__ == '__main__':
    folder = '/Users/dhardman/code/slp'
    print('vcs is %s' % choose_vcs_cmd(folder))
    print('changespec: ' + str(get_change_spec(folder)))
    print('most recent commit: ' + str(get_most_recent_commit_date(folder)))