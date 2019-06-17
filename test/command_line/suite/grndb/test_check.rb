class TestGrnDBCheck < GroongaTestCase
  def setup
    @suppress_options = {:command_line => ["--log-level", "error"]}
  end

  sub_test_case "Groonga log" do
    def test_failed_to_open
      groonga("status", @suppress_options)
      error = assert_raise(CommandRunner::Error) do
        grndb("check",
              "--groonga-log-path", "/nonexistent1",
              "--groonga-log-path", "/nonexistent2")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[/nonexistent1] Can't open Groonga log path: RuntimeError: open /nonexistent1
[/nonexistent2] Can't open Groonga log path: RuntimeError: open /nonexistent2
      MESSAGE
    end

    def test_normal
      omit("This feature isn't implemented yet.")
      groonga("status")
      log_file = Tempfile.new(["grndb-check-log-path", ".log"])
      log_file.puts(<<-GROONGA_LOG)
2017-11-13 15:58:27.712199|n| grn_init: <7.0.8-14-g16082c4>
      GROONGA_LOG
      log_file.close
      result = grndb("check",
                     "--groonga-log-path", log_file.path)
      assert_equal(<<-MESSAGE, result.output)
{:timestamp=>Mon Nov 13 15:58:27 2017, :log_level=>:notice, :pid=>nil, :message=>"grn_init: <7.0.8-14-g16082c4>"}
      MESSAGE
    end
  end

  def test_normal_info_log
    groonga("table_create", "info", "TABLE_NO_KEY", @suppress_options)
    grndb("check", "--log-level", "info")
    assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|i| Some processings with vm.overcommit_memory != 1 may break DB under low memory condition.
1970-01-01 00:00:00.000000|i| To set vm.overcommit_memory to 1
1970-01-01 00:00:00.000000|i| add 'vm.overcommit_memory = 1' to /etc/sysctl.conf and restart your system or
1970-01-01 00:00:00.000000|i| run 'sudo /sbin/sysctl vm.overcommit_memory=1' command.
1970-01-01 00:00:00.000000|i| Checking database: <#{@database_path}>
1970-01-01 00:00:00.000000|i| Database doesn't have orphan 'inspect' object in <#{@database_path}>
1970-01-01 00:00:00.000000|i| Database is not locked: <#{@database_path}>
1970-01-01 00:00:00.000000|i| Database is not corrupted: <#{@database_path}>
1970-01-01 00:00:00.000000|i| Database is not dirty: <#{@database_path}>
1970-01-01 00:00:00.000000|i| [info] Table is not locked
1970-01-01 00:00:00.000000|i| [info] Table is not corrupted
1970-01-01 00:00:00.000000|i| Checked database: <#{@database_path}>
1970-01-01 00:00:00.000000|n| grn_fin (0)
    MESSAGE
  end

  def test_orphan_inspect
    groonga("table_create", "inspect", "TABLE_NO_KEY", @suppress_options)
    _id, _name, path, *_ = JSON.parse(groonga("table_list", @suppress_options).output)[1][1]
    FileUtils.rm(path)
    error = assert_raise(CommandRunner::Error) do
      grndb("check", "--log-level", "info")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Database has orphan 'inspect' object. Remove it by '#{real_grndb_path} recover #{@database_path}'.
    MESSAGE
  end

  def test_locked_database
    groonga("lock_acquire", @suppress_options)
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Database is locked. It may be broken. Re-create the database.
    MESSAGE
    assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| Database is locked. It may be broken. Re-create the database.
1970-01-01 00:00:00.000000|n| grn_fin (0)
    MESSAGE
  end

  sub_test_case "dirty database" do
    def test_only_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga(@suppress_options) do |process|
        process.run_command(<<-COMMAND)
load --table Users
[
{"_key": "Alice"}
]
        COMMAND
        Process.kill(:KILL, process.pid)
      end
      error = assert_raise(CommandRunner::Error) do
        grndb("check")
      end
      assert_equal(<<-MESSAGE, error.error_output)
