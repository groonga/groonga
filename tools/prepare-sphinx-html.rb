#!/usr/bin/env ruby

if ARGV.size != 2
  puts "Usage: #{$0} SOURCE_DIR DEST_DIR"
  exit(false)
end

require 'pathname'

def fix_link(text, extension)
  send("fix_#{extension}_link", text)
end

def fix_link_path(text)
  text.gsub(/\b_(sources|static)\b/, '\1')
end

def fix_html_link(html)
  html.gsub(/(href|src)="(.+?)"/) do
    attribute = $1
    link = $2
    link = fix_link_path(link)
    "#{attribute}=\"#{link}\""
  end
end

def fix_js_link(js)
  fix_link_path(js)
end

source_dir, dest_dir = ARGV

source_dir = Pathname.new(source_dir)
dest_dir = Pathname.new(dest_dir)

locale_dirs = []
source_dir.each_entry do |top_level_path|
  locale_dirs << top_level_path if /\A[a-z]{2}\z/ =~ top_level_path.to_s
end

locale_dirs.each do |locale_dir|
  locale_source_dir = source_dir + locale_dir + "html"
  locale_dest_dir = dest_dir + locale_dir
  locale_source_dir.find do |source_path|
    relative_path = source_path.relative_path_from(locale_source_dir)
    dest_path = locale_dest_dir + relative_path
    if source_path.directory?
      dest_path.mkpath
    else
      case source_path.extname
      when ".html", ".js"
        content = source_path.read
        content = fix_link(content, source_path.extname.gsub(/\A\./, ''))
        dest_path.open("wb") do |dest|
          dest.print(content.strip)
        end
        FileUtils.touch(dest_path, :mtime => source_path.mtime)
      else
        case source_path.basename.to_s
        when ".buildinfo"
          # ignore
        else
          FileUtils.cp(source_path, dest_path, :preserve => true)
        end
      end
    end
  end
end

dest_dir.find do |dest_path|
  if dest_path.directory? and /\A_/ =~ dest_path.basename.to_s
    normalized_dest_path = dest_path + ".."
    normalized_dest_path += dest_path.basename.to_s.gsub(/\A_/, '')
    FileUtils.mv(dest_path, normalized_dest_path)
  end
end
