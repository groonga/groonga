#!/usr/bin/env ruby

require "fileutils"

require "octokit"

if ARGV.size != 3
  $stderr.puts("Usage: #{$0} TAG OUTPUT_DIRECTORY TYPE")
  $stderr.puts(" e.g.: #{$0} v9.0.6 files all")
  exit(false)
end

tag, output_directory, target_type = ARGV

client = Octokit::Client.new
client.access_token = ENV["GITHUB_ACCESS_TOKEN"]
artifacts_response = nil
if tag == "master"
  artifacts_response = client.get("/repos/groonga/groonga/actions/artifacts")
else
  workflow_runs_response = client.workflow_runs("groonga/groonga",
                                                "windows-msvc.yml",
                                                branch: tag)
  workflow_run_id = workflow_runs_response.to_a[1][1][0][:id]
  artifacts_response =
    client.get("/repos/groonga/groonga/actions/runs/#{workflow_run_id}/artifacts")
end

downloaded = false
artifacts_response.to_a[1][1].each do |artifact|
  name = artifact[:name]
  unless target_type == "all"
    next unless name.end_with?(target_type)
  end
  id = artifact[:id]
  FileUtils.mkdir_p(output_directory)
  File.open("#{output_directory}/#{name}.zip", "wb") do |output|
    output.print(client.get("/repos/groonga/groonga/actions/artifacts/#{id}/zip"))
  end
  downloaded = true
  break unless target_type == "all"
end

unless downloaded
  raise "No artifacts on GitHub Actions"
end
