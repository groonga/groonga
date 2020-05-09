class TestGrnDBCheck < GroongaTestCase
  def setup
  end

  sub_test_case "Groonga log" do
    def test_failed_to_open
      groonga("status")
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

  def test_normal
    groonga("table_create", "Data", "TABLE_NO_KEY")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]

    remove_groonga_log
    result = grndb("check", "--log-level", "info")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("info", <<-MESSAGES),
|i| Checking database: <#{@database_path}>
|i| Database doesn't have orphan 'inspect' object: <#{@database_path}>
|i| Database is not locked: <#{@database_path}>
|i| Database is not corrupted: <#{@database_path}>
|i| Database is not dirty: <#{@database_path}>
#{windows? ? "|i| [io][open] open existing file: <#{path}>" : ""}
|i| [Data] Table is not locked
|i| [Data] Table is not corrupted
|i| Checked database: <#{@database_path}>
                   MESSAGES
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  def test_orphan_inspect
    groonga("table_create", "inspect", "TABLE_NO_KEY")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    FileUtils.rm(path)

    remove_groonga_log
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    error_message = <<-MESSAGE
Database has orphan 'inspect' object. Remove it by '#{real_grndb_path} recover #{@database_path}'.
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice", <<-MESSAGES),
|e| system call error: DETAIL: [io][open] failed to open path: <#{@database_path}.0000100>
|e| grn_ctx_at: failed to open object: <256>(<inspect>):<51>(<table:no_key>)
|e| #{error_message.chomp}
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  def test_locked_database
    groonga("lock_acquire")

    remove_groonga_log
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    error_message = <<-MESSAGE
Database is locked. It may be broken. Re-create the database.
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice",
                                        prepend_tag("|e| ", error_message)),
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  sub_test_case "dirty database" do
    def test_only_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga do |process|
        process.run_command(<<-COMMAND)
load --table Users
[
{"_key": "Alice"}
]
        COMMAND
        Process.kill(:KILL, process.pid)
      end

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check")
      end
      error_message = <<-MESSAGE
Database wasn't closed successfully. It may be broken. Re-create the database.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_have_plugin
      groonga("plugin_register", "sharding")
      groonga("io_flush")

      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga do |process|
        process.run_command(<<-COMMAND)
load --table Users
[
{"_key": "Alice"}
]
        COMMAND
        Process.kill(:KILL, process.pid)
      end

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check")
      end
      error_message = <<-MESSAGE
Database wasn't closed successfully. It may be broken. Re-create the database.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
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

    remove_groonga_log
    result = grndb("check")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("notice", ""),
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  def test_nonexistent_table
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    FileUtils.rm(path)

    remove_groonga_log
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    error_message = <<-MESSAGE
[Users] Can't open object. It's broken. Re-create the object or the database.
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice", <<-MESSAGES),
|e| system call error: DETAIL: [io][open] failed to open path: <#{@database_path}.0000100>
|e| grn_ctx_at: failed to open object: <256>(<Users>):<48>(<table:hash_key>)
|e| #{error_message.chomp}
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  def test_locked_table
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("lock_acquire", "Users")

    remove_groonga_log
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    error_message = <<-MESSAGE
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice",
                                        prepend_tag("|e| ", error_message)),
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  def test_locked_data_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")
    groonga("lock_acquire", "Users.age")

    remove_groonga_log
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    error_message = <<-MESSAGE
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice",
                                        prepend_tag("|e| ", error_message)),
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  sub_test_case "locked index column" do
    def test_locked_segment
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

      groonga("table_create", "Ages", "TABLE_PAT_KEY", "UInt8")
      groonga("column_create", "Ages", "users_age",
              "COLUMN_INDEX", "Users", "age")

      groonga("lock_acquire", "Ages.users_age")

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check")
      end
      error_message = <<-MESSAGE
