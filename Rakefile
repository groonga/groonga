# -*- ruby -*-
#
# Copyright(C) 2023  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

require "tmpdir"

def package
  "groonga"
end

def version
  ENV["VERSION"] || File.read("base_version")
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

def env_var(name)
  value = ENV[name]
  raise "${#{name}} is missing" if value.nil?
  value
end

def git_user_name
  `git config --get user.name`.chomp
end

def git_user_email
  `git config --get user.email`.chomp
end

def release_version_update(package,
                           old_version,
                           old_release_date,
                           new_version,
                           new_release_date,
                           *files)
  name = ENV["DEBFULLNAME"] || ENV["NAME"] || git_user_name
  email = ENV["DEBEMAIL"] || ENV["EMAIL"] || git_user_email
  files.each do |file|
    content = replaced_content = File.read(file)
    case file
    when /\.spec(?:\.in)?\z/
      date = Time.parse(new_release_date).strftime("%a %b %d %Y")
      if content !~ /#{Regexp.escape(new_version)}/
        replaced_content = content.sub(/^(%changelog\n)/, <<-ENTRY)
%changelog
* #{date} #{name} <#{email}> - #{new_version}-1
- new upstream release.

        ENTRY
      end
      replaced_content = replaced_content.sub(/^(Release:\s+)\d+/,
                                              "\\11")
    when /debian[^\/]*\/changelog\z/
      date = Time.parse(new_release_date).rfc2822
      if content !~ /#{Regexp.escape(new_version)}/
        replaced_content = content.sub(/\A/, <<-ENTRY)
#{package} (#{new_version}-1) unstable; urgency=low

  * New upstream release.

 -- #{name} <#{email}>  #{date}

        ENTRY
      end
    else
      [[old_version, new_version],
       [old_release_date, new_release_date]].each do |old, new|
        replaced_content = replaced_content.gsub(/#{Regexp.escape(old)}/, new)
        if /\./ =~ old
          old_underscore = old.gsub(/\./, "-")
          new_underscore = new.gsub(/\./, "-")
          replaced_content =
            replaced_content.gsub(/#{Regexp.escape(old_underscore)}/,
                                  new_underscore)
        end
      end
    end
    next if replaced_content == content

    File.open(file, "w") do |output|
      output.print(replaced_content)
    end
  end
end

version = File.read(File.join(__dir__, "base_version"))

namespace :dev do
  namespace :version do
    desc "Bump version for new development"
    task :bump do
      File.write("base_version", env_var("NEW_VERSION"))
    end
  end
end

dist_files = `git ls-files`.split("\n").reject do |file|
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

namespace :release do
  namespace :version do
    desc "Update versions for a new release"
    task :update do
      old_release = env_var("OLD_RELEASE")
      old_release_date = env_var("OLD_RELEASE_DATE")
      new_release_date = env_var("NEW_RELEASE_DATE")
      groonga_org_path = env_var("GROONGA_ORG_DIR")
      release_version_update("groonga",
                             old_release,
                             old_release_date,
                             version,
                             new_release_date,
                             "doc/source/install.rst",
                             *Dir.glob("doc/source/install/*.rst"),
                             *Dir.glob("doc/locale/*/LC_MESSAGES/install/*.po"),
                             File.join(groonga_org_path, "_config.yml"))
      cd("packages") do
        ruby("-S",
             "rake",
             "version:update",
             "RELEASE_DATE=#{new_release_date}")
      end
      sh("git",
         "add",
         "doc/source/install.rst",
         *Dir.glob("doc/source/install/*.rst"),
         *Dir.glob("doc/locale/*/LC_MESSAGES/install/*.po"),
         "packages/debian/changelog",
         "packages/yum/groonga.spec.in")
      sh("git",
         "commit",
         "-m",
         "doc package: update version info to #{version} - #{new_release_date}")
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
end
