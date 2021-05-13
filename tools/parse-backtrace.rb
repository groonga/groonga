#!/usr/bin/env ruby

require "English"
require "optparse"
require "ostruct"
require "tempfile"

options = OpenStruct.new
options.version = nil

parser = OptionParser.new
parser.banner += "\n  Parse Groonga's backtrace in log"
parser.on("--version=VERSION",
          "Groonga version",
          "e.g.: 10.0.9",
          "(#{options.version})") do |version|
  options.version = version
end
parser.parse!

def detect_system_version
  system_release_cpe = "/etc/system-release-cpe"
  if File.exist?(system_release_cpe)
    components = File.read(system_release_cpe).chomp.split(":")
    version = components[-1]
    system_id = components[-3]
    "#{system_id}-#{version}"
  else
    raise "unsupported system"
  end
end

def run_command(*args)
  system(*args) or raise "failed to run: #{args.inspect}"
end

def capture_command(*args)
  tempfile = Tempfile.new("parse-backtrace")
  run_command(*args, out: tempfile)
  tempfile.rewind
  tempfile.read
end

def prepare_system_centos_7(options)
  run_command("yum", "install", "-y",
              "https://packages.groonga.org/centos/7/groonga-release-latest.noarch.rpm")
  run_command("yum", "install", "-y",
              "binutils",
              "yum-utils")
  packages = []
  if options.version
    groonga_package_version = "-#{options.version}-1.el7.x86_64"
  else
    groonga_package_version = ""
  end
  packages << "groonga-libs#{groonga_package_version}"
  packages << "groonga-debuginfo#{groonga_package_version}"
  run_command("yum", "install", "-y", *packages)
end

def prepare_system_amazon_2(options)
  run_command("amazon-linux-extras", "install", "epel", "-y")
  prepare_system_centos_7(options)
end

def prepare_system(system_version, options)
  case system_version
  when "centos-7"
    prepare_system_centos_7(options)
  when "amazon-2"
    prepare_system_amazon_2(options)
  else
    raise "unsupported system: #{system_version}"
  end
end

def resolve_debug_path(path, system_version)
  case system_version
  when /\Acentos-/, /\Aamazon-/
    Dir.glob("/usr/lib/debug#{path}*.debug").first || path
  else
    raise "unsupported system: #{system_version}"
  end
end

def demangle(function)
  capture_command("c++filt", function).chomp
end

def resolve_relative_address(relative_address, path)
  if relative_address.start_with?("+")
    return Integer(relative_address[1..-1])
  end
  base_function, offset = relative_address.split("+", 2)
  base_function = demangle(base_function)
  capture_command("nm", "--demangle", path).each_line do |line|
    case line
    when /\A(\h+) \S (\S+)/
      address = $1
      function = $2
      if function == base_function
        return Integer(address, 16) + Integer(offset, 16)
      end
    end
  end
  raise "can't resolve relative address: #{relative_address}: #{path}"
end

def addr2line(path, address)
  run_command("addr2line", "--exe=#{path}", "%#x" % address)
end

system_version = detect_system_version
prepare_system(system_version, options)

ARGF.each_line do |line|
  case line
  when /\A\d{4}-\d{2}-\d{2} [^ ]+: (\/[^ (\[]+)\((.+?)\)\s*\[(.+?)\]/
    path = $1
    relative_address = $2
    absolute_address = $3
    debug_path = resolve_debug_path(path, system_version)
    puts(line)
    case path
    when /libgroonga/
      relative_address = resolve_relative_address(relative_address, debug_path)
      addr2line(debug_path, relative_address)
    end
  end
end