Database wasn't closed successfully. It may be broken. Re-create the database.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| Database wasn't closed successfully. It may be broken. Re-create the database.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_have_plugin
      groonga("plugin_register", "sharding", @suppress_options)
      groonga("io_flush", @suppress_options)

      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga(@suppress_options) do |process|
        process.run_command(<<-COMMAND)
load --table Users
[
{"_key": "Alice"}
]
        COMMAND
        Process.kill(:KILL, process.pid)
      end
      error = assert_raise(CommandRunner::Error) do
        grndb("check")
      end
      assert_equal(<<-MESSAGE, error.error_output)
Database wasn't closed successfully. It may be broken. Re-create the database.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| Database wasn't closed successfully. It may be broken. Re-create the database.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end
  end

  def test_cleaned_database
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga do |process|
      process.run_command(<<-COMMAND)
load --table Users
[
{"_key": "Alice"}
]
      COMMAND
      process.run_command("io_flush Users")
      Process.kill(:KILL, process.pid)
    end
    result = grndb("check")
    assert_equal(["", ""],
                 [result.output, result.error_output])
  end

  def test_nonexistent_table
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
    _id, _name, path, *_ = JSON.parse(groonga("table_list", @suppress_options).output)[1][1]
    FileUtils.rm(path)
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
[Users] Can't open object. It's broken. Re-create the object or the database.
    MESSAGE
    assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| system call error: No such file or directory: failed to open path: <#{path}>
1970-01-01 00:00:00.000000|e| grn_ctx_at: failed to open object: <256>(<Users>):<48>(<table:hash_key>)
1970-01-01 00:00:00.000000|e| [Users] Can't open object. It's broken. Re-create the object or the database.
1970-01-01 00:00:00.000000|e| grn_ctx_at: failed to open object: <256>(<Users>):<48>(<table:hash_key>)
1970-01-01 00:00:00.000000|n| (1 same messages are truncated)
1970-01-01 00:00:00.000000|n| grn_fin (0)
    MESSAGE
  end

  def test_locked_table
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
    groonga("lock_acquire", "Users", @suppress_options)
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
    MESSAGE
    assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
    MESSAGE
  end

  def test_locked_data_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)
    groonga("lock_acquire", "Users.age", @suppress_options)
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
    MESSAGE
    assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
    MESSAGE
  end

  sub_test_case "locked index column" do
    def test_locked_segment
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)

      groonga("table_create", "Ages", "TABLE_PAT_KEY", "UInt8", @suppress_options)
      groonga("column_create", "Ages", "users_age",
              "COLUMN_INDEX", "Users", "age", @suppress_options)

      groonga("lock_acquire", "Ages.users_age", @suppress_options)

      error = assert_raise(CommandRunner::Error) do
        grndb("check")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Ages.users_age] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Ages.users_age] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end
  end

  def test_corrupt_table
    use_large_tmp_dir
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
    groonga(@suppress_options) do |external_process|
      external_process.input.puts("load --table Users")
      external_process.input.puts("[")
      300000.times do |i|
        key = (("%04d" % i) * 1024)[0, 4096]
        external_process.input.puts("{\"_key\": \"#{key}\"},")
        external_process.input.flush
      end
      external_process.input.puts("{\"_key\": \"x\"}")
      external_process.input.puts("]")
    end
    removed_path = "#{@database_path}.0000100.001"
    FileUtils.rm(removed_path)
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
[Users] Table is corrupt. (1) Truncate the table (truncate Users or '#{real_grndb_path} recover --force-truncate #{@database_path}') and (2) load data again.
    MESSAGE
    assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| system call error: No such file or directory: [io][corrupt] used path doesn't exist: <#{removed_path}>
1970-01-01 00:00:00.000000|e| [Users] Table is corrupt. (1) Truncate the table (truncate Users or '#{real_grndb_path} recover --force-truncate #{@database_path}') and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
    MESSAGE
  end

  def test_corrupt_double_array_table
    groonga("table_create", "Users", "TABLE_DAT_KEY", "ShortText", @suppress_options)
    groonga(@suppress_options) do |external_process|
      external_process.input.puts("load --table Users")
      external_process.input.puts("[")
      external_process.input.puts("{\"_key\": \"x\"}")
      external_process.input.puts("]")
    end
    removed_path = "#{@database_path}.0000100.001"
    FileUtils.rm(removed_path)
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
[Users] Table is corrupt. (1) Truncate the table (truncate Users or '#{real_grndb_path} recover --force-truncate #{@database_path}') and (2) load data again.
    MESSAGE
    assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| system call error: No such file or directory: [dat][corrupt] used path doesn't exist: <#{removed_path}>
