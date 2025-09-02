# -*- ruby -*-
#
# Copyright (C) 2023-2025  Sutou Kouhei <kou@clear-code.com>
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

require "date"
require "json"
require "open-uri"
require "tmpdir"

BASE_VERSION = File.read(File.join(__dir__, "base_version"))

def sh_capture_output(*command_line)
  IO.pipe do |read, write|
    output = nil
    read_thread = Thread.new do
      output = read.read
    end
    sh(*command_line, out: write)
    write.close
    read_thread.join
    output
  end
end

def package
  "groonga"
end

def version
  ENV["VERSION"] || BASE_VERSION
end

def base_name
  "#{package}-#{version}"
end

def archive_format
  "tar.gz"
end

def archive_name
  "#{base_name}.#{archive_format}"
end

def env_var(name, default=nil)
  value = ENV[name] || default
  raise "${#{name}} is missing" if value.nil?
  value
end

def dry_run?
  env_var("DRY_RUN", "false") == "true"
end

def git_user_name
  sh_capture_output("git", "config", "--get", "user.name").chomp
end

def git_user_email
  sh_capture_output("git", "config", "--get", "user.email").chomp
end

namespace :dev do
  namespace :version do
    desc "Bump version for new development"
    task :bump do
      next_version = env_var("NEW_VERSION", version.succ)
      File.write("base_version", next_version)
      sh("git", "add", "base_version")
      sh("git", "commit", "-m", "Start #{next_version}")
      sh("git", "push")
    end
  end

  namespace :mruby do
    desc "Update vendored mruby"
    task :update do
      mruby_version = nil
      cd("vendor/mruby-source") do
        sh("git", "fetch", "--all", "--prune", "--force")
        sh("git", "checkout", env_var("MRUBY_VERSION", "master"))
        mruby_version = sh_capture_output("git", "describe", "--tags").chomp
      end
      cd("vendor/mruby") do
        File.write("version", mruby_version)
        ruby("update.rb", "build_config.rb", "../mruby-source",
             out: "sources.am")
      end
    end
  end
end

dist_files = sh_capture_output("git", "ls-files").split("\n").reject do |file|
  file.start_with?("packages/")
end

file archive_name => dist_files do
  sh("git",
     "archive",
     "--format=tar",
     "--output=#{base_name}.tar",
     "--prefix=#{base_name}/",
     "HEAD")
  sh("git",
     "submodule",
     "foreach",
     "--recursive",
     "git " +
     "archive " +
     "--prefix=#{base_name}/${sm_path}/ " +
     "--output=$(basename ${sm_path}).tar " +
     "HEAD " +
     "&& " +
     "tar " +
     "--concatenate " +
     "--file=${toplevel}/#{base_name}.tar " +
     "$(basename ${sm_path}).tar " +
     "&& " +
     "rm $(basename ${sm_path}).tar")
  sh("gzip", "--force", "#{base_name}.tar")
end

desc "Create archive"
task dist: archive_name

namespace :document do
  desc "Generate C API document (Run doxygen)"
  task :api do
    sh("doxygen", "doc/Doxyfile")
  end
end

