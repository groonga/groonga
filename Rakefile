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

namespace :release do
  namespace :version do
    desc "Update versions for a new release"
    task :update do
      old_release = env_var("OLD_RELEASE")
      old_release_date = env_var("OLD_RELEASE_DATE")
      new_release_date = env_var("NEW_RELEASE_DATE")
      groonga_org_path = env_var("GROONGA_ORG_PATH")
      release_version_update("groonga",
                             old_release,
                             old_release_date,
                             version,
                             new_release_date,
                             "doc/source/install.rst",
                             *Dir.glob("doc/source/install/*.rst"),
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
         "packages/debian/changelog",
         "packages/yum/groonga.spec.in")
      sh("git",
         "commit",
         "-m",
         "doc package: update version info to #{version} - #{new_release_date}")
    end
  end

  desc "Tag"
  task :tag do
    sh("git",
       "tag",
       "v#{version}",
       "-a",
       "-m",
       "Groonga #{version}!!!")
    sh("git", "push", "origin", "v#{version}")
  end
end
