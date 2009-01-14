#!/usr/bin/env python

# Groonga/Dicty server stress tool
# (c) Brazil 2007-

# Copyright(C) 2007 Brazil
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import os
import sys
import time
import random
import pprint
import threading
from optparse import OptionParser

def error(msg, info, lock):
  lock.acquire()
  try:
    sys.stderr.write("* error *\n%s\n" % msg)
    pp = pprint.PrettyPrinter(indent = 2, stream = sys.stderr)
    pp.pprint(info)
  finally:
    lock.release()

def worker(ctx, p, lock):
  info = {'host': random.choice(p.targets)}
  host = info['host'][0]
  port = info['host'][1]

  if port == 'file':
    db = ctx.Db.open(host)
    c = db.ctx_open(ctx.CTX_USEQL)
    if not c:
      error('ctx_open failed', info, lock)
      return
  else:
    c = ctx.Context.connect(host, port, ctx.CTX_USEQL)
    if not c:
      error('connect failed', info, lock)
      return

  info['queries'] = 0
  for i in xrange(p.n_loop):
    for q in p.queries:
      info['queries'] += 1
      rc = c.send(q, 0)
      if rc:
        info['ctxinfo'] = c.info_get()
        error('send error: %d' % rc, info, lock)
        return
      while True:
        (rc, buf, flags) = c.recv()
        # print buf
        if rc:
          info['ctxinfo'] = c.info_get()
          error('recv error: %d' % rc, info, lock)
          return
        if not (flags & ctx.CTX_MORE):
          break

  if p.verbose:
    thd = threading.currentThread()
    print 'end thread (threadname: %s)' % thd.getName()

def parse_opts():
  p = OptionParser(usage = '%prog [options] dest')
  p.add_option('-p', '--process', dest = 'n_proc',
               default = 4, type = 'int',
               help = 'specify number of processes forked')
  p.add_option('-t', '--thread', dest = 'n_thd',
               default = 2, type = 'int',
               help = 'specify number of threads by one process')
  p.add_option('-l', '--loop', dest = 'n_loop',
               default = 10, type = 'int',
               help = 'specify number of loops on one worker thread')
  p.add_option('', '--timeout', dest = 'timeout',
               default = 3, type = 'int',
               help = 'specify timeout on making context')
  p.add_option('-q', '--query-file', dest = 'queryfile',
               help = 'specify query file')
  p.add_option('-v', '--verbose', dest = 'verbose',
               action = 'store_true', default = False,
               help = 'verbose mode')
  return p.parse_args()

def main():
  def check_ctx(targets):
    import groongactx as ctx
    for name, port in targets:
      if port == 'file':
        db = ctx.Db.open(name)
        if not db:
          print 'file: %s cannot be opened with sen_db_open.'
          sys.exit(1)
        c = db.ctx_open(ctx.CTX_USEQL)
        if not c:
          print 'file: %s sen_ctx_open failed.'
          sys.exit(1)
      else:
        c = ctx.Context.connect(name, port, ctx.CTX_USEQL)
        if not c:
          print 'cannot connect groonga/dicty server(host: %s port: %d)'
          sys.exit(1)

  (opts, args) = parse_opts()

  # check and load queryfile
  if not opts.queryfile:
    print 'please specify query file with -q option.'
    sys.exit(1)
  qf = open(opts.queryfile, 'r')
  opts.queries = qf.readlines()
  qf.close()

  if args:
    opts.targets = [x.split(':') for x in args]
    opts.targets = [(x[0], int(x[1])) for x in opts.targets]
  else:
    opts.targets = [('localhost', 10041), ]

  if opts.verbose:
    for name, port in opts.targets:
      if port == 'file':
        print 'local database file: %s' % name
      else:
        print 'host: %s port: %d' % (name, port)

  # check ctx
  pid = os.fork() # avoid import groongactx/sen_init call
  if pid == 0:
    t = threading.Thread(target = check_ctx, args = (opts.targets, ))
    t.setDaemon(True)
    t.start()
    t.join(3)
    if t.isAlive():
      print 'context timeout.'
      sys.exit(1)
    else:
      sys.exit(0)
  (pid, st) = os.wait()
  if os.WEXITSTATUS(st) != 0:
    sys.exit(1)

  print "start %d processs, each process make %d threads." % (
    opts.n_proc, opts.n_thd)
  procs = list()
  stime = time.time()
  for i in xrange(opts.n_proc):
    pid = os.fork()
    if pid == 0:
      import groongactx as ctx # sen_init per process
      lock = threading.Lock()
      threads = list()
      for j in xrange(opts.n_thd):
        t = threading.Thread(target = worker, args = (ctx, opts, lock))
        t.setDaemon(True)
        threads.append(t)
      for t in threads:
        t.start()
      for t in threads:
        t.join()
      if opts.verbose:
        print 'end process(pid: %d)' % os.getpid()
      sys.exit(0) # sen_fin per process
    else:
      procs.append(pid)
  for pid in procs:
    os.waitpid(pid, 0)
  etime = time.time()
  print 'end all processes'

  print """
processes  : %d
threads    : %d (%d per process)
loops      : %d (%d per thread)
queries    : %d (%d per loop)
start time : %s
end time   : %s
total time : %f sec""" % (
  opts.n_proc,
  opts.n_proc * opts.n_thd, opts.n_thd,
  opts.n_proc * opts.n_thd * opts.n_loop, opts.n_loop,
  opts.n_proc * opts.n_thd * opts.n_loop * len(opts.queries), len(opts.queries),
  time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(stime)),
  time.strftime('%Y/%m/%d %H:%M:%S', time.localtime(etime)),
  etime - stime)

main()
