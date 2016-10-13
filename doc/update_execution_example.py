#!/usr/bin/env python
# -*- coding: utf-8; -*-　

import subprocess
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
current_log = None
output_log = False
def close_groonga():
  global groonga_process
  global current_log
  global output_log
  if groonga_process:
    groonga_process.stdin.close()
    groonga_process.stdout.close()
    groonga_process = None
    print '###<<< database: close'
  if current_log:
    current_log.close()
    current_log = None
  output_log = False

current_db_path = None
current_log_path = None
def reconnect(name):
  global groonga_process
  global current_log
  global current_db_path
  global current_log_path
  close_groonga()
  current_db_path = os.path.join(DB_DIRECTORY, name)
  current_log_path = os.path.join(DB_DIRECTORY, name + ".log")
  if os.path.exists(current_db_path):
    groonga_process = subprocess.Popen(["groonga",
                                        "--log-path", current_log_path,
                                        current_db_path],
                                       stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE)
  else:
    groonga_process = subprocess.Popen(["groonga",
                                        "--log-path", current_log_path,
                                        "-n",
                                        current_db_path],
                                       stdin=subprocess.PIPE,
                                       stdout=subprocess.PIPE)
  groonga_process.stdin.write("status\n")
  groonga_process.stdin.flush()
  groonga_process.stdout.readline()
  current_log = open(current_log_path)
  current_log.read()
  print '###>>> database: open <%s>' % current_db_path

def expand_command_line(command_line):
  return command_line.replace('${DB_PATH}', current_db_path)

fout = None

def normalize_file_name(file_name):
  return re.sub('^.*?/lib/', 'lib/', file_name)

def normalize_output(output):
  status = output[0]
  if status:
    normalized_start_time = 1337566253.89858
    normalized_elapsed_time = 0.000355720520019531
    status[1] = normalized_start_time
    status[2] = normalized_elapsed_time
    return_code = status[0]
    if return_code != 0:
      backtraces = status[4]
      if backtraces:
        for backtrace in backtraces:
          file_name = backtrace[1]
          backtrace[1] = normalize_file_name(file_name)
  return output

def remove_continuous_marks(command):
  return re.sub('\\\\\n', '', command)

def prepend_prefix(prefix, command):
  beginning_of_line = re.compile('^', re.M)
  return beginning_of_line.sub(prefix, command)

def execmd(command, fout):
  stdout.write(command + "\n")
  stdout.flush()
  groonga_process.stdin.write(remove_continuous_marks(command) + "\n")
  groonga_process.stdin.flush()
  is_command = re.match("[a-z/]", command)
  is_load_command = re.match("load ", command)
  is_console = not re.match("/", command)
  if fout:
    if is_console:
      prefix = "  "
    else:
      prefix = "  % curl http://localhost:10041"
    formatted_command_line = prepend_prefix(prefix, command) + "\n"
    fout.write(formatted_command_line)
  is_load_data_end = re.match("^\]", command)
  if is_load_command:
    return
  if not is_command and not is_load_data_end:
    return
  output_buffer = ""
  first_timeout = 1
  timeout = first_timeout
  while True:
    out = select([groonga_process.stdout], [], [], timeout)
    timeout = rest_timeout
    if len(out[0]):
      char = groonga_process.stdout.read(1)
      if char is None or char == '':
        stdout.write(output_buffer)
        if fout:
          fout.write(output_buffer)
        break
      else:
        output_buffer += char
        if char == '\n':
          if command == 'dump':
            formatted_output = output_buffer
            stdout.write(output_buffer)
          else:
            parsed_output = json.loads(output_buffer)
            normalized_output = normalize_output(parsed_output)
            if len(output_buffer) < 80:
              formatted_output = json.dumps(normalized_output,
                                            ensure_ascii=False)
            else:
              formatted_output = json.dumps(normalized_output,
                                            indent=2,
                                            ensure_ascii=False)
              formatted_output += "\n"
              formatted_output = formatted_output.encode("utf-8")
            stdout.write(formatted_output)
            stdout.write("\n")
          if fout:
            if is_console:
              prefix = "  # "
            else:
              prefix = "  "
            first_lines_re = re.compile("^", re.M)
            fout.write(first_lines_re.sub(prefix, formatted_output.strip()))
            fout.write("\n")
          if current_log:
            log = current_log.read().strip()
            if len(log) > 0:
              prefix = "  # log: "
              first_lines_re = re.compile("^", re.M)
              formatted_log = first_lines_re.sub(prefix, log)
              stdout.write(first_lines_re.sub(prefix, log))
              stdout.write("\n")
              if output_log and fout:
                fout.write(formatted_log)
                fout.write("\n")
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
        elif cmd.startswith('.. log:'):
          global output_log
          log_value = cmd[cmd.index(":")+1:].strip()
          output_log = log_value == "true"
        elif cmd.startswith('.. include:: '):
          a = rootdir + cmd[13:]
          dir_name = os.path.dirname(a)
          if not os.path.exists(dir_name):
            os.makedirs(dir_name)
          fout = open(a, 'w')
          print '### write start : ' + a
          fout.write("Execution example::\n\n")
        elif cmd.startswith('.. % '):
          command_line = cmd[5:]
          if fout:
            fout.write("  % " + command_line + "\n")
          print command_line
          expanded_command_line = expand_command_line(command_line)
          command_output = subprocess.check_output(expanded_command_line,
                                                   shell=True)
          if fout:
            first_lines_re = re.compile("^", re.M)
            fout.write(first_lines_re.sub("  ", command_output.strip()))
            fout.write("\n")
          else:
            print command_output
        elif cmd.startswith('.. .. '):
          command_line = cmd[6:]
          if fout:
            fout.write("  " + command_line + "\n")
          print command_line
        elif cmd.startswith('..'):
          if cmd.replace(' ', '').replace("\t", '') == '..':
            while len(dat):
              if dat[0] == '' or (dat[0][0] != ' ' and dat[0][0] != '	'):
                break
              execmd(dat.pop(0), fout)
          else:
            cmd = cmd[3:]
            while cmd.endswith('\\'):
              cmd += '\n' + re.sub('^\.\. ', '', dat.pop(0))
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
rest_timeout = 0.1
if len(argv) == 2:
  entry_point = argv[1]
elif len(argv) == 3:
  entry_point = argv[1]
  rest_timeout = float(argv[2])
if os.path.isfile(entry_point):
  readfile(entry_point, 0)
else:
  for root, dirs, files in os.walk(entry_point):
    for fname in files:
      if fname.lower().endswith('.rst'):
        b = os.path.join(root, fname)
        print "===" + b
        readfile(b, 0)

if fout:
  fout.close()
close_groonga()

