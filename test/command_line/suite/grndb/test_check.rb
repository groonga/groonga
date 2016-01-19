class TestGrnDBCheck < GroongaTestCase
  def setup
  end

  def test_locked_database
    groonga("lock_acquire")
    grndb("check")
  end
end
