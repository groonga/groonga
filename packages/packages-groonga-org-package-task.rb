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

class PackagesGroongaOrgPackageTask < PackageTask
  include LaunchpadHelper
  include RepositoryHelper

  def define
    super
    define_release_tasks
    define_ubuntu_tasks
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

  def use_built_package?
    false
  end

  def built_package_url(target_namespace, target)
    raise NotImplementedError
  end

  def built_package_n_split_components
    0
  end

  def download_packages(target_namespace)
    base_dir = __send__("#{target_namespace}_dir")
    repositories_dir = "#{base_dir}/repositories"
    mkdir_p(repositories_dir)
    __send__("#{target_namespace}_targets").each do |target|
      url = built_package_url(target_namespace, target)
      archive = File.expand_path(url.split("/").last)
      rm_f(archive)
      sh("wget", url)
      cd(repositories_dir) do
        sh("tar",
           "xf", archive,
           "--strip-components=#{built_package_n_split_components}")
      end
      rm_f(archive)
    end
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
      tasks = []
      namespace target_namespace do
        enabled = __send__("enable_#{target_namespace}?")
        if use_built_package?
          target_task = Rake.application[target_namespace]
          target_task.prerequisites.delete("#{target_namespace}:build")
          desc "Download #{target_namespace} packages"
          task :download do
            download_packages(target_namespace) if enabled
          end
          tasks << "#{target_namespace}:download"
        end

        desc "Release #{target_namespace} packages"
        task :release do
          release(target_namespace) if enabled
        end
        tasks << "#{target_namespace}:release"
      end
      task target_namespace => tasks
    end
  end

  def dput_configuration_name
    ENV["DPUT_CONFIGUARATION_NAME"] || "groonga-ppa"
  end

  def dput_incoming
    ENV["DPUT_INCOMING"] || "~groonga/ppa/ubuntu/"
  end
end