[Ages.users_age] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end
  end

  def test_corrupt_table
    use_large_tmp_dir
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga do |external_process|
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

    remove_groonga_log
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    error_message = <<-MESSAGE
[Users] Table is corrupt. (1) Truncate the table (truncate Users or '#{real_grndb_path} recover --force-truncate #{@database_path}') and (2) load data again.
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice", <<-MESSAGES),
|e| system call error: DETAIL: [io][corrupt] used path doesn't exist: <#{removed_path}>
|e| #{error_message.chomp}
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  def test_corrupt_double_array_table
    groonga("table_create", "Users", "TABLE_DAT_KEY", "ShortText")
    groonga do |external_process|
      external_process.input.puts("load --table Users")
      external_process.input.puts("[")
      external_process.input.puts("{\"_key\": \"x\"}")
      external_process.input.puts("]")
    end
    removed_path = "#{@database_path}.0000100.001"
    FileUtils.rm(removed_path)

    remove_groonga_log
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    error_message = <<-MESSAGE
[Users] Table is corrupt. (1) Truncate the table (truncate Users or '#{real_grndb_path} recover --force-truncate #{@database_path}') and (2) load data again.
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice", <<-MESSAGES),
|e| system call error: DETAIL: [dat][corrupt] used path doesn't exist: <#{removed_path}>
|e| #{error_message.chomp}
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  def test_corrupt_data_column
    use_large_tmp_dir
    groonga("table_create", "Data", "TABLE_NO_KEY")
    groonga("column_create", "Data", "text", "COLUMN_SCALAR", "Text")
    groonga do |external_process|
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

    remove_groonga_log
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    error_message = <<-MESSAGE
[Data.text] Data column is corrupt. (1) Truncate the column (truncate Data.text or '#{real_grndb_path} recover --force-truncate #{@database_path}') and (2) load data again.
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice", <<-MESSAGES),
|e| system call error: DETAIL: [io][corrupt] used path doesn't exist: <#{removed_path}>
|e| #{error_message.chomp}
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  def test_empty_files
    groonga("table_create", "Data", "TABLE_NO_KEY")
    empty_file_path_object = "#{@database_path}.0000100"
    FileUtils.rm(empty_file_path_object)
    FileUtils.touch(empty_file_path_object)
    empty_file_path_no_object = "#{@database_path}.0000210"
    FileUtils.touch(empty_file_path_no_object)

    remove_groonga_log
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal([
                   "",
                   <<-MESSAGES,
Empty file exists: <#{empty_file_path_object}>
Empty file exists: <#{empty_file_path_no_object}>
[Data] Can't open object. It's broken. Re-create the object or the database.
                   MESSAGES
                   expected_groonga_log("notice", <<-MESSAGES),
|e| Empty file exists: <#{empty_file_path_object}>
|e| Empty file exists: <#{empty_file_path_no_object}>
|e| [io][open] file size is too small: <0>(required: >= 64): <#{empty_file_path_object}>
|e| grn_ctx_at: failed to open object: <256>(<Data>):<51>(<table:no_key>)
|e| [Data] Can't open object. It's broken. Re-create the object or the database.
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalized_groonga_log_content,
                 ])
  end

  sub_test_case "--target" do
    def test_nonexistent_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
      FileUtils.rm(path)

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users")
      end
      error_message = <<-MESSAGE
