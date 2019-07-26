#!/usr/bin/env ruby

require "pathname"
require "open-uri"
require "veyor"
require "octokit"

def download(url, local_file)
  open(url) do |remote_zip|
    File.open(local_file, "wb") do |local_zip|
      local_zip.print(remote_zip.read)
    end
  end
end

client = Octokit::Client.new

groonga_repository = "groonga/groonga"

base_dir = Pathname.new(__FILE__).expand_path.dirname
top_dir = base_dir.parent.parent
base_version = (top_dir + "base_version").read.strip
tag_name = "v#{base_version}"

appveyor_url = "https://ci.appveyor.com/"
appveyor_info = nil
client.statuses(groonga_repository, tag_name).each do |status|
  next unless status.target_url.start_with?(appveyor_url)
  case status.state
  when "pending"
    # Ignore
  else
    match_data = /\/([^\/]+?)\/([^\/]+?)\/builds\/(\d+)\z/.match(status.target_url)
    appveyor_info = {
      account: match_data[1],
      project: match_data[2],
      build_id: match_data[3],
    }
    break
  end
end
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
project["build"]["jobs"].each do |job|
  job_id = job["jobId"]
  artifacts = Veyor.build_artifacts(job_id: job_id)
  artifacts.each do |artifact|
    file_name = artifact["fileName"]
    url = "#{appveyor_url}api/buildjobs/#{job_id}/artifacts/#{file_name}"
    download(url, "files/#{file_name}")
  end
end
