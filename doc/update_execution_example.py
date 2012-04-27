#!/usr/bin/env python
# -*- coding: utf-8; -*-　

from subprocess import *
from select import select
from sys import argv,stdout
import os
import os.path
import shutil
import re
import json

DB_DIRECTORY = "/tmp/groonga-databases"
DEFAULT_DB_NAME = "tutorial.db"

shutil.rmtree(DB_DIRECTORY, ignore_errors=True)
os.makedirs(DB_DIRECTORY)

groonga_process = None
def close_groonga():
  global groonga_process
  if groonga_process:
    groonga_process.stdin.close()
    groonga_process.stdout.close()
    groonga_process = None
    print '###<<< database: close'

def reconnect(name):
  global groonga_process
  close_groonga()
  db_path = os.path.join(DB_DIRECTORY, name)
  if os.path.exists(db_path):
    groonga_process = Popen(["groonga", db_path], stdin=PIPE, stdout=PIPE)
  else:
    groonga_process = Popen(["groonga", "-n", db_path], stdin=PIPE, stdout=PIPE)
  print '###>>> database: open <%s>' % db_path

fout = None

def execmd(command, fout):
  formatted_command_line = '> ' + command + "\n"
  stdout.write(formatted_command_line)
  stdout.flush()
  groonga_process.stdin.write(command + "\n")
  groonga_process.stdin.flush()
  if fout:
    fout.write(formatted_command_line + "  ")
  output_buffer = ""
  while True:
    out = select([groonga_process.stdout], [], [], 0.2)
    if len(out[0]):
      char = groonga_process.stdout.read(1)
      if char is None:
        stdout.write(output_buffer)
        if fout:
          fout.write(output_buffer)
      else:
        output_buffer += char
        if char == '\n':
          if len(output_buffer) < 80:
            formatted_output = output_buffer
          else:
            parsed_output = json.loads(output_buffer)
            formatted_output = json.dumps(parsed_output,
                                          indent=2,
                                          ensure_ascii=False)
            formatted_output += "\n"
            formatted_output = formatted_output.encode("utf-8")
          stdout.write(formatted_output)
          if fout:
            fout.write(re.sub("\n", "\n  ", formatted_output))
          output_buffer = ""
    else:
      stdout.flush()
      break

processed_files = []
def readfile(fname, outflag):
  if fname in processed_files:
    print "skipped processed file: %s" % fname
    return
  if outflag > 32:
    print "!!!! INCLUDE DEPTH OVER !!!!"
    raise
  processed_files.append(fname)

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
        if cmd.startswith('.. database:'):
          database_name = cmd[cmd.index(":")+1:].strip()
          reconnect(database_name)
        elif cmd.startswith('.. include:: '):
          a = rootdir + cmd[13:]
          if outflag == 0:
            dir_name = os.path.dirname(a)
            if not os.path.exists(dir_name):
              os.makedirs(dir_name)
            fout = open(a, 'w')
            print '### write start : ' + a
            fout.write("Execution example::\n\n  ")
        elif cmd.startswith('.. % '):
          a = cmd[5:]
          if fout:
            fout.write(a + "\n  ")
          print a
          os.system(a)
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

entry_point = "source/"
if len(argv) == 2:
  entry_point = argv[1]
if os.path.isfile(entry_point):
  readfile(entry_point, 0)
else:
  for root, dirs, files in os.walk(entry_point):
    for fname in files:
      if fname.lower().endswith('.txt'):
        b = os.path.join(root, fname)
        print "===" + b
        readfile(b, 0)

if fout:
  fout.close()
close_groonga()

