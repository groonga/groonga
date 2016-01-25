class TestGrnDBCheck < GroongaTestCase
  def setup
  end

  def test_locked_database
    groonga("lock_acquire")
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
Database is locked. It may be broken. Re-create the database.
    MESSAGE
  end

  def test_locked_table
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("lock_acquire", "Users")
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
    MESSAGE
  end

  def test_locked_data_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")
    groonga("lock_acquire", "Users.age")
    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
    MESSAGE
  end

  def test_locked_index_column
    groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
    groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

    groonga("table_create", "Ages", "TABLE_PAT_KEY", "UInt8")
    groonga("column_create", "Ages", "users_age", "COLUMN_INDEX", "Users", "age")

    groonga("lock_acquire", "Ages.users_age")

    error = assert_raise(CommandRunner::Error) do
      grndb("check")
    end
    assert_equal(<<-MESSAGE, error.error_output)
[Ages.users_age] Index column is locked. It may be broken. Re-create index by '#{grndb_path} recover #{@database_path}'.
    MESSAGE
  end

  sub_test_case "--target" do
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

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
    end

    def test_locked_data_column
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.age")

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Users.age")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
    end

    def test_referenced_table
      groonga("table_create", "Users", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Users", "age", "COLUMN_SCALAR", "UInt8")

      groonga("table_create", "Bookmarks", "TABLE_HASH_KEY", "ShortText")
      groonga("column_create", "Bookmarks", "user", "COLUMN_SCALAR", "Users")

      groonga("lock_acquire", "Users")
      groonga("lock_acquire", "Users.age")
      groonga("lock_acquire", "Bookmarks")
      groonga("lock_acquire", "Bookmarks.user")

      error = assert_raise(CommandRunner::Error) do
        grndb("check", "--target", "Bookmarks.user")
      end
      assert_equal(<<-MESSAGE, error.error_output)
[Bookmarks.user] Data column is locked. It may be broken. (1) Truncate the column (truncate Bookmarks.user) or clear lock of the column (lock_clear Bookmarks.user) and (2) load data again.
[Users] Table is locked. It may be broken. (1) Truncate the table (truncate Users) or clear lock of the table (lock_clear Users) and (2) load data again.
[Users.age] Data column is locked. It may be broken. (1) Truncate the column (truncate Users.age) or clear lock of the column (lock_clear Users.age) and (2) load data again.
      MESSAGE
    end
  end
end
