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
workflow_runs_response = client.workflow_runs("groonga/groonga",
                                              "windows-msvc.yml",
                                              branch: tag)
downloaded = false
workflow_runs_response.workflow_runs.each do |workflow_run|
  artifacts_response =
    client.get("/repos/groonga/groonga/actions/runs/#{workflow_run.id}/artifacts")
  next if artifacts_response.total_count.zero?

  artifacts_response.artifacts.each do |artifact|
    name = artifact.name
    unless target_type == "all"
      next unless name.end_with?(target_type)
    end
    id = artifact.id
    FileUtils.mkdir_p(output_directory)
    puts("Downloading #{name}...")
    File.open("#{output_directory}/#{name}.zip", "wb") do |output|
      output.print(client.get("/repos/groonga/groonga/actions/artifacts/#{id}/zip"))
    end
    downloaded = true
    break unless target_type == "all"
  end
  break
end

unless downloaded
  raise "No artifacts on GitHub Actions"
end
