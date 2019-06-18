class TestGrnDBRecover < GroongaTestCase
  def setup
  end

  def test_normal
    groonga("table_create", "info", "TABLE_NO_KEY")

    FileUtils.rm(@log_path)
    result = grndb("recover", "--log-level", "info")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("info", <<-MESSAGES),
|i| Recovering database: <#{@database_path}>
|i| Recovered database: <#{@database_path}>
                   MESSAGES
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end

  def test_orphan_inspect
    groonga("table_create", "inspect", "TABLE_NO_KEY")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    FileUtils.rm(path)

    FileUtils.rm(@log_path)
    result = grndb("recover", "--log-level", "info")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("info", <<-MESSAGES),
|i| Recovering database: <#{@database_path}>
|e| system call error: DETAIL: failed to open path: <#{path}>
|e| grn_ctx_at: failed to open object: <256>(<inspect>):<51>(<table:no_key>)
|i| [db][recover] removed orphan 'inspect' object: <256>
|i| Recovered database: <#{@database_path}>
                   MESSAGES
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])

    FileUtils.rm(@log_path)
    result = grndb("check")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("notice", ""),
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end

  def test_locked_database
    groonga("lock_acquire")

    FileUtils.rm(@log_path)
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    error_message = <<-MESSAGE
Failed to recover database: <#{@database_path}>
object corrupt: <[db][recover] database may be broken. Please re-create the database>(-55)
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice", <<-MESSAGES),
|e| [db][recover] database may be broken. Please re-create the database
#{prepend_tag("|e| ", error_message).chomp}
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end

  def test_empty_files
    groonga("table_create", "Data", "TABLE_NO_KEY")
    empty_file_path_object = "#{@database_path}.0000100"
    FileUtils.rm(empty_file_path_object)
    FileUtils.touch(empty_file_path_object)
    empty_file_path_object_addtional = "#{empty_file_path_object}.001"
    FileUtils.touch(empty_file_path_object_addtional)
    empty_file_path_no_object = "#{@database_path}.0000210"
    FileUtils.touch(empty_file_path_no_object)

    FileUtils.rm(@log_path)
    result = grndb("recover", "--log-level", "info")
    message = <<-MESSAGE
Removed empty file: <#{empty_file_path_no_object}>
[Data] Remove a broken object that has empty file: <#{empty_file_path_object}>
    MESSAGE
    assert_equal([
                   "",
                   message,
                   expected_groonga_log("info", <<-MESSAGES),
|i| Recovering database: <#{@database_path}>
#{prepend_tag("|i| ", message.lines[0]).chomp}
|i| [io][remove] removed path: <#{empty_file_path_object}>
|i| [io][remove] removed numbered path: <1>: <#{empty_file_path_object_addtional}>
#{prepend_tag("|i| ", message.lines[1]).chomp}
|i| Recovered database: <#{@database_path}>
                   MESSAGES
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])

    FileUtils.rm(@log_path)
    result = grndb("check")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("notice", ""),
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end

  sub_test_case("locked table") do
    def setup
      @suppress_options = {:command_line => ["--log-level", "error"]}
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("lock_acquire", "Users")

      _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
      @table_path = path

      FileUtils.rm(@log_path)
    end

    def test_default
      error = assert_raise(CommandRunner::Error) do
        grndb("recover")
      end
      recover_error_message = <<-MESSAGE.chomp
[db][recover] table may be broken: <Users>: please truncate the table (or clear lock of the table) and load data again
      MESSAGE
      error_message = <<-MESSAGE
Failed to recover database: <#{@database_path}>
object corrupt: <#{recover_error_message}>(-55)
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice", <<-MESSAGES),
|e| #{recover_error_message}
#{prepend_tag("|e| ", error_message).chomp}
                     MESSAGES
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalize_groonga_log(File.read(@log_path)),
                   ])
    end

    def test_force_truncate
      additional_path = "#{@table_path}.002"
      FileUtils.touch(additional_path)
      result = grndb("recover",
                     "--force-truncate",
                     "--log-level", "info")
      message = <<-MESSAGE
