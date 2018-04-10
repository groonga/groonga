module Sandbox
  private
  def setup_sandbox
    setup_tmp_directory
    setup_log_path
    setup_error_logger

    setup_encoding
    setup_context

    setup_database
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

    @tmp_dir = @base_tmp_dir + "groonga-mruby"
    FileUtils.rm_rf(@tmp_dir.to_s)
    FileUtils.mkdir_p(@tmp_dir.to_s)
  end

  def setup_log_path
    @dump_log = false

    @log_path = @tmp_dir + "groonga.log"
    logger = Groonga::FileLogger.new(@log_path.to_s)
    Groonga::Logger.register(logger)

    @query_log_path = @tmp_dir + "groonga-query.log"
    query_logger = Groonga::FileQueryLogger.new(@query_log_path.to_s)
    Groonga::QueryLogger.register(query_logger, :all => true)
  end

  def setup_encoding
    Groonga::Encoding.default = nil
  end

  def setup_context
    Groonga::Context.default = nil
    Groonga::Context.default_options = nil
  end

  def context
    Groonga::Context.default
  end

  def encoding
    Groonga::Encoding.default
  end

  def setup_error_logger
    Groonga::Logger.register(:max_level => :error) do |*args|
      event, level, time, title, message, location = args
      if event == :log
        puts("#{time}:#{level[0]}:#{location}:#{message}")
      end
    end
  end

  def setup_logger
    Groonga::Logger.register(:max_level => :dump) do |*args|
      p args
    end
  end

  def setup_database
    name_for_path = name.gsub(/[\(\)\[\] ]/, "-")
    @database_path = @tmp_dir + "#{name_for_path}.db"
    @database = Groonga::Database.create(:path => @database_path.to_s)
  end

  def teardown_database
    return if @database.nil?

    @database.close
    @database = nil
  end

  def teardown_sandbox
    teardown_database
    Groonga::Context.default.close
    Groonga::Context.default = nil
    teardown_log_path
    teardown_tmp_directory
  end

  def teardown_log_path
    return unless @dump_log
    log_path = Groonga::Logger.log_path
    if File.exist?(log_path)
      header = "--- log: #{log_path} ---"
      puts(header)
      puts(File.read(log_path))
      puts("-" * header.length)
    end
    if @query_log_path.exist?
      header = "--- query log: #{@query_log_path} ---"
      puts(header)
      puts(@query_log_path.read)
      puts("-" * header.length)
    end
  end

  def teardown_tmp_directory
    FileUtils.rm_rf(@tmp_dir.to_s)
    FileUtils.rm_rf(@base_tmp_dir.to_s)
  end
end
