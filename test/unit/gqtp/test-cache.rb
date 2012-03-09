# -*- coding: utf-8 -*-
#
# Copyright (C) 2010  Ryo Onodera <onodera@clear-code.com>
# Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

class CacheTest < Test::Unit::TestCase
  include GroongaLocalGQTPTestUtils

  def setup
    setup_local_database
  end

  def teardown
    teardown_local_database
  end

  def test_cache_with_illegal_select
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0,0.0,0.0],true]
[[0,0.0,0.0],true]
[[0,0.0,0.0],true]
[[0,0.0,0.0],true]
[[0,0.0,0.0],1]
[[0,0.0,0.0],true]
EXPECTED
table_create --name Site --flags TABLE_HASH_KEY --key_type ShortText
column_create --table Site --name title --flags COLUMN_SCALAR --type ShortText
table_create --name Terms --flags TABLE_PAT_KEY|KEY_NORMALIZE --key_type ShortText --default_tokenizer TokenBigram
column_create --table Terms --name blog_title --flags COLUMN_INDEX|WITH_POSITION --type Site --source title
load --table Site
[
 {"_key":"http://example.org/","title":"This is test record 1!"}
]
COMMANDS

    expected= <<EXPECTED
[[-63,0.0,0.0,"Syntax error! (<)",[["yy_syntax_error","ecmascript.y",19]]],[]]
[[-63,0.0,0.0,"Syntax error! (<)",[["yy_syntax_error","ecmascript.y",19]]],[]]
[[0,0.0,0.0],true]
EXPECTED

    commands = <<COMMANDS
select --table Site --filter "<"
COMMANDS

    output = nil
    IO.popen(construct_command_line(@database_path), "w+") do |pipe|
      sleep 1
      pipe.write(commands)
      sleep 1
      pipe.write(commands)
      pipe.write("shutdown\n")
      output = pipe.read
    end
    assert_error_command_output(expected, output)
  end

  private
  def assert_error_command_output(expected, actual)
    actual = actual.gsub(/^\[\[(-63|0),[\d\.e\-]+,[\d\.e\-]+/) do
      "[[#{$1},0.0,0.0"
    end
    assert_equal(expected, actual)
  end
end
