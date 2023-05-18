# Copyright(C) 2014-2021  Sutou Kouhei <kou@clear-code.com>
# Copyright(C) 2020-2023  Horimoto Yasuhiro <horimoto@clear-code.com>
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

require_relative "../vendor/apache-arrow-source/dev/tasks/linux-packages/helper"
require_relative "../vendor/apache-arrow-source/dev/tasks/linux-packages/package-task"
require_relative "launchpad-helper"
require_relative "repository-helper"

class PackagesGroongaOrgPackageTask < PackageTask
  include Helper::ApacheArrow
  include LaunchpadHelper
  include RepositoryHelper

  def define
    super
    define_source_task
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

  def enable_source?
    true
  end

  def source_dir
    "source"
  end

  def source_targets
    return [] unless enable_source?

    targets = (ENV["SOURCE_TARGETS"] || "").split(",")
    return targets unless targets.empty?

    source_targets_default
  end

  def version_of_begin_redirection
    return [] unless enable_source?

    version_of_begin_redirection_default
  end

  def define_source_task
    namespace :source do
      desc "Clean files for source"
      task :clean do
        rm_rf("#{source_dir}/repositories")
      end
    end
    desc "Release files for Source"
    task source: ["source:clean"]
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
    when :source
      "#{base_dir}/repositories/source/#{@package}"
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
      when :source
        cd(repositories_dir) do
          cp(archive, ".")
          archive_base_name = File.basename(archive)
          sh("gpg",
             "--local-user", repository_gpg_key_ids[0],
             "--armor",
             "--detach-sign",
             archive_base_name)
          latest_link_base_name = archive_base_name.gsub(@version, "latest")
          ln_sf(archive_base_name, latest_link_base_name)
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

    case target_namespace
    when :source
      rsync_dir =
        "#{repository_rsync_base_path}/#{target_namespace}/#{@package}/"
      sh("rsync",
         "-av",
         "--include=.htaccess",
         "--exclude=*",
         rsync_dir,
         "#{repositories_dir}/")
    end
  end

  def redirect_target_versions
    first_redirect_target_version = __send__("version_of_begin_redirection")
    redirect_target_versions = [first_redirect_target_version]

    micro_version = first_redirect_target_version.split('.').last.to_i
    minor_version = first_redirect_target_version.split('.')[1].to_i
    major_version = first_redirect_target_version.split('.').first.to_i

    loop do
      if micro_version == 9
        if minor_version == 9
          major_version = major_version.next
          minor_version = 0
        else
          minor_version = minor_version.next
          micro_version = 0
        end
      else
        micro_version = micro_version.next
      end
      next_redirect_target_version = "#{major_version}.#{minor_version}.#{micro_version}"
      redirect_target_versions << next_redirect_target_version
      break if next_redirect_target_version == @version
    end

    redirect_target_versions
  end

  def prepare(target_namespace)
    case target_namespace
    when :windows, :source
      repositories_dir = download_repositories_dir(target_namespace)
      mkdir_p(repositories_dir)

      htaccess_path = "#{repositories_dir}/.htaccess"
      if File.exist?(htaccess_path)
        htaccess_content = File.read(htaccess_path)
      else
        htaccess_content = ""
      end
      File.open(htaccess_path, "w") do |htaccess|
        htaccess_content.each_line do |line|
          htaccess.puts(line) unless line.include?("-latest")
        end

        if target_namespace == :windows
          __send__("#{target_namespace}_targets").each do |target|
            redirect_url = built_package_url(target_namespace, target)
            [
              target,
              target.gsub(@version, "latest")
            ].each do |redirect_target_file|
              redirect_target =
                "/#{target_namespace}/#{@package}/#{redirect_target_file}"
              htaccess.puts("Redirect #{redirect_target} #{redirect_url}")
              if target_namespace == :source
                htaccess.puts("Redirect #{redirect_target}.asc #{redirect_url}.asc")
              end
            end
          end
        else
          redirect_target_versions.each do |redirect_target_version|
            __send__("#{target_namespace}_targets").each do |target|
              redirect_url = built_package_url(target_namespace, target)
              if @version == redirect_target_version
                [
                  target,
                  target.gsub(@version, "latest")
                ].each do |redirect_target_file|
                  redirect_target =
                    "/#{target_namespace}/#{@package}/#{redirect_target_file}"
                  htaccess.puts("Redirect #{redirect_target} #{redirect_url}")
                  htaccess.puts("Redirect #{redirect_target}.asc #{redirect_url}.asc")
                end
              else
                redirect_target_file = target.gsub(@version, redirect_target_version)
                redirect_url.gsub!(@version, redirect_target_version)
                redirect_target =
                  "/#{target_namespace}/#{@package}/#{redirect_target_file}"
                htaccess.puts("Redirect #{redirect_target} #{redirect_url}")
                htaccess.puts("Redirect #{redirect_target}.asc #{redirect_url}.asc")
              end
            end
          end
        end
      end
    end
  end

  def release(target_namespace)
    base_dir = __send__("#{target_namespace}_dir")
    repositories_dir = "#{base_dir}/repositories"
    destination = "#{repository_rsync_base_path}/"
    rsync_options = ["-av"]
    case target_namespace
    when :source
      sh({"GH_TOKEN" => github_access_token},
         "gh",
         "release",
         "upload",
         @version,
         "--clobber",
         "--repo", github_repository,
         *Dir.glob("#{repositories_dir}/#{target_namespace}/#{@package}/*.asc"))
      rsync_options << "--exclude=*.asc"
      rsync_options << "--exclude=*.tar.gz"
      rsync_options << "--exclude=*.zip"
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
    [:apt, :yum, :source, :windows].each do |target_namespace|
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
