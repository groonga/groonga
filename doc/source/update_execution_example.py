#!/usr/bin/env python
# -*- coding: utf-8; -*-　

from subprocess import *
from select import select
from sys import argv,stdout
import os

GROONGA_PATH = os.environ.get("GROONGA")
if GROONGA_PATH is None:
  GROONGA_PATH = "groonga"
DB_PATH = "/tmp/example.db"

os.system('rm -rf %s*' % DB_PATH)
ioobj = Popen([GROONGA_PATH, '-n', DB_PATH], stdin = PIPE, stdout = PIPE)
ioin  = ioobj.stdin
ioout = ioobj.stdout

fout = None

def execmd(cmd, fout):
  a = '> ' + cmd + "\n"
  stdout.write(a)
  stdout.flush()
  ioin.write(cmd + "\n")
  ioin.flush()
  if fout:
    fout.write(a + "  ")
  while True:
    out = select([ioout], [], [], 0.2)
    if len(out[0]):
      a = ioout.read(1)
      if a != None:
        stdout.write(a)
        if fout:
          if a == '\n':
            fout.write(a + "  ")
          else:
            fout.write(a)
    else:
      stdout.flush()
      break

def readfile(fname, outflag):
  if outflag > 32:
    print "!!!! INCLUDE DEPTH OVER !!!!"
    raise
  b = fname.rfind('/')
  if b < 0:
    rootdir = './'
  else:
    rootdir = fname[0:b+1]

  fi = open(fname, 'r')
  dat = fi.read().split("\n")
  fi.close()

  line = 0;
  while len(dat):
    cmd = dat.pop(0)
    if cmd.startswith('.. groonga-command'):
      print '### command start'
      fout = None
      while len(dat):
        cmd = dat.pop(0)
        if cmd.startswith('.. include:: '):
          a = rootdir + cmd[13:]
          if outflag == 0:
            dir_name = os.path.dirname(a)
            if not os.path.exists(dir_name):
              os.makedirs(dir_name)
            fout = open(a, 'w')
            print '### write start : ' + a
            fout.write("実行例 ::\n\n  ")
        elif cmd.startswith('.. .. '):
          a = cmd[6:]
          if fout:
            fout.write(a + "\n  ")
          print a
        elif cmd.startswith('..'):
          if cmd.replace(' ','').replace("\t",'') == '..':
            while len(dat):
              if dat[0] == '' or (dat[0][0] != ' ' and dat[0][0] != '	'):
                break
              execmd(dat.pop(0), fout)
          else:
            cmd = cmd[3:]
            execmd(cmd, fout)
        else:
          print '### command end'
          if fout:
            fout.close()
          break
    elif cmd.startswith('.. groonga-include : '):
      a = rootdir + cmd[21:]
      print '###>>> include : ' + a
      readfile(a, outflag + 1)
      print '###<<< include end'


if len(argv) > 1:
  readfile(argv[1], 0)
else:
  for root, dirs, files in os.walk('./'):
    for fname in files:
      if fname.lower().endswith('.txt'):
        b = os.path.join(root, fname)
        print "===" + b
        readfile(b, 0)

if fout:
  fout.close()
ioin.close()
ioout.close()

