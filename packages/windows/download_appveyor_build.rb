#!/usr/bin/env ruby

require "pathname"
require "open-uri"
require "veyor"
require "octokit"

if ARGV.size != 3
  $stderr.puts("Usage: #{$0} TAG OUTPUT_DIRECTORY TYPE")
  $stderr.puts(" e.g.: #{$0} v9.0.6 files all")
  exit(false)
end

tag, output_directory, target_type = ARGV

def download(url, local_path)
  URI(url).open do |remote_file|
    File.open(local_path, "wb") do |local_file|
      local_file.print(remote_file.read)
    end
  end
end

appveyor_url = "https://ci.appveyor.com/"
appveyor_account = "groonga"
appveyor_project = "groonga"

build_version = nil
if tag == "master"
  n_builds = 50
  build_history = Veyor.project_history(account: appveyor_account,
                                        project: appveyor_project,
                                        limit: n_builds)
  build_history["builds"].each do |build|
    if build["status"] == "success"
      build_version = build["buildNumber"]
      break
    end
  end
  if build_version.nil?
    raise "No AppVeyor success build in the last #{n_builds} builds"
  end
else
  appveyor_info = nil
  last_appveyor_info = nil
  groonga_repository = "groonga/groonga"

  client = Octokit::Client.new
  client.statuses(groonga_repository, tag).each do |status|
    pp status
    next unless status.target_url.start_with?(appveyor_url)

    match_data = /\/([^\/]+?)\/([^\/]+?)\/builds\/(\d+)\z/.match(status.target_url)
    last_appveyor_info = {
      account: match_data[1],
      project: match_data[2],
      build_id: match_data[3],
    }
    pp [status.state, last_appveyor_info]
    unless status.state == "pending"
      appveyor_info = last_appveyor_info
      break
    end
  end
  appveyor_info ||= last_appveyor_info
  if appveyor_info.nil?
    raise "No AppVeyor build"
  end

  start_build = appveyor_info[:build_id].to_i + 1
  build_history = Veyor.project_history(account: appveyor_account,
                                        project: appveyor_project,
                                        start_build: start_build,
                                        limit: 1)
  build_version = build_history["builds"][0]["buildNumber"]
end

project = Veyor.project(account: appveyor_account,
                        project: appveyor_project,
                        version: build_version)
downloaded = false
project["build"]["jobs"].each do |job|
  job_id = job["jobId"]
  artifacts = Veyor.build_artifacts(job_id: job_id)
  artifacts.each do |artifact|
    file_name = artifact["fileName"]
    unless target_type == "all"
      next unless File.basename(file_name, ".zip").end_with?(target_type)
    end
    url = "#{appveyor_url}api/buildjobs/#{job_id}/artifacts/#{file_name}"
    download(url, "#{output_directory}/#{file_name}")
    downloaded = true
  end
end

unless downloaded
  build_id = project["build"]["buildId"]
  url = "#{appveyor_url}project/#{appveyor_account}/#{appveyor_project}/builds/#{build_id}"
  raise "No artifacts on AppVeyor: #{url}"
end