[Users] Truncated broken object: <#{@table_path}>
[Users] Removed broken object related file: <#{additional_path}>
      MESSAGE
      assert_equal([
                     "",
                     message,
                     expected_groonga_log("info", <<-MESSAGES),
|i| Recovering database: <#{@database_path}>
|i| [io][remove] removed path: <#{@table_path}>
#{prepend_tag("|i| ", message).chomp}
|i| Recovered database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalize_groonga_log(File.read(@log_path)),
                   ])
    end
  end

  sub_test_case("locked data column") do
    def setup
      @suppress_options = {:command_line => ["--log-level", "error"]}
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")
      groonga("lock_acquire", "Users.age")

      users_column_list = JSON.parse(groonga("column_list", "Users").output)
      _id, _name, path, *_ = users_column_list[1][2]
      @column_path = path

      FileUtils.rm(@log_path)
    end

    def test_default
      error = assert_raise(CommandRunner::Error) do
        grndb("recover")
      end
      recover_error_message = <<-MESSAGE.chomp
[db][recover] column may be broken: <Users.age>: please truncate the column (or clear lock of the column) and load data again
      MESSAGE
      error_message = <<-MESSAGE
Failed to recover database: <#{@database_path}>
object corrupt: <#{recover_error_message}>(-55)
      MESSAGE
      assert_equal([
                     "",
                     error_message,
                     expected_groonga_log("notice", <<-MESSAGES),
|e| #{recover_error_message}
#{prepend_tag("|e| ", error_message).chomp}
                     MESSAGES
                   ],
                   [
                     error.output,
                     error.error_output,
                     normalize_groonga_log(File.read(@log_path)),
                   ])
    end

    def test_force_truncate
      additional_path = "#{@column_path}.002"
      FileUtils.touch(additional_path)
      result = grndb("recover",
                     "--force-truncate",
                     "--log-level", "info")
      message = <<-MESSAGE
[Users.age] Truncated broken object: <#{@column_path}>
[Users.age] Removed broken object related file: <#{additional_path}>
      MESSAGE
      assert_equal([
                     "",
                     message,
                     expected_groonga_log("info", <<-MESSAGES),
|i| Recovering database: <#{@database_path}>
|i| [io][remove] removed path: <#{@column_path}>
#{prepend_tag("|i| ", message).chomp}
|i| Recovered database: <#{@database_path}>
                     MESSAGES
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalize_groonga_log(File.read(@log_path)),
                   ])
    end
  end

  sub_test_case("locked index column") do
    def setup
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

      groonga("table_create", "Ages", "TABLE_PAT_KEY", "UInt8")
      groonga("column_create", "Ages", "users_age",
              "COLUMN_INDEX", "Users", "age")

      values = [{"_key" => "alice", "age" => 29}]
      groonga("load",
              "--table", "Users",
              "--values",
              Shellwords.escape(JSON.generate(values)))
      groonga("truncate", "Ages")
      groonga("lock_acquire", "Ages.users_age")

      FileUtils.rm(@log_path)
    end

    def test_default
      result = grndb("recover")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("notice", ""),
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalize_groonga_log(File.read(@log_path)),
                   ])

      select_result = groonga_select("Users", "--query", "age:29")
      n_hits, _columns, *_records = select_result[0]
      assert_equal([1], n_hits)
    end

    def test_force_truncate
      result = grndb("recover", "--force-truncate")
      assert_equal([
                     "",
                     "",
                     expected_groonga_log("notice", ""),
                   ],
                   [
                     result.output,
                     result.error_output,
                     normalize_groonga_log(File.read(@log_path)),
                   ])

      select_result = groonga_select("Users", "--query", "age:29")
      n_hits, _columns, *_records = select_result[0]
      assert_equal([1], n_hits)
    end
  end

  def test_force_clear_locked_database
    groonga("lock_acquire")

    FileUtils.rm(@log_path)
    result = grndb("recover", "--force-lock-clear", "--log-level", "info")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("info", <<-MESSAGES),
|i| Recovering database: <#{@database_path}>
|i| Clear locked database: <#{@database_path}>
|i| Recovered database: <#{@database_path}>
                   MESSAGES
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end

  def test_force_clear_locked_table
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("lock_acquire", "Users")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]

    FileUtils.rm(@log_path)
    result = grndb("recover", "--force-lock-clear", "--log-level", "info")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("info", <<-MESSAGES),
