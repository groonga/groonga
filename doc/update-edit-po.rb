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

locale = ARGV.shift
pot = ARGV.shift
po = ARGV.shift
po_time_stamp = ARGV.shift
edit_po = ARGV.shift

def po_file_is_updated?(po, po_time_stamp)
  return false unless File.exist?(po)
  return true unless File.exist?(po_time_stamp)
  File.mtime(po) > File.mtime(po_time_stamp)
end

if po_file_is_updated?(po, po_time_stamp)
  FileUtils.rm_f(edit_po)
end
if File.exist?(po)
  FileUtils.cp(po, edit_po)
else
  GetText::Tools::MsgInit.run("--input=#{pot}",
                              "--locale=#{locale}",
                              "--no-translator",
                              "--output=#{edit_po}")
end

edit_po_mtime = File.mtime(edit_po)
GetText::Tools::MsgMerge.run("--sort-by-location",
                             "--no-wrap",
                             "--update",
                             "--use-one-line-per-reference",
                             edit_po,
                             pot)
if File.exist?(po) and File.mtime(po) > edit_po_mtime
  GetText::Tools::MsgMerge.run("--output=#{edit_po}",
                               "--sort-by-location",
                               "--no-wrap",
                               "--no-obsolete-entries",
                               "--use-one-line-per-reference",
                               po,
                               edit_po)
end
