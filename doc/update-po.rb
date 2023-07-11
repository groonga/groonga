#!/usr/bin/env ruby
#
# Copyright(C) 2023  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

require "fileutils"
require "gettext/tools"

po = ARGV.shift
po_time_stamp = ARGV.shift
edit_po = ARGV.shift

GetText::Tools::MsgCat.run("--no-fuzzy",
                           "--no-location",
                           "--no-obsolete-entries",
                           "--no-report-warning",
                           "--no-wrap",
                           "--output=#{po}",
                           "--remove-header-field=PO-Revision-Date",
                           "--remove-header-field=POT-Creation-Date",
                           "--sort-by-location",
                           edit_po)
FileUtils.touch(po_time_stamp)
