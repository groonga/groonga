#!/usr/bin/env ruby

require "English"
require "optparse"
require "ostruct"
require "tempfile"

options = OpenStruct.new
options.version = nil
options.pgroonga_version = nil
options.postgresql_version = nil
options.use_mecab = false

parser = OptionParser.new
parser.banner += "\n  Parse Groonga and its families' backtrace in log"
parser.on("--version=VERSION",
          "Groonga version",
          "e.g.: 10.0.9",
          "(#{options.version})") do |version|
  options.version = version
end
parser.on("--pgroonga-version=VERSION",
          "PGroonga version",
          "e.g.: 3.0.0",
          "(#{options.pgroogna_version})") do |version|
  options.pgroonga_version = version
end
parser.on("--postgresql-version=VERSION",
          "PostgreSQL version",
          "e.g.: 15.2",
          "(#{options.postgresql_version})") do |version|
  options.postgresql_version = version
end
parser.on("--[no-]use-mecab",
          "Use MeCab",
          "(#{options.use_mecab})") do |boolean|
  options.use_mecab = boolean
end
parser.parse!

def detect_system_version
  system_release_cpe = "/etc/system-release-cpe"
  os_release = "/etc/os-release"
  if File.exist?(system_release_cpe)
    components = File.read(system_release_cpe).chomp.split(":")
    if components[-1] == "baseos"
      components.pop
      components.pop
    end
    version = components[-1]
    system_id = components[-3]
    "#{system_id}-#{version}"
  elsif File.exist?(os_release)
    variables = {}
    File.readlines(os_release, chomp: true).each do |line|
      key, value = line.split("=", 2)
      value = value.gsub(/\A"(.*?)"\z/, "\\1")
      variables[key] = value
    end
    id = variables["ID"]
    version_id = variables["VERSION_ID"]
    "#{id}-#{version_id}"
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
  if options.use_mecab
    packages << "groonga-tokenizer-mecab#{groonga_package_version}"
    packages << "groonga-tokenizer-mecab-debuginfo#{groonga_package_version}"
  end
  run_command("yum", "install", "-y", *packages)
end

def prepare_system_almalinux_8(options)
  run_command("dnf", "install", "-y",
              "epel-release",
              "https://packages.groonga.org/almalinux/8/groonga-release-latest.noarch.rpm")
  run_command("dnf", "install", "-y", "binutils")
  packages = []
  if options.version
    groonga_package_version = "-#{options.version}-1.el8.x86_64"
  else
    groonga_package_version = ""
  end
  packages << "groonga-libs#{groonga_package_version}"
  packages << "groonga-libs-debuginfo#{groonga_package_version}"
  if options.use_mecab
    packages << "groonga-tokenizer-mecab#{groonga_package_version}"
    packages << "groonga-tokenizer-mecab-debuginfo#{groonga_package_version}"
  end
  if options.pgroonga_version and options.postgresql_version
    postgresql_major_version = options.postgresql_version.split(".")[0]
    package_prefix = "postgresql#{postgresql_major_version}-"
    postgresql_package_version =
      "-#{options.postgresql_version}-1PGDG.rhel8.x86_64"
    pgroonga_package_version = "-#{options.pgroonga_version}-1.el8.x86_64"

    run_command("dnf", "module", "-y", "disable", "postgresql")
    run_command("dnf", "install", "-y",
                "https://download.postgresql.org/pub/repos/yum/reporpms/EL-8-x86_64/pgdg-redhat-repo-latest.noarch.rpm")
    run_command("dnf", "config-manager", "--set-enabled",
                "pgdg#{postgresql_major_version}-debuginfo")
    packages << "#{package_prefix}server#{postgresql_package_version}"
    packages << "#{package_prefix}server-debuginfo#{postgresql_package_version}"
    packages << "#{package_prefix}debugsource#{postgresql_package_version}"
    packages << "#{package_prefix}pgdg-pgroonga#{pgroonga_package_version}"
    packages << "#{package_prefix}pgdg-pgroonga-debuginfo#{pgroonga_package_version}"
  end
  run_command("dnf", "install", "--enablerepo=powertools", "-y", *packages)
