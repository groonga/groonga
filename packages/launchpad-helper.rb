# Copyright(C) 2014-2020  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

require "pathname"

module LaunchpadHelper
  private
  def enable_ubuntu?
    true
  end

  def ubuntu_targets
    return [] unless enable_ubuntu?

    targets = (ENV["UBUNTU_TARGETS"] || "").split(",")
    return targets.each_slice(2).to_a unless targets.empty?

    ubuntu_targets_default
  end

  def ubuntu_targets_default
    [
      ["bionic", "18.04"],
      ["focal", "20.04"],
      ["groovy", "20.10"],
      ["hirsute", "21.04"],
    ]
  end

  def ubuntu_dir
    "ubuntu"
  end

  def dput_configuration_name
    env_value("DPUT_CONFIGUARATION_NAME")
  end

  def dput_incoming
    env_value("DPUT_INCOMING")
  end

  def ubuntu_pgp_sign_key
    env_value("LAUNCHPAD_UPLOADER_PGP_KEY")
  end

  def ensure_dput_configuration
    dput_cf_path = Pathname.new("~/.dput.cf").expand_path
    if dput_cf_path.exist?
      dput_cf_content = dput_cf_path.read
    else
      dput_cf_content = ""
    end
    dput_cf_content.each_line do |line|
      return if line.chomp == "[#{dput_configuration_name}]"
    end

    dput_cf_path.open("w") do |dput_cf|
      dput_cf.puts(dput_cf_content)
      dput_cf.puts(<<-CONFIGURATION)
[#{dput_configuration_name}]
fqdn = ppa.launchpad.net
method = ftp
incoming = #{dput_incoming}
login = anonymous
allow_unsigned_uploads = 0
      CONFIGURATION
    end
  end

  def detect_current_deb_version
    /\((.+)\)/ =~ File.read("debian/changelog").lines.first
    $1
  end

  def ubuntu_upload(code_name, version)
    ensure_dput_configuration

    absoulte_deb_archive_name = File.expand_path(deb_archive_name)
    tmp_dir = "#{ubuntu_dir}/tmp"
    rm_rf(tmp_dir)
    mkdir_p(tmp_dir)
    apt_prepare_debian_dir(tmp_dir, "ubuntu-#{code_name}")
    cd(tmp_dir) do
      deb_archive_base_name = File.basename(absoulte_deb_archive_name, ".tar.gz")
      sh("tar", "xf", absoulte_deb_archive_name)
      cp(absoulte_deb_archive_name,
         "#{@package}_#{@deb_upstream_version}.orig.tar.gz")
      cd(deb_archive_base_name) do
        cp_r("../debian.ubuntu-#{code_name}", "debian")
        minor_version = ENV["UBUNTU_MINOR_VERSION"] || "1"
        version_suffix = "ubuntu#{version}.#{minor_version}"
        deb_version = "#{detect_current_deb_version}.#{version_suffix}"
        sh("dch",
           "--distribution", code_name,
           "--newversion", deb_version,
           "Build for #{code_name}.")
        sh("debuild",
           "--set-envvar=LINTIAN_PROFILE=ubuntu",
           # Workaround for Launchpad. Launchpad doesn't accept
           # .buildinfo yet.
           # See also: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=853795
           "--buildinfo-option=-O",
           "-d",
           "-S",
           "-sa",
           "-k#{ubuntu_pgp_sign_key}")
        sh("dput",
           dput_configuration_name,
           "../#{@package}_#{deb_version}_source.changes")
      end
    end
  end

  def define_ubuntu_tasks
    namespace :ubuntu do
      upload_task_names = []
      namespace :upload do
        ubuntu_targets.each do |code_name, version|
          upload_task_names << "ubuntu:upload:#{code_name}"
          desc "Upload Ubuntu package for #{code_name}"
          task code_name => deb_archive_name do
            ubuntu_upload(code_name, version)
          end
        end
      end

      desc "Upload Ubuntu packages"
      task :upload => upload_task_names
    end

    desc "Release Ubuntu packages"
    task :ubuntu => ["ubuntu:upload"]
  end
end
