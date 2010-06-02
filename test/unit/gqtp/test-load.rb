# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Ryo Onodera <onodera@clear-code.com>
# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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

class LoadTest < Test::Unit::TestCase
  include GroongaLocalGQTPTestUtils

  def setup
    setup_local_database
  end

  def teardown
    teardown_local_database
  end

  def test_table_with_key_with_no_column
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0,0.0,0.0]]
[[0,0.0,0.0],2]
[[0,0.0,0.0],[[[2],[["_id","UInt32"],["_key","ShortText"]],[2,"bash"],[1,"gcc"]]]]
[[0,0.0,0.0]]
EXPECTED
table_create commands 1 ShortText
load --table commands
[
["_key"],
["gcc"],
["bash"]
]
select commands
COMMANDS
  end

  def test_table_with_key_with_one_column
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0,0.0,0.0]]
[[0,0.0,0.0]]
[[0,0.0,0.0],2]
[[0,0.0,0.0],[[[2],[["_id","UInt32"],["_key","ShortText"],["body","ShortText"]],[2,"bash","a shell"],[1,"gcc","a compiler"]]]]
[[0,0.0,0.0]]
EXPECTED
table_create commands 1 ShortText
column_create commands body 0 ShortText
load --table commands
[
["_key","body"],
["gcc","a compiler"],
["bash","a shell"]
]
select commands
COMMANDS
  end

  def test_table_with_key_with_two_columns
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0,0.0,0.0]]
[[0,0.0,0.0]]
[[0,0.0,0.0]]
[[0,0.0,0.0],2]
[[0,0.0,0.0],[[[2],[["_id","UInt32"],["_key","ShortText"],["location","ShortText"],["body","ShortText"]],[2,"bash","/bin/bash","a shell"],[1,"gcc","/usr/bin/gcc","a compiler"]]]]
[[0,0.0,0.0]]
EXPECTED
table_create commands 1 ShortText
column_create commands body 0 ShortText
column_create commands location 0 ShortText
load --table commands
[
["_key","body","location"],
["gcc","a compiler","/usr/bin/gcc"],
["bash","a shell","/bin/bash"]
]
select commands
COMMANDS
  end

  def test_key_at_not_first_position
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0,0.0,0.0]]
[[0,0.0,0.0]]
[[0,0.0,0.0]]
[[0,0.0,0.0],2]
[[0,0.0,0.0],[[[2],[["_id","UInt32"],["_key","ShortText"],["location","ShortText"],["body","ShortText"]],[2,"bash","/bin/bash","a shell"],[1,"gcc","/usr/bin/gcc","a compiler"]]]]
[[0,0.0,0.0]]
EXPECTED
table_create commands 1 ShortText
column_create commands body 0 ShortText
column_create commands location 0 ShortText
load --table commands
[
["body","location","_key"],
["a compiler","/usr/bin/gcc","gcc"],
["a shell","/bin/bash","bash"]
]
select commands
COMMANDS
  end

  def test_table_with_no_key_with_no_column
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0,0.0,0.0]]
[[0,0.0,0.0],2]
[[0,0.0,0.0],[[[2],[["_id","UInt32"]],[1],[2]]]]
[[0,0.0,0.0]]
EXPECTED
table_create commands 3
load --table commands
[
[],
[],
[]
]
select commands
COMMANDS
  end

  def test_table_with_no_key_with_one_column
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0,0.0,0.0]]
[[0,0.0,0.0]]
[[0,0.0,0.0],2]
[[0,0.0,0.0],[[[2],[["_id","UInt32"],["body","ShortText"]],[1,\"a compiler\"],[2,\"a shell\"]]]]
[[0,0.0,0.0]]
EXPECTED
table_create commands 3
column_create commands body 0 ShortText
load --table commands
[
["body"],
["a compiler"],
["a shell"]
]
select commands
COMMANDS
  end

  def test_table_with_no_key_with_two_columns
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0,0.0,0.0]]
[[0,0.0,0.0]]
[[0,0.0,0.0]]
[[0,0.0,0.0],2]
[[0,0.0,0.0],[[[2],[["_id","UInt32"],["location","ShortText"],["body","ShortText"]],[1,"/usr/bin/gcc","a compiler"],[2,"/bin/bash","a shell"]]]]
[[0,0.0,0.0]]
EXPECTED
table_create commands 3
column_create commands body 0 ShortText
column_create commands location 0 ShortText
load --table commands
[
["body","location"],
["a compiler","/usr/bin/gcc"],
["a shell","/bin/bash"]
]
select commands
COMMANDS
  end
end
