#!/bin/ruby

require 'open-uri'
require 'yaml'

def apache_arrow_source_dir
  "apache-arrow-source"
end

def detect_latest_release_version
  version_yaml =
    YAML.load(
      URI.parse(
        "https://raw.githubusercontent.com/apache/arrow-site/master/_data/versions.yml"
      ).read
    )
  version_yaml["current"]["number"]
end

def update_bundled_apache_arrow
  Dir.chdir(apache_arrow_source_dir) do
    system("git", "fetch", "--all", "--prune", "--tags", "--force")
    system("git", "checkout", "apache-arrow-#{detect_latest_release_version}")
  end
  system("git", "add", "#{apache_arrow_source_dir}")
end

update_bundled_apache_arrow
