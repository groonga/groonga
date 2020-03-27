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

client = Octokit::Client.new

groonga_repository = "groonga/groonga"

appveyor_url = "https://ci.appveyor.com/"
appveyor_info = nil
last_appveyor_info = nil
client.statuses(groonga_repository, tag).each do |status|
  next unless status.target_url.start_with?(appveyor_url)

  match_data = /\/([^\/]+?)\/([^\/]+?)\/builds\/(\d+)\z/.match(status.target_url)
  last_appveyor_info = {
    account: match_data[1],
    project: match_data[2],
    build_id: match_data[3],
  }
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
build_history = Veyor.project_history(account: appveyor_info[:account],
                                      project: appveyor_info[:project],
                                      start_build: start_build,
                                      limit: 1)
build_version = build_history["builds"][0]["buildNumber"]
project = Veyor.project(account: appveyor_info[:account],
                        project: appveyor_info[:project],
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
  url = "#{appveyor_url}project/#{groonga_repository}/builds/#{build_id}"
  raise "No artifacts on AppVeyor: #{url}"
end
