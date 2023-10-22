#!/usr/bin/env ruby

require "pathname"
require "fileutils"
require "open-uri"
require "openssl"
require "rubygems/package"
require "zlib"

@debug = (ENV["DEBUG"] == "true" or ARGV.include?("--debug"))

groonga_nromalizer_mysql_base =
  Pathname.new(__dir__).expand_path + "plugins" + "groonga-normalizer-mysql"

def extract_tar_gz(tar_gz_path, base)
  Zlib::GzipReader.open(tar_gz_path) do |tar_io|
    Gem::Package::TarReader.new(tar_io) do |tar|
      metadata = nil
      tar.each do |entry|
        path = entry.full_name
        path = metadata["path"] || path if metadata
        path = path.gsub(/\A[^\/]+/) {base.to_s}
        p [entry.header.typeflag, entry.full_name, path, metadata] if @debug
        if entry.directory?
          FileUtils.mkdir_p(path)
        elsif entry.file?
          File.open(path, "wb") do |file|
            file.print(entry.read)
          end
        elsif entry.header.typeflag == "x"
          metadata = {}
          entry.read.each_line do |line|
            _size, key_value = line.chomp.split(" ", 2)
            key, value = key_value.split("=", 2)
            metadata[key] = value
          end
          next
        end
        metadata = nil
      end
    end
  end
end

def download(url, base)
  ssl_verify_mode = nil
  if /mingw/ =~ RUBY_PLATFORM
    ssl_verify_mode = OpenSSL::SSL::VERIFY_NONE
  end

  FileUtils.mkdir_p(base.parent.to_s)

  tar = "#{base}.tar"
  tar_gz = "#{tar}.gz"
  URI(url).open(:ssl_verify_mode => ssl_verify_mode) do |remote_tar_gz|
    File.open(tar_gz, "wb") do |local_tar_gz|
      local_tar_gz.print(remote_tar_gz.read)
    end
  end
  FileUtils.rm_rf(base)
  extract_tar_gz(tar_gz, base)
  FileUtils.rm_rf(tar_gz)
end

url = "https://packages.groonga.org/source/groonga-normalizer-mysql/groonga-normalizer-mysql-latest.tar.gz"
# TODO: Remove me when we release groonga-normalizer-mysql-1.2.3.
url = "https://packages.groonga.org/source/groonga-normalizer-mysql/groonga-normalizer-mysql-1.2.1.tar.gz"
download(url, groonga_nromalizer_mysql_base)
