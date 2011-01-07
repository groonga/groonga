/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010- Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int
print_error(const char *format, ...)
{
  int r;
  va_list l;

  va_start(l, format);
  vfprintf(stderr, format, l);
  r = fprintf(stderr, "\n");
  fflush(stderr);
  va_end(l);

  return r;
}

int
daemonize(void)
{
  pid_t pid;

  switch (fork()) {
  case 0:
    break;
  case -1:
    print_error("fork failed.");
    return -1;
  default:
    wait(NULL);
    _exit(0);
  }
  switch ((pid = fork())) {
  case 0:
    break;
  case -1:
    perror("fork");
    return -1;
  default:
    fprintf(stderr, "%d\n", pid);
    _exit(0);
  }
  {
    int null_fd = open("/dev/null", O_RDWR, 0);
    if (null_fd != -1) {
      dup2(null_fd, 0);
      dup2(null_fd, 1);
      dup2(null_fd, 2);
      if (null_fd > 2) { close(null_fd); }
    }
  }
  return 1;
}