namespace :release do
  namespace :version do
    desc "Validate version for release"
    task :validate do
      if version != BASE_VERSION
        message = "You must NOT specify VERSION= for 'rake release'. "
        message += "You must run 'rake dev:version:bump NEW_VERSION=#{version}' "
        message += "before 'rake release'."
        raise message
      end
    end

    desc "Update versions for a new release"
    task :update do
      new_release_date = env_var("NEW_RELEASE_DATE", Date.today.iso8601)
      cd("packages") do
        ruby("-S",
             "rake",
             "version:update",
             "RELEASE_DATE=#{new_release_date}")
      end
      sh("git",
         "add",
         "packages/debian/changelog",
         "packages/yum/groonga.spec.in")
      sh("git",
         "commit",
         "-m",
         "package: update version info to #{version} (#{new_release_date})")
      sh("git", "push")
    end
  end

  namespace :document do
    desc "Update document"
    task :update do
      build_dir = env_var("BUILD_DIR")
      groonga_org_path = File.expand_path(env_var("GROONGA_ORG_DIR"))
      Dir.mktmpdir do |dir|
        sh({"DESTDIR" => dir},
           "cmake",
           "--build", build_dir,
           "--target", "install")
        Dir.glob("#{dir}/**/share/doc/groonga/*/") do |doc|
          locale = File.basename(doc)
          if locale == "en"
            groonga_org_docs = File.join(groonga_org_path, "docs")
          else
            groonga_org_docs = File.join(groonga_org_path, locale, "docs")
          end
          rm_rf(groonga_org_docs)
          mv(doc, groonga_org_docs)
          rm_f(File.join(groonga_org_docs, ".buildinfo"))
        end
      end
    end
  end

  desc "Tag"
  task :tag do
    latest_news = Dir.glob("doc/source/news/*.*").max do |a, b|
      File.basename(a).to_f - File.basename(b).to_f
    end
    latest_release_note = File.read(latest_news).split(/^## /)[1]
    latest_release_note_version = latest_release_note.lines.first[/[\d.]+/]
    if latest_release_note_version != version
      raise "release note isn't written"
    end

    changelog = "packages/debian/changelog"
    case File.readlines(changelog)[0]
    when /\((.+)-1\)/
      package_version = $1
      unless package_version == version
        raise "package version isn't updated: #{package_version}"
      end
    else
      raise "failed to detect deb package version: #{changelog}"
    end

    sh("git",
       "tag",
       "v#{version}",
       "-a",
       "-m",
       "Groonga #{version}!!!")
    sh("git", "push", "origin", "v#{version}")
  end

  desc "Post release announces"
  task :announce do
    require "x"

    latest_news = Dir.glob("doc/source/news/*.*").max do |a, b|
      File.basename(a).to_i - File.basename(b).to_i
    end
    latest_release_note = File.read(latest_news).split(/^## /)[1]
    latest_release_note_title = latest_release_note.lines.first
    latest_release_note_content = latest_release_note.lines[1..-1].join
    latest_release_note_version = latest_release_note_title[/[\d.]+/]
    if latest_release_note_version != version
      raise "release note isn't written" unless dry_run?
    end
    latest_release_note_summary =
      latest_release_note_content.split(/^### /, 2)[0].strip
    unless latest_release_note_summary
      raise "release summary isn't written"
    end
    latest_release_date = latest_release_note_title[/\d{4}-\d{2}-\d{2}/]
    latest_release_anchor = "#release-#{version.gsub(".", "-")}"
    latest_release_url =
      "https://groonga.org/docs/news/#{File.basename(latest_news, ".*")}.html"
    latest_release_announce = <<-ANNOUNCE.gsub(/\n+/, " ").strip
Groonga #{version} (#{latest_release_date}) has been released!
#{latest_release_note_summary}
See: #{latest_release_url}#{latest_release_anchor}
    ANNOUNCE

    x_credentials = {
      api_key: ENV["X_API_KEY"],
      api_key_secret: ENV["X_API_KEY_SECRET"],
      access_token: ENV["X_ACCESS_TOKEN"],
      access_token_secret: ENV["X_ACCESS_TOKEN_SECRET"]
    }
    x_client = X::Client.new(**x_credentials)
    tweet_body = { text: latest_release_announce }
    if dry_run?
      puts tweet_body
    else
      x_client.post("tweets", tweet_body.to_json)
    end
  end

  desc "Release to Arch Linux"
  task :arch_linux do
    releases_url = "https://api.github.com/repos/groonga/groonga/releases"
    latest_released_version = URI(releases_url).open do |input|
      JSON.parse(input.read)[0]["tag_name"].delete_prefix("v")
    end
    sha512_url = "https://github.com/groonga/groonga/releases/download/" +
                 "v#{latest_released_version}/" +
                 "groonga-#{latest_released_version}.tar.gz.sha512"
    sha512 = URI(sha512_url).open do |input|
      input.read.split[0]
    end
    pkgbuild = File.expand_path("ci/arch-linux/PKGBUILD")
    pkgbuild_content = File.read(pkgbuild)
    pkgbuild_content.gsub!(/^pkgver=.*$/) {"pkgver=#{latest_released_version}"}
    pkgbuild_content.gsub!(/^pkgrel=.*$/) {"pkgrel=1"}
    pkgbuild_content.gsub!(/^  "\h{128}"$/) {"  \"#{sha512}\""}
    File.write(pkgbuild, pkgbuild_content)
    sh("git", "add", pkgbuild)
    sh("git", "commit", "-m", "arch-linux: Update to #{latest_released_version}")
    sh("git", "push")

    Dir.mktmpdir do |dir|
      cd(dir) do
        sh("git", "clone", "ssh://aur@aur.archlinux.org/groonga.git")
        cd("groonga") do
          cp(pkgbuild, "./")
          sh("makepkg", "--printsrcinfo", out: ".SRCINFO")
          sh("git", "add", "PKGBUILD", ".SRCINFO")
          sh("git", "commit", "-m", "groonga-#{latest_released_version}-1")
          sh("git", "push")
        end
      end
    end
  end
end

desc "Release"
task release: [
  "release:version:validate",
  "release:version:update",
  "release:tag",
  "dev:version:bump"
]

namespace :nfkc do
  icu_version = ENV["ICU_VERSION"] || ""
  icu_version_hyphen = icu_version.gsub(".", "-")
  icu_version_underscore = icu_version.gsub(".", "_")

  icu4c_src_tgz = File.basename("icu4c-#{icu_version_underscore}-src.tgz")

  file icu4c_src_tgz do
    icu4c_src_tgz_uri = "https://github.com/unicode-org/icu/releases/download/"
    icu4c_src_tgz_uri += "release-#{icu_version_hyphen}/"
    icu4c_src_tgz_uri += icu4c_src_tgz
    icu4c_src_tgz_uri = URI(icu4c_src_tgz_uri)
    icu4c_src_tgz_uri.open do |input|
      File.open(icu4c_src_tgz, "wb") do |output|
        IO.copy_stream(input, output)
      end
    end
  end

  icu_prefix = "tmp/icu-#{icu_version}/local"
  file icu_prefix => icu4c_src_tgz do
    rm_rf("tmp")
    mkdir_p("tmp")
    sh("tar", "xf", icu4c_src_tgz, "-C", "tmp")
    absolute_icu_prefix = File.expand_path(icu_prefix)
    cd("tmp/icu/source") do
      sh("./configure", "--prefix=#{absolute_icu_prefix}")
      sh("make", "-j#{Etc.nprocessors}")
      sh("make", "install")
    end
  end

  desc "Update NFKC to #{icu_version}"
  task update: icu_prefix do
    sh({"ICU_HOME" => File.expand_path(icu_prefix)},
       FileUtils::RUBY,
       "lib/nfkc.rb",
       "--source-directory=lib")
  end
end
