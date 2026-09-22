# Copyright (C) 2012-2015  Brazil
# Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

if(NOT WASI)
  # The administration HTML is served by the HTTP server mode. We
  # don't provide it for WASI yet.
  add_subdirectory(html)
  # groonga.conf is read by the groonga command. We don't provide the
  # groonga command for WASI yet.
  install(FILES groonga.conf DESTINATION "${GRN_CONFIG_DIR}/")
endif()

install(FILES synonyms.tsv DESTINATION "${GRN_CONFIG_DIR}/")
