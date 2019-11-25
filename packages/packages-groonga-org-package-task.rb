require_relative "../vendor/apache-arrow-source/dev/tasks/linux-packages/package-task"

class PackagesGroongaOrgPackageTask < PackageTask
  def define
    super
    define_release_tasks
  end

  private
  def release(target_namespace)
    base_dir = __send__("#{target_namespace}_dir")
    repositories_dir = "#{base_dir}/repositories"
    sh("sudo", "-H",
       "chown",
       "-R",
       "#{Process.uid}:#{Process.gid}",
       repositories_dir)
    rm_rf(repositories_dir)
    Rake::Task["#{target_namespace}:build"].invoke
    sh("rsync",
       "-av",
       "#{repositories_dir}/",
       "packages@packages.groonga.org:public/")
  end

  def define_release_tasks
    [:apt, :yum].each do |target_namespace|
      namespace target_namespace do
        task :release do
          release(target_namespace) if __send__("enable_#{target_namespace}?")
        end
      end
    end
  end
end
