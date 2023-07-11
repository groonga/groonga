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

require "find"

def list_paths(variable_name, paths, output)
  output.puts("#{variable_name} = \\")
  paths.each do |path|
    output.puts("\t#{path} \\")
  end
  output.puts("\t$(NULL)")
  output.puts
end

source_dir = __dir__

# For GNU Autotools
File.open(File.join(source_dir, "files.am"), "w") do |output|
  # Sphinx related
  Dir.chdir(source_dir) do
    files = []
    Find.find("source") do |path|
      next unless File.file?(path)
      next if path.end_with?(".pyc")
      files << path
    end
    files.sort!
    ## absolute source file file list.
    list_paths("absolute_source_files",
               files.collect {|file| "$(top_srcdir)/doc/#{file}"},
               output)
    ## source file path list from doc/.
    list_paths("source_files_relative_from_doc_dir",
               files,
               output)
  end

  # gettext related
  pot_files = Dir.chdir("en/gettext") do
    Dir.glob("**/*.pot").sort
  end
  ["po", "edit_po", "mo"].each do |type|
    if type == "edit_po"
      extension = ".edit"
    else
      extension = ".#{type}"
    end
    target_files = pot_files.collect {|pot| pot.gsub(/\.pot\z/, extension)}
    list_paths("#{type}_files", target_files, output)
    list_paths("#{type}_files_relative_from_locale_dir",
               target_files.collect {|path| "LC_MESSAGES/#{path}"},
               output)
  end

  # output files
  Dir.chdir("en") do
    ## HTML file path list relative from locale/$LANG/ dir.
    html_files = []
    Find.find("html") do |path|
      next unless File.file?(path)
      next if path == "html/.buildinfo"
      html_files << path
    end
    html_files.sort!
    list_paths("html_files_relative_from_locale_dir", html_files, output)
  end
end

# For CMake
File.open(File.join(source_dir, "files.cmake"), "w") do |output|
  # Sphinx related
  Dir.chdir(File.join(source_dir, "source")) do
    files = []
    Find.find(".") do |path|
      path = path.delete_prefix("./")
      next unless File.file?(path)
      next if path.end_with?(".pyc")
      files << path
    end
    files.sort!
    output.puts("set(GRN_DOC_SOURCES")
    files.each do |file|
      output.puts("  #{file}")
    end
    output.puts(")")
    output.puts
  end

  # output files
  Dir.chdir("en") do
    ## HTML file path list relative from locale/$LANG/ dir.
    html_files = []
    Find.find(".") do |path|
      path = path.delete_prefix("./")
      next unless File.file?(path)
      next if path == ".buildinfo"
      html_files << path
    end
    html_files.sort!
    output.puts("set(GRN_DOC_HTML_FILES")
    html_files.each do |html_file|
      output.puts("  #{html_file}")
    end
    output.puts(")")
    output.puts
  end
end