[Users] Can't open object. It's broken. Re-create the object or the database.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice", <<-MESSAGES),
|e| system call error: DETAIL: [io][open] failed to open path: <#{path}>
|e| grn_ctx_at: failed to open object: <256>(<Users>):<48>(<table:hash_key>)
|e| #{error_message.chomp}
                     MESSAGES
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_locked_table
      groonga("table_create", "Bookmarks", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Bookmarks", "title",
              "COLUMN_SCALAR", "ShortText")

      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

      groonga("lock_acquire", "Bookmarks")
      groonga("lock_acquire", "Bookmarks.title")
      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.age")

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users")
      end
      error_message = <<-MESSAGE
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_locked_data_column
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.age")

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users.age")
      end
      error_message = <<-MESSAGE
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_nonexistent_referenced_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

      groonga("table_create", "Bookmarks", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Bookmarks", "user", "COLUMN_SCALAR", "Users")

      removed_path = nil
      JSON.parse(groonga("table_list").output)[1].each do |table|
        _id, name, path, *_ = table
        if name == "Users"
          removed_path = path
          FileUtils.rm(path)
          break
        end
      end

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Bookmarks")
      end
      error_message = <<-MESSAGE
[Users] Can't open object. It's broken. Re-create the object or the database.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice", <<-MESSAGES)
|e| system call error: DETAIL: [io][open] failed to open path: <#{removed_path}>
|e| grn_ctx_at: failed to open object: <256>(<Users>):<48>(<table:hash_key>)
|e| #{error_message.chomp}
                     MESSAGES
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_referenced_table_by_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

      groonga("table_create", "Admins", "TABLE_HASH_KEY", "Users")

      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.age")
      groonga("lock_acquire", "Admins")

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Admins")
      end
      error_message = <<-MESSAGE
[Admins] Table is locked. It may be broken. (1) Truncate the table (truncate Admins) or clear lock of the table (lock_clear Admins) and (2) load data again.
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_referenced_table_by_column
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

      groonga("table_create", "Bookmarks", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Bookmarks", "user", "COLUMN_SCALAR", "Users")

      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.age")
      groonga("lock_acquire", "Bookmarks")
      groonga("lock_acquire", "Bookmarks.user")

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Bookmarks.user")
      end
      error_message = <<-MESSAGE
[Bookmarks.user] Data column is locked. It may be broken. (1) Truncate the column (truncate Bookmarks.user) or clear lock of the column (lock_clear Bookmarks.user) and (2) load data again.
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_locked_index_column
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")
      groonga("column_create", "Users", "name", "COLUMN_SCALAR", "ShortText")

      groonga("table_create", "Ages", "TABLE_PAT_KEY", "UInt8")
      groonga("column_create", "Ages", "users_age", "COLUMN_INDEX",
              "Users", "age")

      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.age")
      groonga("lock_acquire", "Users.name")
      groonga("lock_acquire", "Ages")
      groonga("lock_acquire", "Ages.users_age")

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Ages")
      end
      error_message = <<-MESSAGE
[Ages] Table is locked. It may be broken. (1) Truncate the table (truncate Ages) or clear lock of the table (lock_clear Ages) and (2) load data again.
[Ages.users_age] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_indexed_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")
      groonga("column_create", "Users", "name", "COLUMN_SCALAR", "ShortText")

      groonga("table_create", "Names", "TABLE_PAT_KEY", "ShortText")
      groonga("column_create", "Names", "users_names",
              "COLUMN_INDEX|WITH_SECTION", "Users", "_key,name")

      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.age")
      groonga("lock_acquire", "Users.name")
      groonga("lock_acquire", "Names")
      groonga("lock_acquire", "Names.users_names")

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Names")
      end
      error_message = <<-MESSAGE
[Names] Table is locked. It may be broken. (1) Truncate the table (truncate Names) or clear lock of the table (lock_clear Names) and (2) load data again.
[Names.users_names] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.name] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.name) or clear lock of the column (lock_clear Users.name) and (2) load data again.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_indexed_data_column
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")
      groonga("column_create", "Users", "name", "COLUMN_SCALAR", "ShortText")

      groonga("table_create", "Names", "TABLE_PAT_KEY", "ShortText")
      groonga("column_create", "Names", "users_name",
              "COLUMN_INDEX", "Users", "name")

      groonga("table_create", "NormalizedNames", "TABLE_PAT_KEY", "ShortText",
              "--normalizer", "NormalizerAuto")
      groonga("column_create", "NormalizedNames", "users_name",
              "COLUMN_INDEX", "Users", "name")

      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.age")
      groonga("lock_acquire", "Users.name")
      groonga("lock_acquire", "Names")
      groonga("lock_acquire", "Names.users_name")
      groonga("lock_acquire", "NormalizedNames")
      groonga("lock_acquire", "NormalizedNames.users_name")

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users.name")
      end
      error_message = <<-MESSAGE
