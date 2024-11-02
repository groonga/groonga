#!/usr/bin/env ruby

require "fileutils"
require "open-uri"
require "openssl"

@debug = (ENV["DEBUG"] == "true" or ARGV.include?("--debug"))

def download(url, output_path=nil)
  ssl_verify_mode = nil
  if /mingw/ =~ RUBY_PLATFORM
    ssl_verify_mode = OpenSSL::SSL::VERIFY_NONE
  end

  output_path ||= File.basename(url)
  URI(url).open(:ssl_verify_mode => ssl_verify_mode) do |input|
    File.open(output_path, "wb") do |output|
      IO.copy_stream(input, output)
    end
  end

  if block_given?
    base_dir = output_path.sub(/\.tar\..*\z/, "")
    FileUtils.rm_rf(base_dir)
    system("tar", "xf", output_path)
    Dir.chdir(base_dir) do
      yield
    end
    system("tar", "czf", output_path, base_dir)
    FileUtils.rm_rf(base_dir)
  end
end

all_targets = [
  "blosc",
  "croaring",
  "h3",
  "llama.cpp",
  "message-pack",
  "simdjson",
  "simsimd",
  "xsimd",
  "zstd",
]

if ARGV.empty?
  targets = all_targets
else
  targets = ARGV
end

cmakelists = File.read(File.join(__dir__, "..", "CMakeLists.txt"))
targets.each do |target|
  case target
  when "blosc"
    version = cmakelists[/set\(GRN_BLOSC_BUNDLED_VERSION \"(.+)"\)/, 1]
    url = "https://github.com/Blosc/c-blosc2/archive/refs/tags/"
    url << "v#{version}.tar.gz"
    download(url, "c-blosc2-#{version}.tar.gz")
  when "croaring"
    version = cmakelists[/set\(GRN_CROARING_EP_VERSION \"(.+)"\)/, 1]
    url = "https://github.com/RoaringBitmap/CRoaring/archive/refs/tags/"
    url << "v#{version}.tar.gz"
    download(url, "CRoaring-#{version}.tar.gz") do
      FileUtils.rm_rf("benchmarks/realdata/")
    end
  when "h3"
    version = cmakelists[/set\(GRN_H3_BUNDLED_VERSION \"(.+)"\)/, 1]
    url = "https://github.com/uber/h3/archive/refs/tags/"
    url << "v#{version}.tar.gz"
    download(url, "h3-#{version}.tar.gz")
  when "llama.cpp"
    version = cmakelists[/set\(GRN_LLAMA_CPP_BUNDLED_VERSION \"(.+)"\)/, 1]
    url = "https://github.com/ggerganov/llama.cpp/archive/refs/tags/"
    url << "#{version}.tar.gz"
    download(url, "llama.cpp-#{version}.tar.gz")
  when "message-pack"
    version = cmakelists[/set\(GRN_MESSAGE_PACK_BUNDLED_VERSION \"(.+)"\)/, 1]
    url = "https://github.com/msgpack/msgpack-c/releases/download/"
    url << "c-#{version}/"
    url << "msgpack-c-#{version}.tar.gz"
    download(url, "msgpack-c-#{version}.tar.gz")
  when "simdjson"
    version = cmakelists[/set\(GRN_SIMDJSON_BUNDLED_VERSION \"(.+)"\)/, 1]
    url = "https://github.com/simdjson/simdjson/archive/refs/tags/"
    url << "v#{version}.tar.gz"
    download(url, "simdjson-#{version}.tar.gz")
  when "simsimd"
    version = cmakelists[/set\(GRN_SIMSIMD_BUNDLED_VERSION \"(.+)"\)/, 1]
    url = "https://github.com/ashvardanian/SimSIMD/archive/refs/tags/"
    url << "v#{version}.tar.gz"
    download(url, "simsimd-#{version}.tar.gz")
  when "xsimd"
    version = cmakelists[/set\(GRN_XSIMD_BUNDLED_VERSION \"(.+)"\)/, 1]
    url = "https://github.com/xtensor-stack/xsimd/archive/refs/tags/"
    url << "#{version}.tar.gz"
    download(url, "xsimd-#{version}.tar.gz")
  when "zstd"
    version = cmakelists[/set\(GRN_ZSTD_BUNDLED_VERSION \"(.+)"\)/, 1]
    url = "https://github.com/facebook/zstd/releases/download/"
    url << "v#{version}/zstd-#{version}.tar.gz"
    download(url)
  else
    $stderr.puts("Unsupported target #{target}")
    exit(false)
  end
end
