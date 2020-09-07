# Copyright(C) 2014-2020  Sutou Kouhei <kou@clear-code.com>
# Copyright(C) 2020  Horimoto Yasuhiro <horimoto@clear-code.com>
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

require_relative "../vendor/apache-arrow-source/dev/tasks/linux-packages/package-task"
require_relative "launchpad-helper"
require_relative "repository-helper"
require_relative "github-release-helper"

class PackagesGroongaOrgPackageTask < PackageTask
  include LaunchpadHelper
  include RepositoryHelper
  include GitHubReleaseHelper

  def define
    super
    define_release_tasks
    define_ubuntu_tasks
    define_upload_task
  end

  private
  def detect_release_time
    release_time_env = ENV["RELEASE_TIME"]
    if release_time_env
      Time.parse(release_time_env).utc
    else
      Time.now.utc
    end
  end

  def define_apt_task
    namespace :apt do
      desc "Clean APT repositories"
      task :clean do
        rm_rf("#{apt_dir}/repositories")
      end
    end
    task apt: ["apt:clean"]
    super
  end

  def define_yum_task
    namespace :yum do
      desc "Clean Yum repositories"
      task :clean do
        rm_rf("#{yum_dir}/repositories")
      end
    end
    task yum: ["yum:clean"]
    super
  end

  def release(target_namespace)
    base_dir = __send__("#{target_namespace}_dir")
    repositories_dir = "#{base_dir}/repositories"
    sh("rsync",
       "-av",
       "#{repositories_dir}/",
       "#{repository_rsync_base_path}/")
  end

  def define_release_tasks
    [:apt, :yum].each do |target_namespace|
      namespace target_namespace do
        desc "Release #{target_namespace} packages"
        task :release do
          release(target_namespace) if __send__("enable_#{target_namespace}?")
        end
      end
      task target_namespace => ["#{target_namespace}:release"]
    end
  end

  def download_packages(target_namespace)
    package_archives = []
    if target_namespace.equal?(:yum)
      yum_targets_default.each do |target|
        download_url = "#{release_base_url}/#{release_version}/#{target}.tar.gz"
        package_archives << "#{target}.tar.gz"
        sh("wget", "#{download_url}")
      end
    elsif target_namespace.equal?(:apt)
      apt_targets_default.each do |target|
        if target.end_with?("i386")
          download_url = "#{release_base_url}/#{release_version}/#{target}.tar.gz"
          package_archives << "#{target}.tar.gz"
        else
          download_url = "#{release_base_url}/#{release_version}/#{target}-amd64.tar.gz"
          package_archives << "#{target}-amd64.tar.gz"
        end
        sh("wget", "#{download_url}")
      end
    end
    unless package_archives.empty?
      package_archives.each do |archive|
        sh("tar", "-zxvf", "#{archive}")
      end
    end
  end

  def upload_packages(target_namespace)
    sh("rync",
       "-av",
       "packages/#{target_namespace.to_s}/repositories/",
       "#{repository_rsync_base_path}/")
  end

  def upload(target_namespace)
    download_packages(target_namespace)
    upload_packages(target_namespace)
    sh("rm", "-rf", "packages")
  end

  def define_upload_task
    [:apt, :yum].each do |target_namespace|
      namespace target_namespace do
        desc "Upload #{target_namespace} packages"
        task :upload_packages do
          upload(target_namespace) if __send__("enable_#{target_namespace}?")
        end
      end
      task target_namespace => ["#{target_namespace}:release"]
    end
  end

  def dput_configuration_name
    ENV["DPUT_CONFIGUARATION_NAME"] || "groonga-ppa"
  end

  def dput_incoming
    ENV["DPUT_INCOMING"] || "~groonga/ppa/ubuntu/"
  end
end
