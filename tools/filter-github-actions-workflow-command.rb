#!/usr/bin/env ruby
#
# Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>
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

# Workflow commands for GitHub Actions:
# https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions

diff_from = nil
diff_to = nil
diff_from_line = nil
diff_to_line = nil
in_diff = false
diff_content = +""

def remove_escape_sequences(text)
  text.gsub(/\e\[.+?m/, "")
end

flush_diff = lambda do
  return unless in_diff

  parameters = [
    "file=#{diff_to}",
    "line=#{diff_to_line[0]}",
    "endLine=#{diff_to_line[1]}",
    "title=diff",
  ].join(",")
  # We need to use URL encode for new line:
  # https://github.com/actions/toolkit/issues/193
  message = remove_escape_sequences(diff_content).gsub("\n", "%0A")
  puts("::error #{parameters}::#{message}")
end

ARGF.each_line do |line|
  raw_line = remove_escape_sequences(line)
  case raw_line
  when /\A--- a\/(.+)$/ # git diff
    path = $1
    flush_diff.call
    diff_from = path
    diff_to = nil
    diff_from_line = nil
    diff_to_line = nil
    in_diff = false
  when /\A?\+\+\+ b\/(.+)$/ # git diff
    diff_to = $1
  when /\A?@@ -(\d+),(\d+) \+(\d+),(\d+) @@/
    diff_from_line = [$1.to_i, $1.to_i + $2.to_i]
    diff_to_line = [$3.to_i, $3.to_i + $4.to_i]
    in_diff = (diff_from and diff_to)
  when /\A[-+ ]/
    diff_content << line if in_diff
  else
    flush_diff.call
    in_diff = false
  end
  puts(line)
end

flush_diff.call
