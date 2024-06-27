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

flush_diff = lambda do
  return unless in_diff

  parameters = [
    "file=#{diff_to}",
    "line=#{diff_to_line}",
    "title=diff",
  ].join(",")
  # We need to use URL encode for new line:
  # https://github.com/actions/toolkit/issues/193
  message = diff_content.gsub("\n", "%0A")
  puts("::error #{parameters}::#{message}")
end

ARGF.each_line do |line|
  case line
  when /\A(?:\e\[\d+m)?--- a\/(.+)$/ # git diff
    path = $1
    flush_diff.call
    diff_from = path
    diff_to = nil
    diff_from_line = nil
    diff_to_line = nil
    in_diff = false
  when /\A(?:\e\[\d+m)?\+\+\+ b\/(.+)$/ # git diff
    diff_to = $1
  when /\A(?:\e\[\d+m)?@@ -(\d+),(\d+) \+(\d+),(\d+) @@/
    diff_from_line = $1
    diff_to_line = $3
    in_diff = true
  else
    if in_diff
      diff_content << line
    end
  end
  puts(line)
end

flush_diff.call
