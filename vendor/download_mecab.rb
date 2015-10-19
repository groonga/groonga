#!/usr/bin/env ruby

require "pathname"
require "fileutils"
require "open-uri"
require "rubygems/package"
require "zlib"

base_dir = Pathname.new(__FILE__).expand_path.dirname.parent

mecab_version = (base_dir + "bundled_mecab_version").read.strip
mecab_naist_jdic_version = (base_dir + "bundled_mecab_naist_jdic_version").read.strip

mecab_base = "mecab-#{mecab_version}"
mecab_naist_jdic_base = "mecab-naist-jdic-#{mecab_naist_jdic_version}"

def extract_tar_gz(tar_gz_path)
  Zlib::GzipReader.open(tar_gz_path) do |tar_io|
    Gem::Package::TarReader.new(tar_io) do |tar|
      tar.each do |entry|
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
  tar = "#{base}.tar"
  tar_gz = "#{tar}.gz"
  open(url) do |remote_tar_gz|
    File.open(tar_gz, "wb") do |local_tar_gz|
      local_tar_gz.print(remote_tar_gz.read)
    end
  end
  FileUtils.rm_rf(base)
  extract_tar_gz(tar_gz)
  FileUtils.rm_rf(tar_gz)
end

download("https://drive.google.com/uc?export=download&id=0B4y35FiV1wh7cENtOXlicTFaRUE",
         mecab_base)

download("http://osdn.dl.sourceforge.jp/naist-jdic/48487/#{mecab_naist_jdic_base}.tar.gz",
         mecab_naist_jdic_base)
