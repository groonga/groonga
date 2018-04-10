module Sandbox
  private
  def setup_sandbox
    setup_tmp_directory
    setup_log_path
    setup_database_path
  end

  def setup_tmp_directory
    base_dir = ENV["BASE_DIR"] || __dir__
    @base_tmp_dir = Pathname(base_dir) + "tmp"
    memory_file_system = "/run/shm"
    if File.exist?(memory_file_system)
      FileUtils.mkdir_p(@base_tmp_dir.parent.to_s)
      FileUtils.rm_f(@base_tmp_dir.to_s)
      FileUtils.ln_s(memory_file_system, @base_tmp_dir.to_s)
    else
      FileUtils.mkdir_p(@base_tmp_dir.to_s)
    end

    @tmp_dir = @base_tmp_dir + "groonga-command-line"
    FileUtils.rm_rf(@tmp_dir.to_s)
    FileUtils.mkdir_p(@tmp_dir.to_s)
  end

  def setup_log_path
    @log_path = @tmp_dir + "groonga.log"
    @query_log_path = @tmp_dir + "groonga-query.log"
    @output_log_path = @tmp_dir + "output.log"
    @error_output_log_path = @tmp_dir + "error-output.log"
  end

  def setup_database_path
    name_for_path = name.gsub(/[\(\)\[\]: ]/, "-")
    @database_path = @tmp_dir + "#{name_for_path}.db"
  end

  def teardown_sandbox
    teardown_tmp_directory
  end

  def teardown_tmp_directory
    FileUtils.rm_rf(@tmp_dir.to_s)
    FileUtils.rm_rf(@base_tmp_dir.to_s)
  end
end
