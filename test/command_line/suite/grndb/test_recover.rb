class TestGrnDBRecover < GroongaTestCase
  def setup
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

  def test_locked_table
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("lock_acquire", "Users")
    error = assert_raise(CommandRunner::Error) do
      grndb("recover")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Failed to recover database: <#{@database_path}>
object corrupt: <[db][recover] table may be broken: <Users>: please truncate the table (or clear lock of the table) and load data again>(-55)
    MESSAGE
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

    groonga("lock_acquire", "Ages.users_age")

    result = grndb("recover")
    assert_equal("", result.error_output)
  end
end