1970-01-01 00:00:00.000000|e| [Users] Table is corrupt. (1) Truncate the table (truncate Users or '#{real_grndb_path} recover --force-truncate #{@database_path}') and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
    MESSAGE
  end

  def test_corrupt_data_column
    use_large_tmp_dir
    groonga("table_create", "Data", "TABLE_NO_KEY", @suppress_options)
    groonga("column_create", "Data", "text", "COLUMN_SCALAR", "Text", @suppress_options)
    groonga(@suppress_options) do |external_process|
      external_process.input.puts("load --table Data")
      external_process.input.puts("[")
      data = "a" * 10000000
      100.times do |i|
        external_process.input.puts("{\"text\": \"#{data}\"},")
        external_process.input.flush
      end
      external_process.input.puts("{\"text\": \"x\"}")
      external_process.input.puts("]")
    end
    removed_path = "#{@database_path}.0000101.001"
    FileUtils.rm(removed_path)
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
[Data.text] Data column is corrupt. (1) Truncate the column (truncate Data.text or '#{real_grndb_path} recover --force-truncate #{@database_path}') and (2) load data again.
    MESSAGE
    assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| system call error: No such file or directory: [io][corrupt] used path doesn't exist: <#{removed_path}>
1970-01-01 00:00:00.000000|e| [Data.text] Data column is corrupt. (1) Truncate the column (truncate Data.text or '#{real_grndb_path} recover --force-truncate #{@database_path}') and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
    MESSAGE
  end

  def test_empty_files
    groonga("table_create", "Data", "TABLE_NO_KEY")
    empty_file_path_object = "#{@database_path}.0000100"
    FileUtils.rm(empty_file_path_object)
    FileUtils.touch(empty_file_path_object)
    empty_file_path_no_object = "#{@database_path}.0000210"
    FileUtils.touch(empty_file_path_no_object)
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Empty file exists: <#{empty_file_path_object}>
Empty file exists: <#{empty_file_path_no_object}>
[Data] Can't open object. It's broken. Re-create the object or the database.
    MESSAGE
  end

  sub_test_case "--target" do
    def test_nonexistent_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      _id, _name, path, *_ = JSON.parse(groonga("table_list", @suppress_options).output)[1][1]
      FileUtils.rm(path)
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Users] Can't open object. It's broken. Re-create the object or the database.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| system call error: No such file or directory: failed to open path: <#{path}>
1970-01-01 00:00:00.000000|e| grn_ctx_at: failed to open object: <256>(<Users>):<48>(<table:hash_key>)
1970-01-01 00:00:00.000000|e| [Users] Can't open object. It's broken. Re-create the object or the database.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_locked_table
      groonga("table_create", "Bookmarks", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Bookmarks", "title",
              "COLUMN_SCALAR", "ShortText", @suppress_options)

      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)

      groonga("lock_acquire", "Bookmarks", @suppress_options)
      groonga("lock_acquire", "Bookmarks.title", @suppress_options)
      groonga("lock_acquire", "Users", @suppress_options)
      groonga("lock_acquire", "Users.age", @suppress_options)

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_locked_data_column
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)

      groonga("lock_acquire", "Users", @suppress_options)
      groonga("lock_acquire", "Users.age", @suppress_options)

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users.age")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_nonexistent_referenced_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)

      groonga("table_create", "Bookmarks", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Bookmarks", "user", "COLUMN_SCALAR", "Users", @suppress_options)

      JSON.parse(groonga("table_list", @suppress_options).output)[1].each do |table|
        _id, name, path, *_ = table
        FileUtils.rm(path) if name == "Users"
      end
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Bookmarks")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Users] Can't open object. It's broken. Re-create the object or the database.
      MESSAGE
      removed_path = "#{@database_path}.0000100"
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| system call error: No such file or directory: failed to open path: <#{removed_path}>
1970-01-01 00:00:00.000000|e| grn_ctx_at: failed to open object: <256>(<Users>):<48>(<table:hash_key>)
1970-01-01 00:00:00.000000|e| [Users] Can't open object. It's broken. Re-create the object or the database.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_referenced_table_by_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)

      groonga("table_create", "Admins", "TABLE_HASH_KEY", "Users", @suppress_options)

      groonga("lock_acquire", "Users", @suppress_options)
      groonga("lock_acquire", "Users.age", @suppress_options)
      groonga("lock_acquire", "Admins", @suppress_options)

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Admins")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Admins] Table is locked. It may be broken. (1) Truncate the table (truncate Admins) or clear lock of the table (lock_clear Admins) and (2) load data again.
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Admins] Table is locked. It may be broken. (1) Truncate the table (truncate Admins) or clear lock of the table (lock_clear Admins) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_referenced_table_by_column
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)

      groonga("table_create", "Bookmarks", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Bookmarks", "user", "COLUMN_SCALAR", "Users", @suppress_options)

      groonga("lock_acquire", "Users", @suppress_options)
      groonga("lock_acquire", "Users.age", @suppress_options)
      groonga("lock_acquire", "Bookmarks", @suppress_options)
      groonga("lock_acquire", "Bookmarks.user", @suppress_options)

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Bookmarks.user")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Bookmarks.user] Data column is locked. It may be broken. (1) Truncate the column (truncate Bookmarks.user) or clear lock of the column (lock_clear Bookmarks.user) and (2) load data again.
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Bookmarks.user] Data column is locked. It may be broken. (1) Truncate the column (truncate Bookmarks.user) or clear lock of the column (lock_clear Bookmarks.user) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_locked_index_column
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)
      groonga("column_create", "Users", "name", "COLUMN_SCALAR", "ShortText", @suppress_options)

      groonga("table_create", "Ages", "TABLE_PAT_KEY", "UInt8", @suppress_options)
      groonga("column_create", "Ages", "users_age", "COLUMN_INDEX",
              "Users", "age", @suppress_options)

      groonga("lock_acquire", "Users", @suppress_options)
      groonga("lock_acquire", "Users.age", @suppress_options)
      groonga("lock_acquire", "Users.name", @suppress_options)
      groonga("lock_acquire", "Ages", @suppress_options)
      groonga("lock_acquire", "Ages.users_age", @suppress_options)

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Ages")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Ages] Table is locked. It may be broken. (1) Truncate the table (truncate Ages) or clear lock of the table (lock_clear Ages) and (2) load data again.
[Ages.users_age] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Ages] Table is locked. It may be broken. (1) Truncate the table (truncate Ages) or clear lock of the table (lock_clear Ages) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Ages.users_age] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
1970-01-01 00:00:00.000000|e| [Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_indexed_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)
      groonga("column_create", "Users", "name", "COLUMN_SCALAR", "ShortText", @suppress_options)

      groonga("table_create", "Names", "TABLE_PAT_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Names", "users_names",
              "COLUMN_INDEX|WITH_SECTION", "Users", "_key,name", @suppress_options)

      groonga("lock_acquire", "Users", @suppress_options)
      groonga("lock_acquire", "Users.age", @suppress_options)
      groonga("lock_acquire", "Users.name", @suppress_options)
      groonga("lock_acquire", "Names", @suppress_options)
      groonga("lock_acquire", "Names.users_names", @suppress_options)

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Names")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Names] Table is locked. It may be broken. (1) Truncate the table (truncate Names) or clear lock of the table (lock_clear Names) and (2) load data again.
[Names.users_names] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.name] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.name) or clear lock of the column (lock_clear Users.name) and (2) load data again.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Names] Table is locked. It may be broken. (1) Truncate the table (truncate Names) or clear lock of the table (lock_clear Names) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Names.users_names] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
1970-01-01 00:00:00.000000|e| [Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Users.name] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.name) or clear lock of the column (lock_clear Users.name) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_indexed_data_column
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8", @suppress_options)
      groonga("column_create", "Users", "name", "COLUMN_SCALAR", "ShortText", @suppress_options)

      groonga("table_create", "Names", "TABLE_PAT_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Names", "users_name",
              "COLUMN_INDEX", "Users", "name", @suppress_options)

      groonga("table_create", "NormalizedNames", "TABLE_PAT_KEY", "ShortText",
              "--normalizer", "NormalizerAuto", @suppress_options)
      groonga("column_create", "NormalizedNames", "users_name",
              "COLUMN_INDEX", "Users", "name", @suppress_options)

      groonga("lock_acquire", "Users", @suppress_options)
      groonga("lock_acquire", "Users.age", @suppress_options)
      groonga("lock_acquire", "Users.name", @suppress_options)
      groonga("lock_acquire", "Names", @suppress_options)
      groonga("lock_acquire", "Names.users_name", @suppress_options)
      groonga("lock_acquire", "NormalizedNames", @suppress_options)
      groonga("lock_acquire", "NormalizedNames.users_name", @suppress_options)

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users.name")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Users.name] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.name) or clear lock of the column (lock_clear Users.name) and (2) load data again.
[NormalizedNames.users_name] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[Names.users_name] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[NormalizedNames] Table is locked. It may be broken. (1) Truncate the table (truncate NormalizedNames) or clear lock of the table (lock_clear NormalizedNames) and (2) load data again.
[Names] Table is locked. It may be broken. (1) Truncate the table (truncate Names) or clear lock of the table (lock_clear Names) and (2) load data again.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Users.name] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.name) or clear lock of the column (lock_clear Users.name) and (2) load data again.
1970-01-01 00:00:00.000000|e| [NormalizedNames.users_name] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
1970-01-01 00:00:00.000000|e| [Names.users_name] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
1970-01-01 00:00:00.000000|e| [NormalizedNames] Table is locked. It may be broken. (1) Truncate the table (truncate NormalizedNames) or clear lock of the table (lock_clear NormalizedNames) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Names] Table is locked. It may be broken. (1) Truncate the table (truncate Names) or clear lock of the table (lock_clear Names) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end

    def test_cycle_reference
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText", @suppress_options)

      groonga("table_create", "Logs", "TABLE_PAT_KEY", "ShortText", @suppress_options)
      groonga("column_create", "Logs", "user", "COLUMN_SCALAR", "Users", @suppress_options)

      groonga("column_create", "Users", "logs_user", "COLUMN_INDEX",
              "Logs", "user", @suppress_options)

      groonga("lock_acquire", "Logs", @suppress_options)
      groonga("lock_acquire", "Logs.user", @suppress_options)
      groonga("lock_acquire", "Users", @suppress_options)
      groonga("lock_acquire", "Users.logs_user", @suppress_options)

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.logs_user] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[Logs] Table is locked. It may be broken. (1) Truncate the table (truncate Logs) or clear lock of the table (lock_clear Logs) and (2) load data again.
[Logs.user] Data column is locked. It may be broken. (1) Truncate the column (truncate Logs.user) or clear lock of the column (lock_clear Logs.user) and (2) load data again.
      MESSAGE
      assert_equal(<<-MESSAGE, normalize_groonga_log(File.read(@log_path)))
1970-01-01 00:00:00.000000|n| grn_init: <VERSION>
1970-01-01 00:00:00.000000|n| vm.overcommit_memory kernel parameter should be 1: <0>: See INFO level log to resolve this
1970-01-01 00:00:00.000000|e| [Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Users.logs_user] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
1970-01-01 00:00:00.000000|e| [Logs] Table is locked. It may be broken. (1) Truncate the table (truncate Logs) or clear lock of the table (lock_clear Logs) and (2) load data again.
1970-01-01 00:00:00.000000|e| [Logs.user] Data column is locked. It may be broken. (1) Truncate the column (truncate Logs.user) or clear lock of the column (lock_clear Logs.user) and (2) load data again.
1970-01-01 00:00:00.000000|n| grn_fin (0)
      MESSAGE
    end
  end
end
