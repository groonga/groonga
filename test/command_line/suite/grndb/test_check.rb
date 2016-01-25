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
end
