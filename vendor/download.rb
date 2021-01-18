#!/usr/bin/env ruby

require "open-uri"
require "openssl"

@debug = (ENV["DEBUG"] == "true" or ARGV.include?("--debug"))

def download(url)
  ssl_verify_mode = nil
  if /mingw/ =~ RUBY_PLATFORM
    ssl_verify_mode = OpenSSL::SSL::VERIFY_NONE
  end

  output_path = File.basename(url)
  URI(url).open(:ssl_verify_mode => ssl_verify_mode) do |input|
    File.open(output_path, "wb") do |output|
      IO.copy_stream(input, output)
    end
  end
end

target = ARGV[0]
if target.nil?
  $stderr.puts("Usage: #{$0} TARGET")
  $stderr.puts(" e.g.: #{$0} zstd")
  exit(false)
end

case target
when "zstd"
  cmakelists = File.read(File.join(__dir__, "..", "CMakeLists.txt"))
  version = cmakelists.scan(/set\(GRN_ZSTD_EP_VERSION \"(.+)"\)/).flatten[0]
  url = "https://github.com/facebook/zstd/releases/download/"
  url << "v#{version}/zstd-#{version}.tar.gz"
  download(url)
else
  $stderr.puts("Unsupported target #{target}")
  exit(false)
end
