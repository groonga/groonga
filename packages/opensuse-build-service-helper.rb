# Copyright(C) 2023 Kentaro Hayashi <hayashi@clear-code.com>
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

require "pathname"

module OpenSUSEBuildServiceHelper

  def define_obsbuild_tasks
    ensure_osc_configuration
    define_apt_obsbuild_task
    define_yum_obsbuild_task
  end

  private

  def obs_project_name
    # e.g. isv:milter-manager
    ENV["OBS_PROJECT"] ? ENV["OBS_PROJECT"] : "isv:#{@package}"
  end

  def obs_package_dir(package_name)
    File.join(obs_project_name, package_name)
  end

  def oscrc_path
    Pathname.new("~/.config/osc/oscrc").expand_path
  end

  def obs_user
    obs_user = nil
    if oscrc_path.exist?
      oscrc_content = oscrc_path.read
    else
      oscrc_content = ""
    end
    oscrc_content.each_line do |line|
      if line.start_with?("user=")
        obs_user = line.split('=', 2).last.chomp
      end
    end
    obs_user ? obs_user : ENV["USER"]
  end

  def srpm_sources_dir
    Pathname("#{Dir.pwd}/home/#{ENV['USER']}/rpmbuild/SOURCES")
  end

  def srpm_spec_dir
    Pathname("#{Dir.pwd}/home/#{ENV['USER']}/rpmbuild/SPECS")
  end

  def ensure_osc_configuration
    unless oscrc_path.exist?
      raise "can't detect #{oscrc_path}"
    end
  end

  def apply_obs_pkg_metadata(project_name, package_name)
    cd(project_name) do
      require 'cgi'
      require 'tempfile'
      Tempfile.create("meta.xml") do |meta_xml|
        meta_xml.puts(<<-XML)
<package name="#{CGI.escape_html(package_name)}" project="#{CGI.escape_html(project_name)}">
  <title>milter manager #{CGI.escape_html(detect_version)}</title>
  <description/>
</package>
        XML
        meta_xml.close
        sh("osc", "meta", "pkg", package_name, "--file", meta_xml.path)
      end
      sh("osc", "update")
    end
  end

  def copy_obs_source_files(repositories_dir, package_name, pattern)
    files = []
    repositories_dir.glob(pattern) do |path|
      if path.basename.to_s.start_with?("#{@package}_#{@version}") or
        path.basename.to_s.start_with?("#{@package}-#{@version}")
        # e.g.: "debian/pool/bullseye/main/m/milter-manager/milter-manager*.dsc"
        # e.g.: "centos/7/SRPMS/milter-manager*.src.rpm"
        files << path.basename.to_s
        cp(path.to_s, obs_package_dir(package_name))
      elsif path.basename.to_s.end_with?(".spec") and
           path.basename.to_s.start_with?("#{@package}")
        # allow .spec without version suffix
        files << path.basename.to_s
        cp(path.to_s, obs_package_dir(package_name))
      end
    end
    files
  end

  def define_apt_obsbuild_task
    namespace :apt do
      desc "Push downloaded source packages to openSUSE Build Service"
      task :obsbuild => ["apt:download"] do
        ensure_osc_configuration
        sh("osc", "checkout", obs_project_name) unless Dir.exist?(obs_project_name)
        apply_obs_pkg_metadata(obs_project_name, @archive_base_name)
        files = copy_obs_source_files(Pathname("#{apt_dir}/repositories"), @archive_base_name, "**/*.{dsc,gz,xz}")
        cd(obs_package_dir(@archive_base_name)) do
          sh("osc", "status")
          files.each do |file|
            sh("osc", "add", file)
          end
          sh("osc", "commit", "--message", "Add #{@version}")
        end
      end
    end
    task :apt => ["apt:obsbuild"]
  end

  def define_yum_obsbuild_task
    namespace :yum do
      desc "Push downloaded RPM packages to openSUSE Build Service"
      task :obsbuild => ["yum:download"] do
        sh("osc", "checkout", obs_project_name) unless Dir.exist?(obs_project_name)
        apply_obs_pkg_metadata(obs_project_name, @archive_base_name)
        files = copy_obs_source_files(Pathname("#{yum_dir}/repositories"), @archive_base_name, "**/*.src.rpm")
        unless Dir.exist?(obs_package_dir(@archive_base_name))
          cd(obs_project_name) do
            files.each do |file|
              sh("osc", "importsrcpkg", "--project", obs_project_name,
                 "--name", @archive_base_name,
                 "--title", "milter manager #{@version}",
                 File.join(@archive_base_name, file))
            end
          end
        else
          # When already package meta data is committed, importsrcpkg will fail,
          # so explicitly add each source files.
          files.each do |file|
            sh("rpm", "-ivh", "--root", Dir.pwd, File.join(obs_package_dir(@archive_base_name), file))
          end
          files = [copy_obs_source_files(srpm_sources_dir, @archive_base_name, "*.{gz,patch}"),
                   copy_obs_source_files(srpm_spec_dir, @archive_base_name, "*.{spec}")].flatten
          cd(obs_package_dir(@archive_base_name)) do
            files.each do |file|
              sh("osc", "add", file)
            end
            sh("osc", "commit", "--message", "Add #{@version}")
          end
        end
      end
    end
    task :yum => ["yum:obsbuild"]
  end

end