[Users.name] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.name) or clear lock of the column (lock_clear Users.name) and (2) load data again.
[NormalizedNames.users_name] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[Names.users_name] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[NormalizedNames] Table is locked. It may be broken. (1) Truncate the table (truncate NormalizedNames) or clear lock of the table (lock_clear NormalizedNames) and (2) load data again.
[Names] Table is locked. It may be broken. (1) Truncate the table (truncate Names) or clear lock of the table (lock_clear Names) and (2) load data again.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_cycle_reference
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")

      groonga("table_create", "Logs", "TABLE_PAT_KEY", "ShortText")
      groonga("column_create", "Logs", "user", "COLUMN_SCALAR", "Users")

      groonga("column_create", "Users", "logs_user", "COLUMN_INDEX",
              "Logs", "user")

      groonga("lock_acquire", "Logs")
      groonga("lock_acquire", "Logs.user")
      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.logs_user")

      remove_groonga_log
      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users")
      end
      error_message = <<-MESSAGE
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.logs_user] Index column is locked. It may be broken. Re-create index by '#{real_grndb_path} recover #{@database_path}'.
[Logs] Table is locked. It may be broken. (1) Truncate the table (truncate Logs) or clear lock of the table (lock_clear Logs) and (2) load data again.
[Logs.user] Data column is locked. It may be broken. (1) Truncate the column (truncate Logs.user) or clear lock of the column (lock_clear Logs.user) and (2) load data again.
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice",
                                          prepend_tag("|e| ", error_message)),
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalized_groonga_log_content,
                   ])
    end
  end

  sub_test_case "--since" do
    def setup
      super
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      @seconds_per_day = 60 * 60 * 24
      touch_database(Time.now - (8 * @seconds_per_day))
      _id, _name, @table_path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    end

    def touch_database(time)
      Dir.glob("#{@database_path}*") do |path|
        FileUtils.touch(path, mtime: time)
      end
    end

    def adjust_start_time
      while (Time.now.sec % 10) >= 5
        sleep(1)
      end
    end

    def compute_since(relative_seconds)
      Time.now + relative_seconds
    end

    def test_database_keys
      remove_groonga_log
      FileUtils.touch(@database_path,
                      mtime: Time.now - (6 * @seconds_per_day))
      adjust_start_time
      since = compute_since(-7 * @seconds_per_day)
      result = grndb("check",
                     "--log-level", "info",
                     "--since=-7d")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("info", <<-MESSAGES),
|i| Checking database: <#{@database_path}>: <#{format_since(since)}>
|i| Database doesn't have orphan 'inspect' object: <#{@database_path}>
|i| Database is not locked: <#{@database_path}>
|i| Database is not corrupted: <#{@database_path}>
|i| Database is not dirty: <#{@database_path}>
|i| Checked database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_database_specs
      remove_groonga_log
      FileUtils.touch("#{@database_path}.001",
                      mtime: Time.now - (6 * @seconds_per_day))
      adjust_start_time
      since = compute_since(-7 * @seconds_per_day)
      result = grndb("check",
                     "--log-level", "info",
                     "--since=-7d")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("info", <<-MESSAGES),
|i| Checking database: <#{@database_path}>: <#{format_since(since)}>
|i| Database doesn't have orphan 'inspect' object: <#{@database_path}>
|i| Database is not locked: <#{@database_path}>
|i| Database is not corrupted: <#{@database_path}>
|i| Database is not dirty: <#{@database_path}>
|i| Checked database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_database_config
      remove_groonga_log
      FileUtils.touch("#{@database_path}.conf",
                      mtime: Time.now - (6 * @seconds_per_day))
      adjust_start_time
      since = compute_since(-7 * @seconds_per_day)
      result = grndb("check",
                     "--log-level", "info",
                     "--since=-7d")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("info", <<-MESSAGES),
|i| Checking database: <#{@database_path}>: <#{format_since(since)}>
|i| Database doesn't have orphan 'inspect' object: <#{@database_path}>
|i| Database is not locked: <#{@database_path}>
|i| Database is not corrupted: <#{@database_path}>
|i| Database is not dirty: <#{@database_path}>
|i| Checked database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_database_options
      remove_groonga_log
      FileUtils.touch("#{@database_path}.options",
                      mtime: Time.now - (6 * @seconds_per_day))
      adjust_start_time
      since = compute_since(-7 * @seconds_per_day)
      result = grndb("check",
                     "--log-level", "info",
                     "--since=-1w")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("info", <<-MESSAGES),
|i| Checking database: <#{@database_path}>: <#{format_since(since)}>
|i| Database doesn't have orphan 'inspect' object: <#{@database_path}>
|i| Database is not locked: <#{@database_path}>
|i| Database is not corrupted: <#{@database_path}>
|i| Database is not dirty: <#{@database_path}>
|i| Checked database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_object
      remove_groonga_log
      FileUtils.touch(@table_path,
                      mtime: Time.now - (6 * @seconds_per_day))
      adjust_start_time
      since = compute_since(-7 * @seconds_per_day)
      result = grndb("check",
                     "--log-level", "info",
                     "--since=-1weeks")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("info", <<-MESSAGES),
|i| Checking database: <#{@database_path}>: <#{format_since(since)}>
#{windows? ? "|i| [io][open] open existing file: <#{@table_path}>" : ""}
|i| [Users] Table is not locked
|i| [Users] Table is not corrupted
|i| Checked database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    def test_target
      remove_groonga_log
      adjust_start_time
      since = compute_since(-7 * @seconds_per_day)
      result = grndb("check",
                     "--log-level", "info",
                     "--target", "Users",
                     "--since=-1weeks")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("info", <<-MESSAGES),
