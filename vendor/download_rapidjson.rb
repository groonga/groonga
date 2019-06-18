#!/usr/bin/env ruby

require "pathname"
require "fileutils"
require "open-uri"
require "openssl"
require "rubygems/package"
require "zlib"

@debug = (ENV["DEBUG"] == "true" or ARGV.include?("--debug"))

base_dir = Pathname.new(__FILE__).expand_path.dirname.parent

rapidjson_version = (base_dir + "bundled_rapidjson_version").read.strip

rapidjson_base = "rapidjson-#{rapidjson_version}"

def extract_tar_gz(tar_gz_path)
  Zlib::GzipReader.open(tar_gz_path) do |tar_io|
    Gem::Package::TarReader.new(tar_io) do |tar|
      tar.each do |entry|
        p [entry.header.typeflag, entry.full_name] if @debug
        if entry.directory?
          FileUtils.mkdir_p(entry.full_name)
        elsif entry.file?
          File.open(entry.full_name, "wb") do |file|
            file.print(entry.read)
          end
        end
      end
    end
  end
end

def download(url, base)
  ssl_verify_mode = nil
  if /mingw/ =~ RUBY_PLATFORM
    ssl_verify_mode = OpenSSL::SSL::VERIFY_NONE
  end

  tar = "#{base}.tar"
  tar_gz = "#{tar}.gz"
  open(url, :ssl_verify_mode => ssl_verify_mode) do |remote_tar_gz|
    File.open(tar_gz, "wb") do |local_tar_gz|
      local_tar_gz.print(remote_tar_gz.read)
    end
  end
  FileUtils.rm_rf(base)
  extract_tar_gz(tar_gz)
  FileUtils.rm_rf(tar_gz)
end

download("https://github.com/Tencent/rapidjson/archive/v#{rapidjson_version}.tar.gz",
         rapidjson_base)