|i| Recovering database: <#{@database_path}>
|i| [Users] Clear locked object: <#{path}>
|i| Recovered database: <#{@database_path}>
                   MESSAGES
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end

  def test_force_clear_locked_data_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")
    groonga("lock_acquire", "Users.age")
    _id, _name, path, *_ = JSON.parse(groonga("column_list Users").output)[1][2]

    FileUtils.rm(@log_path)
    result = grndb("recover", "--force-lock-clear", "--log-level", "info")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("info", <<-MESSAGES),
|i| Recovering database: <#{@database_path}>
|i| [Users.age] Clear locked object: <#{path}>
|i| Recovered database: <#{@database_path}>
                   MESSAGES
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end

  def test_force_clear_locked_index_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

    groonga("table_create", "Ages", "TABLE_PAT_KEY", "UInt8")
    groonga("column_create", "Ages", "users_age", "COLUMN_INDEX", "Users", "age")
    _id, _name, path, *_ = JSON.parse(groonga("column_list Ages").output)[1][2]

    groonga("load",
            "--table", "Users",
            "--values",
            Shellwords.escape(JSON.generate([{"_key" => "alice", "age" => 29}])))
    groonga("truncate", "Ages")
    groonga("lock_acquire", "Ages.users_age")
    select_result = groonga_select("Users", "--query", "age:29")
    n_hits, _columns, *_records = select_result[0]
    assert_equal([0], n_hits)

    FileUtils.rm(@log_path)
    result = grndb("recover", "--force-lock-clear", "--log-level", "info")
    assert_equal([
                   "",
                   "",
                   expected_groonga_log("info", <<-MESSAGES),
|i| Recovering database: <#{@database_path}>
|i| [io][remove] removed path: <#{path}>
|i| [io][remove] removed path: <#{path}.c>
|i| [ii][builder][fin] removed path: <#{path}XXXXXX>
|i| Recovered database: <#{@database_path}>
                   MESSAGES
                 ],
                 [
                   result.output,
                   result.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])

    select_result = groonga_select("Users", "--query", "age:29")
    n_hits, _columns, *_records = select_result[0]
    assert_equal([1], n_hits)
  end

  def test_broken_id
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    data = File.binread(path)
    data[0] = "X"
    File.binwrite(path, data)

    FileUtils.rm(@log_path)
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    error_message = <<-MESSAGE
Failed to recover database: <#{@database_path}>
incompatible file format: <failed to open: format ID is different: <#{path[0..85]}>(-65)
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice", <<-MESSAGES)
|e| failed to open: format ID is different: <#{path}>: <GROONGA:IO:00001>
|e| grn_ctx_at: failed to open object: <256>(<Users>):<48>(<table:hash_key>)
#{prepend_tag("|e| ", error_message).chomp}
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end

  def test_broken_type_hash
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    data = File.binread(path)
    data[16] = "\0"
    File.binwrite(path, data)

    FileUtils.rm(@log_path)
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    error_message = <<-MESSAGE
Failed to recover database: <#{@database_path}>
invalid format: <[table][hash] file type must be 0x30: <0000>>(-54)
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice", <<-MESSAGES)
|e| [table][hash] file type must be 0x30: <0000>
|e| grn_ctx_at: failed to open object: <256>(<Users>):<48>(<table:hash_key>)
#{prepend_tag("|e| ", error_message).chomp}
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end

  def test_broken_type_array
    groonga("table_create", "Logs", "TABLE_NO_KEY")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    data = File.binread(path)
    data[16] = "\0"
    File.binwrite(path, data)

    FileUtils.rm(@log_path)
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    error_message = <<-MESSAGE
Failed to recover database: <#{@database_path}>
invalid format: <[table][array] file type must be 0x33: <0000>>(-54)
    MESSAGE
    assert_equal([
                   "",
                   error_message,
                   expected_groonga_log("notice", <<-MESSAGES)
|e| [table][array] file type must be 0x33: <0000>
|e| grn_ctx_at: failed to open object: <256>(<Logs>):<51>(<table:no_key>)
#{prepend_tag("|e| ", error_message).chomp}
                   MESSAGES
                 ],
                 [
                   error.output,
                   error.error_output,
                   normalize_groonga_log(File.read(@log_path)),
                 ])
  end
end
