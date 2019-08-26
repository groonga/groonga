#!/usr/bin/env ruby
#
# Copyright(C) 2014-2019  Kouhei Sutou <kou@clear-code.com>
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

require "optparse"
require "fileutils"
require "pathname"

class Uploader
  def initialize
    @dput_configuration_name = "groonga-ppa"
    @dput_incoming = "~groonga/ppa/ubuntu/"
  end

  def run
    parse_command_line!

    ensure_dput_configuration

    @ubuntu_code_names.zip(@ubuntu_versions) do |code_name, version|
      upload(code_name, version || code_name)
    end
  end

  private
  def ensure_dput_configuration
    dput_cf_path = Pathname.new("~/.dput.cf").expand_path
    if dput_cf_path.exist?
      dput_cf_content = dput_cf_path.read
    else
      dput_cf_content = ""
    end
    dput_cf_content.each_line do |line|
      return if line.chomp == "[#{@dput_configuration_name}]"
    end

    dput_cf_path.open("w") do |dput_cf|
      dput_cf.puts(dput_cf_content)
      dput_cf.puts(<<-CONFIGURATION)
[#{@dput_configuration_name}]
fqdn = ppa.launchpad.net
method = ftp
incoming = #{@dput_incoming}
login = anonymous
allow_unsigned_uploads = 0
      CONFIGURATION
    end
  end

  def parse_command_line!

    parser = OptionParser.new
    parser.on("--package=NAME",
              "The package name") do |name|
      @package = name
    end
    parser.on("--version=VERSION",
              "The version") do |version|
      @version = version
    end
    parser.on("--source-archive=ARCHIVE",
              "The source archive") do |source_archive|
      @source_archive = Pathname.new(source_archive).expand_path
    end
    parser.on("--code-names=CODE_NAME1,CODE_NAME2,CODE_NAME3,...", Array,
              "The target code names",
              "Deprecated. Use --ubuntu-code-names instead.") do |code_names|
      @ubuntu_code_names = code_names
    end
    parser.on("--ubuntu-code-names=CODE_NAME1,CODE_NAME2,CODE_NAME3,...", Array,
              "The target code names") do |code_names|
      @ubuntu_code_names = code_names
    end
    parser.on("--ubuntu-versions=VERSION1,VERSION2,VERSION3,...", Array,
              "The target versions") do |versions|
      @ubuntu_versions = versions
    end
    parser.on("--debian-directory=DIRECTORY",
              "The debian/ directory") do |debian_directory|
      @debian_directory = Pathname.new(debian_directory).expand_path
    end
    parser.on("--pgp-sign-key=KEY",
              "The PGP key to sign .changes and .dsc") do |pgp_sign_key|
      @pgp_sign_key = pgp_sign_key
    end
    parser.on("--ppa=PPA",
              "The personal package archive name (groonga-ppa or groonga-nightly",
              "(#{@dput_configuration_name})") do |ppa|
      @dput_configuration_name = ppa
    end
    parser.on("--ppa-incoming=INCOMING",
              "The incoming value in dput.cf for personal package archive",
              "(#{@dput_incoming})") do |incoming|
      @dput_incoming = incoming
    end

    parser.parse!
  end

  def upload(ubuntu_code_name, ubuntu_version)
    in_temporary_directory do
      archive_basename, archive_suffix = @source_archive.to_s.split(".tar.")
      FileUtils.cp(@source_archive.to_s,
                   "#{@package}_#{@version}.orig.tar.#{archive_suffix}")
      run_command("tar", "xf", @source_archive.to_s)
      directory_name = File.basename(archive_basename)
      Dir.chdir(directory_name) do
        FileUtils.cp_r(@debian_directory.to_s, "debian")
        if ubuntu_version
          version_suffix = "ubuntu#{ubuntu_version}.1"
        else
          version_suffix = "#{ubuntu_code_name}1"
        end
        deb_version = "#{current_deb_version.succ}~#{version_suffix}"
        run_command("dch",
                    "--distribution", ubuntu_code_name,
                    "--newversion", deb_version,
                    "Build for #{ubuntu_code_name}.")
        run_command("debuild",
                    "--set-envvar=LINTIAN_PROFILE=ubuntu",
                    # Workaround for Launchpad. Launchpad doesn't accept
                    # .buildinfo yet.
                    # See also: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=853795
                    "--buildinfo-option=-O",
                    "-d",
                    "-S",
                    "-sa",
                    "-k#{@pgp_sign_key}")
        run_command("dput", @dput_configuration_name,
                    "../#{@package}_#{deb_version}_source.changes")
      end
    end
  end

  def current_deb_version
    /\((.+)\)/ =~ File.read("debian/changelog").lines.first
    $1
  end

  def in_temporary_directory
    name = "tmp"
    FileUtils.rm_rf(name)
    FileUtils.mkdir_p(name)
    Dir.chdir(name) do
      yield
    end
  end

  def run_command(*command_line)
    unless system(*command_line)
      raise "failed to run command: #{command_line.join(' ')}"
    end
  end
end

uploader = Uploader.new
uploader.run
