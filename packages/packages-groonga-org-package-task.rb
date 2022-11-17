# Copyright(C) 2014-2021  Sutou Kouhei <kou@clear-code.com>
# Copyright(C) 2020-2021  Horimoto Yasuhiro <horimoto@clear-code.com>
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
    define_windows_task
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

  def enable_windows?
    true
  end

  def windows_dir
    "windows"
  end

  def windows_targets
    return [] unless enable_windows?

    targets = (ENV["WINDOWS_TARGETS"] || "").split(",")
    return targets unless targets.empty?

    windows_targets_default
  end

  def define_windows_task
    namespace :windows do
      desc "Clean files for Windows"
      task :clean do
        rm_rf("#{windows_dir}/repositories")
      end
    end
    desc "Release files for Windows"
    task windows: ["windows:clean"]
  end

  def use_built_package?
    false
  end

  def github_repository
    raise NotImplementedError
  end

  def github_actions_workflow_file_name(target_namespace, target)
    raise NotImplementedError
  end

  def github_actions_artifact_name(target_namespace, target)
    raise NotImplementedError
  end

  def github_access_token
    ENV["GITHUB_ACCESS_TOKEN"]
  end

  def detect_built_package_url_github_actions(target_namespace,
                                              target,
                                              branch,
                                              artifact_name)
    require "octokit"
    client = Octokit::Client.new
    client.access_token = github_access_token
    workflow_file_name = github_actions_workflow_file_name(target_namespace,
                                                           target)
    workflow_runs_response = client.workflow_runs(github_repository,
                                                  workflow_file_name,
                                                  branch: branch)
    workflow_runs_response.workflow_runs.each do |workflow_run|
      artifacts_response = client.get(workflow_run.artifacts_url)
      next if artifacts_response.total_count.zero?

      artifacts_response.artifacts.each do |artifact|
        if artifact.name == artifact_name
          return artifact.archive_download_url
        end
      end
    end
    message = "can't detect built package URL:"
    message << " target_namespace=<#{target_namespace.inspect}>"
    message << " target=<#{target.inspect}>"
    message << " branch=<#{branch.inspect}>"
    message << " workflow_file_name=<#{workflow_file_name.inspect}>"
    message << " artifact_name=<#{artifact_name.inspect}>"
    raise message
  end

  def built_package_url(target_namespace, target)
    raise NotImplementedError
  end

  def built_package_n_split_components
    0
  end

  def open_url(url, &block)
    if url.start_with?("https://api.github.com/")
      access_token = github_access_token
      URI(url).open("Authorization" => "token #{access_token}",
                    &block)
    else
      super
    end
  end

  def download_repositories_dir(target_namespace)
    base_dir = __send__("#{target_namespace}_dir")
    case target_namespace
    when :windows
      "#{base_dir}/repositories/windows/#{@package}"
    else
      "#{base_dir}/repositories"
    end
  end

  def download_packages(target_namespace)
    base_dir = __send__("#{target_namespace}_dir")
    repositories_dir = download_repositories_dir(target_namespace)
    mkdir_p(repositories_dir)
    download_dir = "#{base_dir}/tmp/downloads/#{@version}"
    mkdir_p(download_dir)
    __send__("#{target_namespace}_targets").each do |target|
      branch = ENV["BRANCH"]
      if branch
        artifact_name = github_actions_artifact_name(target_namespace, target)
        url = detect_built_package_url_github_actions(target_namespace,
                                                      target,
                                                      branch,
                                                      artifact_name)
        download_path = "#{download_dir}/#{artifact_name}.zip"
        archive = download(url, download_path)
      else
        url = built_package_url(target_namespace, target)
        archive = download(url, download_dir)
      end
      case target_namespace
      when :apt, :yum
        cd(repositories_dir) do
          case File.extname(archive)
          when ".zip"
            sh("unzip", "-o", archive)
          else
            sh("tar",
               "xf", archive,
               "--strip-components=#{built_package_n_split_components}")
          end
        end
      when :windows
        cd(repositories_dir) do
          cp(archive, ".")
          archive_base_name = File.basename(archive)
          latest_link_base_name = archive_base_name.gsub(@version, "latest")
          ln_sf(archive_base_name, latest_link_base_name)
        end
      end
    end
  end

  def prepare(target_namespace)
  end

  def release(target_namespace)
    base_dir = __send__("#{target_namespace}_dir")
    repositories_dir = "#{base_dir}/repositories"
    destination = "#{repository_rsync_base_path}/"
    rsync_options = ["-av"]
    case target_namespace
    when :windows
    else
      rsync_options << "--exclude=*.buildinfo"
      rsync_options << "--exclude=*.changes"
      destination << "incoming/"
    end
    sh("rsync",
       *rsync_options,
       "#{repositories_dir}/",
       destination)
  end

  def define_release_tasks
    [:apt, :yum, :windows].each do |target_namespace|
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

        desc "Prepare #{target_namespace} packages"
        task :prepare do
          prepare(target_namespace) if enabled
        end
        tasks << "#{target_namespace}:prepare"

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
    ENV["DPUT_CONFIGURATION_NAME"] || "groonga-ppa"
  end

  def dput_incoming
    ENV["DPUT_INCOMING"] || "~groonga/ppa/ubuntu/"
  end
end
