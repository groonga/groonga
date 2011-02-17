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
#ifndef GRN_SUGGEST_UTIL_H
#define GRN_SUGGEST_UTIL_H

#include <sys/queue.h>
#include <event.h>
#include <stdint.h>

int print_error(const char *format, ...);
int daemonize(void);
void parse_keyval(struct evkeyvalq *get_args,
                  const char **query, const char **types,
                  const char **client_id, const char **target_name,
                  const char **learn_target_name,
                  const char **callback,
                  uint64_t *millisec);

#endif /* GRN_SUGGEST_UTIL_H */
