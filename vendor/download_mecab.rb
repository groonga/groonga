#!/usr/bin/env ruby

require "pathname"
require "fileutils"
require "open-uri"

base_dir = Pathname.new(__FILE__).expand_path.dirname.parent

mecab_version = (base_dir + "bundled_mecab_version").read.strip
mecab_naist_jdic_version = (base_dir + "bundled_mecab_naist_jdic_version").read.strip

mecab_base = "mecab-#{mecab_version}"
mecab_naist_jdic_base = "mecab-naist-jdic-#{mecab_naist_jdic_version}"

def download(url, base)
  tar = "#{base}.tar"
  tar_gz = "#{tar}.gz"
  open(url) do |remote_tar_gz|
    File.open(tar_gz, "wb") do |local_tar_gz|
      local_tar_gz.print(remote_tar_gz.read)
    end
  end
  FileUtils.rm_rf(base)
  system("7z", "x", tar_gz)
  system("7z", "x", tar)
  FileUtils.rm_rf(tar)
  FileUtils.rm_rf(tar_gz)
end

download("https://drive.google.com/uc?export=download&id=0B4y35FiV1wh7cENtOXlicTFaRUE",
         mecab_base)

download("http://osdn.dl.sourceforge.jp/naist-jdic/48487/#{mecab_naist_jdic_base}.tar.gz",
         mecab_naist_jdic_base)
