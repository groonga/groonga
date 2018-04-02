class TestGrnDBRecover < GroongaTestCase
  def setup
  end

  def test_orphan_inspect
    groonga("table_create", "inspect", "TABLE_NO_KEY")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    FileUtils.rm(path)
    result = grndb("recover")
    assert_equal("", result.error_output)
    result = grndb("check")
    assert_equal("", result.error_output)
  end

  def test_locked_database
    groonga("lock_acquire")
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Failed to recover database: <#{@database_path}>
object corrupt: <[db][recover] database may be broken. Please re-create the database>(-55)
    MESSAGE
  end

  sub_test_case("locked table") do
    def setup
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("lock_acquire", "Users")

      _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
      @table_path = path
    end

    def test_default
      error = assert_raise(CommandRunner::Error) do
        grndb("recover")
      end
      assert_equal(<<-MESSAGE, error.error_output)
Failed to recover database: <#{@database_path}>
object corrupt: <[db][recover] table may be broken: <Users>: please truncate the table (or clear lock of the table) and load data again>(-55)
      MESSAGE
    end

    def test_force_truncate
      result = grndb("recover", "--force-truncate")
      assert_equal(<<-MESSAGE, result.error_output)
[Users] Truncated broken object: <#{@table_path}>
      MESSAGE
    end
  end

  def test_locked_data_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")
    groonga("lock_acquire", "Users.age")
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Failed to recover database: <#{@database_path}>
object corrupt: <[db][recover] column may be broken: <Users.age>: please truncate the column (or clear lock of the column) and load data again>(-55)
    MESSAGE
  end

  def test_locked_index_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

    groonga("table_create", "Ages", "TABLE_PAT_KEY", "UInt8")
    groonga("column_create", "Ages", "users_age", "COLUMN_INDEX", "Users", "age")

    groonga("load",
            "--table", "Users",
            "--values",
            Shellwords.escape(JSON.generate([{"_key" => "alice", "age" => 29}])))
    groonga("truncate", "Ages")
    groonga("lock_acquire", "Ages.users_age")
    select_result = groonga_select("Users", "--query", "age:29")
    n_hits, _columns, *_records = select_result[0]
    assert_equal([0], n_hits)

    result = grndb("recover")
    assert_equal("", result.error_output)

    select_result = groonga_select("Users", "--query", "age:29")
    n_hits, _columns, *_records = select_result[0]
    assert_equal([1], n_hits)
  end

  def test_force_clear_locked_database
    groonga("lock_acquire")
    result = grndb("recover", "--force-lock-clear")
    assert_equal("", result.error_output)
  end

  def test_force_clear_locked_table
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("lock_acquire", "Users")
    result = grndb("recover", "--force-lock-clear")
    assert_equal("", result.error_output)
  end

  def test_force_clear_locked_data_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")
    groonga("lock_acquire", "Users.age")
    result = grndb("recover", "--force-lock-clear")
    assert_equal("", result.error_output)
  end

  def test_force_clear_locked_index_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

    groonga("table_create", "Ages", "TABLE_PAT_KEY", "UInt8")
    groonga("column_create", "Ages", "users_age", "COLUMN_INDEX", "Users", "age")

    groonga("load",
            "--table", "Users",
            "--values",
            Shellwords.escape(JSON.generate([{"_key" => "alice", "age" => 29}])))
    groonga("truncate", "Ages")
    groonga("lock_acquire", "Ages.users_age")
    select_result = groonga_select("Users", "--query", "age:29")
    n_hits, _columns, *_records = select_result[0]
    assert_equal([0], n_hits)

    result = grndb("recover", "--force-lock-clear")
    assert_equal("", result.error_output)

    select_result = groonga_select("Users", "--query", "age:29")
    n_hits, _columns, *_records = select_result[0]
    assert_equal([1], n_hits)
  end

  def test_empty_file
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    FileUtils.rm(path)
    FileUtils.touch(path)
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Failed to recover database: <#{@database_path}>
incompatible file format: <[io][open] file size is too small: <0>(required: >= 64): <#{path[0..68]}>(-65)
    MESSAGE
  end

  def test_broken_id
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    data = File.binread(path)
    data[0] = "X"
    File.binwrite(path, data)
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Failed to recover database: <#{@database_path}>
incompatible file format: <failed to open: format ID is different: <#{path[0..85]}>(-65)
    MESSAGE
  end

  def test_broken_type_hash
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    data = File.binread(path)
    data[16] = "\0"
    File.binwrite(path, data)
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Failed to recover database: <#{@database_path}>
invalid format: <[table][hash] file type must be 0x30: <0000>>(-54)
    MESSAGE
  end

  def test_broken_type_array
    groonga("table_create", "Logs", "TABLE_NO_KEY")
    _id, _name, path, *_ = JSON.parse(groonga("table_list").output)[1][1]
    data = File.binread(path)
    data[16] = "\0"
    File.binwrite(path, data)
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Failed to recover database: <#{@database_path}>
invalid format: <[table][array] file type must be 0x33: <0000>>(-54)
    MESSAGE
  end
end