end

def prepare_system_amazon_2(options)
  run_command("amazon-linux-extras", "install", "epel", "-y")
  prepare_system_centos_7(options)
end

def prepare_system_debian_11(options)
  code_name = "bullseye"
  run_command("apt", "update")
  run_command("apt", "install", "-y", "-V", "wget")
  groonga_apt_source_deb = "groonga-apt-source-latest-#{code_name}.deb"
  run_command("wget",
              "https://packages.groonga.org/debian/#{groonga_apt_source_deb}")
  run_command("apt", "install", "-y", "-V", "./#{groonga_apt_source_deb}")
  run_command("apt", "update")
  packages = []
  packages << "binutils"
  if options.version
    groonga_package_version = "=#{options.version}-1"
  else
    groonga_package_version = ""
  end
  packages << "libgroonga0#{groonga_package_version}"
  packages << "libgroonga0-dbgsym#{groonga_package_version}"
  if options.use_mecab
    packages << "groonga-tokenizer-mecab#{groonga_package_version}"
    packages << "groonga-tokenizer-mecab-dbgsym#{groonga_package_version}"
  end
  if options.pgroonga_version and options.postgresql_version
    run_command("wget", "https://www.postgresql.org/media/keys/ACCC4CF8.asc")
    run_command("gpg",
                "--no-default-keyring",
                "--keyring", "/usr/share/keyrings/pgdg.gpg",
                "--import", "ACCC4CF8.asc")
    File.write("/etc/apt/sources.list.d/pgdg.sources", <<-SOURCES)
Types: deb
URIs: http://apt.postgresql.org/pub/repos/apt
Suites: #{code_name}-pgdg
Components: main
Signed-By: /usr/share/keyrings/pgdg.gpg
    SOURCES
    run_command("apt", "update")

    postgresql_major_version = options.postgresql_version.split(".")[0]
    package_prefix = "postgresql-#{postgresql_major_version}"
    postgresql_package_version =
      "=#{options.postgresql_version}-1.pgdg*"
    pgroonga_package_version = "=#{options.pgroonga_version}-1"

    packages << "#{package_prefix}#{postgresql_package_version}"
    packages << "#{package_prefix}-dbgsym#{postgresql_package_version}"
    packages << "#{package_prefix}-pgdg-pgroonga#{pgroonga_package_version}"
    packages << "#{package_prefix}-pgdg-pgroonga-dbgsym#{pgroonga_package_version}"
  end
  run_command("apt", "install", "-y", "-V", *packages)
end

def prepare_system_ubuntu_22(options)
  run_command("apt", "update")
  run_command("apt", "install", "-y", "-V", "software-properties-common")
  run_command("add-apt-repository", "-y", "universe")
  run_command("add-apt-repository", "-y", "ppa:groonga/ppa")
  run_command("add-apt-repository", "-y", "-c", "main/debug", "ppa:groonga/ppa")
  run_command("apt", "update")
  packages = []
  packages << "binutils"
  if options.version
    groonga_package_version = "=#{options.version}-1.*"
  else
    groonga_package_version = ""
  end
  packages << "libgroonga0#{groonga_package_version}"
  packages << "libgroonga0-dbgsym#{groonga_package_version}"
  if options.use_mecab
    packages << "groonga-tokenizer-mecab#{groonga_package_version}"
    packages << "groonga-tokenizer-mecab-dbgsym#{groonga_package_version}"
  end
  if options.pgroonga_version and options.postgresql_version
    run_command("apt", "install", "-y", "-V", "wget")
    run_command({"DEBIAN_FRONTEND" => "noninteractive"}, "apt", "install", "-y", "-V", "tzdata")
    run_command("wget", "https://www.postgresql.org/media/keys/ACCC4CF8.asc")
    run_command("gpg",
                "--no-default-keyring",
                "--keyring", "/usr/share/keyrings/pgdg.gpg",
                "--import", "ACCC4CF8.asc")
    File.write("/etc/apt/sources.list.d/pgdg.sources", <<-SOURCES)
