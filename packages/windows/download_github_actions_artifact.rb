#!/usr/bin/env ruby

require "fileutils"
require "open-uri"

require "octokit"

if ARGV.size < 3 || ARGV.size > 4
  $stderr.puts("Usage: #{$0} TAG OUTPUT_DIRECTORY TYPE [OWNER]")
  $stderr.puts(" e.g.: #{$0} v9.0.6 files all")
  $stderr.puts(" e.g.: #{$0} v9.0.6 files all groonga")
  exit(false)
end

tag, output_directory, target_type, owner = ARGV
owner ||= "groonga"
repository = "#{owner}/groonga"

client = Octokit::Client.new
client.access_token = ENV["GITHUB_ACCESS_TOKEN"]
artifacts_response = nil
workflow_runs_response = client.workflow_runs(repository,
                                              "cmake.yml",
                                              branch: tag)
downloaded = false
workflow_runs_response.workflow_runs.each do |workflow_run|
  artifacts_response =
    client.get("/repos/#{repository}/actions/runs/#{workflow_run.id}/artifacts")
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
      uri = URI.parse(client.artifact_download_url(repository, id))
      uri.open do |input|
        IO.copy_stream(input, output)
      end
    end
    downloaded = true
    break unless target_type == "all"
  end
  break
end

unless downloaded
  raise "No artifacts on GitHub Actions"
end
