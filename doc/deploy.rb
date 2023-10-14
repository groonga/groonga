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

def run(*command_line)
  unless system(*command_line)
    $stderr.puts("Failed to run: #{command_line.join(' ')}")
    exit(false)
  end
end

groonga_org_path = ENV["GROONGA_ORG_PATH"]
if groonga_org_path.nil?
  $stderr.puts("Must specify GROONGA_ORG_PATH")
  exit(false)
end

install_path = File.join(Dir.pwd, "install")
FileUtils.rm_rf(install_path)
run("cmake",
    "--install",
    Dir.pwd,
    "--component", "Document",
    "--prefix", install_path)
docs_path = File.join(groonga_org_path, "docs")
FileUtils.rm_rf(docs_path)
FileUtils.mv(File.join(install_path, "share", "doc", "groonga", "en"),
             docs_path)
Dir.each_child(File.join(install_path, "share", "doc", "groonga")) do |lang|
  next unless File.directory?(lang)
  lang_docs_path = File.join(groonga_org_path, lang, "docs")
  FileUtils.rm_rf(lang_docs_path)
  FileUtils.mv(File.join(install_path, "share", "doc", "groonga", lang),
               lang_docs_path)
end
