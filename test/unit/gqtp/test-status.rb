# -*- coding: utf-8 -*-
#
# Copyright (C) 2010  Nobuyoshi Nakada <nakada@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

class StatusTest < Test::Unit::TestCase
  include GroongaLocalGQTPTestUtils

  def setup
    setup_local_database
  end

  def teardown
    teardown_local_database
  end

  def test_exit_successfully
    output = run_groonga(@database_path, "status")
    assert_predicate($?, :success?)
  end

  def test_command_version
    output = run_groonga(@database_path, "status", "--command_version", "1")
    rc, result = JSON.parse(output)
    assert_equal(1, result["command_version"])
  end

  def test_unsupported_command_version
    output = run_groonga(@database_path, "status", "--command_version", "10000")
    rc, result = JSON.parse(output)
    assert_equal(Result::UNSUPPORTED_COMMAND_VERSION, rc[0])
  end
end