Types: deb
URIs: http://apt.postgresql.org/pub/repos/apt
Suites: jammy-pgdg
Components: main
Signed-By: /usr/share/keyrings/pgdg.gpg
    SOURCES
    run_command("apt", "update")

    postgresql_major_version = options.postgresql_version.split(".")[0]
    package_prefix = "postgresql-#{postgresql_major_version}"
    postgresql_package_version =
      "=#{options.postgresql_version}-1.pgdg*"
    pgroonga_package_version = "=#{options.pgroonga_version}-1.*"

    packages << "#{package_prefix}#{postgresql_package_version}"
    packages << "#{package_prefix}-dbgsym#{postgresql_package_version}"
    # todo
    # packages << "#{package_prefix}-pgdg-pgroonga#{pgroonga_package_version}"
    # packages << "#{package_prefix}-pgdg-pgroonga-dbgsym#{pgroonga_package_version}"
    packages << "#{package_prefix}-pgroonga#{pgroonga_package_version}"
    packages << "#{package_prefix}-pgroonga-dbgsym#{pgroonga_package_version}"
  end
  run_command("apt", "install", "-y", "-V", *packages)
end

def prepare_system(system_version, options)
  case system_version
  when "centos-7"
    prepare_system_centos_7(options)
  when "almalinux-8"
    prepare_system_almalinux_8(options)
  when "amazon-2"
    prepare_system_amazon_2(options)
  when "debian-11"
    prepare_system_debian_11(options)
  when "ubuntu-22.04"
    prepare_system_ubuntu_22(options)
  else
    raise "unsupported system: #{system_version}"
  end
end

def postgresql_path(system_version, options)
  return nil unless options.postgresql_version
  postgresql_major_version = options.postgresql_version.split(".")[0]
  postgres = "/usr/pgsql-#{postgresql_major_version}/bin/postgres"
  return nil unless File.exist?(postgres)
  postgres
end

def resolve_debug_path(path, system_version)
  case system_version
  when /\Acentos-/, /\Aalmalinux-/, /\Aamazon-/
    Dir.glob("/usr/lib/debug#{path}*.debug").first || path
  when /\Adebian-/, /\Aubuntu-/
    build_id = nil
    capture_command("readelf", "-n", path).each_line do |line|
      case line
      when /\A\s*Build ID:\s*(\h+)/
        build_id = $1
        break
      end
    end
    if build_id
      debug_path = "/usr/lib/debug/.build-id/#{build_id[0, 2]}/#{build_id[2..-1]}.debug"
      return debug_path if File.exist?(debug_path)
    end
    path
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
  capture_command("addr2line", "--exe=#{path}", "%#x" % address)
end

def resolve_line(relative_address, path)
  relative_address = resolve_relative_address(relative_address, path)
  addr2line(path, relative_address)
end

system_version = detect_system_version
prepare_system(system_version, options)

cache = {}
ARGF.each_line do |line|
  unless line.valid_encoding?
    puts("invalid encoding line:")
    p(line)
    next
  end
  case line
  when /\A\d{4}-\d{2}-\d{2} [^ ]+[:|] (\/[^ \(\[]+)\((.+?)\)\s*\[(.+?)\]\Z/
    path = $1
    relative_address = $2
    absolute_address = $3
    debug_path = resolve_debug_path(path, system_version)
    puts(line)
    case path
    when /libgroonga/, /pgroonga/
      next unless File.exist?(path)
      cache_key = [relative_address, debug_path]
      cache[cache_key] ||= resolve_line(relative_address, debug_path)
      puts(cache[cache_key])
    end
  when /\A\d{4}-\d{2}-\d{2} [^ ]+[:|] (?:postgres: .+)\s*\[(.+?)\]\Z/
    absolute_address = $1
    path = postgresql_path(system_version, options)
    next if path.nil?
    debug_path = resolve_debug_path(path, system_version)
    puts(line)
    cache_key = [debug_path, absolute_address]
    cache[cache_key] ||= addr2line(debug_path, absolute_address)
    puts(cache[cache_key])
  end
end