|i| Checking database: <#{@database_path}>: <#{format_since(since)}>
|i| Checking object: <Users>
|i| Checked object: <Users>
|i| Checked database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    data("s"       => ["-30s",       -30])
    data("sec"     => ["-30sec",     -30])
    data("secs"    => ["-30secs",    -30])
    data("second"  => ["-30second",  -30])
    data("seconds" => ["-30seconds", -30])
    data("m"       => ["-30m",       -30 * 60])
    data("min"     => ["-30min",     -30 * 60])
    data("mins"    => ["-30mins",    -30 * 60])
    data("minute"  => ["-30minute",  -30 * 60])
    data("minutes" => ["-30minutes", -30 * 60])
    data("h"       => ["-30h"  ,     -30 * 60 * 60])
    data("hour"    => ["-30h",       -30 * 60 * 60])
    data("hours"   => ["-30h",       -30 * 60 * 60])
    data("d"       => ["-3d",        -3 * 60 * 60 * 24])
    data("day"     => ["-3day",      -3 * 60 * 60 * 24])
    data("days"    => ["-3days",     -3 * 60 * 60 * 24])
    data("w"       => ["-0.3w",      -0.3 * 60 * 60 * 24 * 7])
    data("week"    => ["-0.3week",   -0.3 * 60 * 60 * 24 * 7])
    data("weeks"   => ["-0.3weeks",  -0.3 * 60 * 60 * 24 * 7])
    data("month"   => ["-0.1month",  -0.1 * 60 * 60 * 24 * 30])
    data("months"  => ["-0.1months", -0.1 * 60 * 60 * 24 * 30])
    data("y"       => ["-0.01y",     -0.01 * 60 * 60 * 24 * 365])
    data("year"    => ["-0.01year",  -0.01 * 60 * 60 * 24 * 365])
    data("years"   => ["-0.01years", -0.01 * 60 * 60 * 24 * 365])
    def test_relative_time(data)
      argument, relative_time = data
      remove_groonga_log
      adjust_start_time
      since = compute_since(relative_time)
      result = grndb("check",
                     "--log-level", "info",
                     "--since=#{argument}")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("info", <<-MESSAGES),
|i| Checking database: <#{@database_path}>: <#{format_since(since)}>
|i| Checked database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalized_groonga_log_content,
                   ])
    end

    now = Time.now
    data("YYYY" => [
           "%04d" % [now.year],
           Time.local(now.year, 1, 1, 0, 0, 0, 0),
         ])
    data("YYYY-MM" => [
           "%04d-%02d" % [now.year, now.month],
           Time.local(now.year, now.month, 1, 0, 0, 0, 0),
         ])
    data("YYYY-MM-DD" => [
           "%04d-%02d-%02d" % [now.year, now.month, now.day],
           Time.local(now.year, now.month, now.day, 0, 0, 0, 0),
         ])
    data("YYYY-MM-DDThh" => [
           "%04d-%02d-%02dT02" % [now.year, now.month, now.day],
           Time.local(now.year, now.month, now.day, 2, 0, 0, 0),
         ])
    data("YYYY-MM-DDThh:mm" => [
           "%04d-%02d-%02dT02:09" % [now.year, now.month, now.day],
           Time.local(now.year, now.month, now.day, 2, 9, 0, 0),
         ])
    data("YYYY-MM-DDThh:mm:ss" => [
           "%04d-%02d-%02dT02:09:30" % [now.year, now.month, now.day],
           Time.local(now.year, now.month, now.day, 2, 9, 30, 0),
         ])
    data("YYYY-MM-DDThh:mm:ss.uuuuuu" => [
           "%04d-%02d-%02dT02:09:30.292929" % [now.year, now.month, now.day],
           Time.local(now.year, now.month, now.day, 2, 9, 30, 292929),
         ])
    def test_absolute_time(data)
      argument, since = data
      touch_database(since - 1)
      remove_groonga_log
      adjust_start_time
      result = grndb("check",
                     "--log-level", "info",
                     "--since=#{argument}")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("info", <<-MESSAGES),
|i| Checking database: <#{@database_path}>: <#{format_since(since)}>
|i| Checked database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalized_groonga_log_content,
                   ])
    end
  end
end
