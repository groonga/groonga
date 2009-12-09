# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Ryo Onodera <onodera@clear-code.com>
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

class DumpTest < Test::Unit::TestCase
  include GroongaTestUtils

  def setup
    @tmp_dir = File.join(File.dirname(__FILE__), "tmp")
    FileUtils.rm_rf(@tmp_dir)
    FileUtils.mkdir_p(@tmp_dir)
    @database_path = File.join(@tmp_dir, "database")
  end

  def teardown
    FileUtils.rm_rf(@tmp_dir)
  end

  def test_multiple_table_create
    assert_dump("table_create users 0 ShortText\n" +
                "table_create admin_users 0 users\n")
  end

  def test_order_of_table_create
    assert_dump(('a'..'z').to_a.shuffle.collect do |letter|
                  "table_create #{letter} 0 ShortText\n"
                end.join)
  end

  def test_column_create_short_text
    assert_dump("table_create Entry 0 ShortText\n" +
                "column_create Entry body 0 ShortText\n")
  end

  def test_column_create_int32
    assert_dump("table_create Entry 0 ShortText\n" +
                "column_create Entry body 0 Int32\n")
  end

  def test_scaler_column_create
    assert_dump("table_create Entry 0 ShortText\n" +
                "column_create Entry body 0 ShortText\n")
  end

  def test_vector_column_create
    assert_dump("table_create Entry 1 ShortText\n" +
                "column_create Entry body 1 ShortText\n")
  end

  def test_index_column_create
    assert_dump("table_create Entry 0 ShortText\n" +
                "column_create Entry body 0 ShortText\n" +
                "table_create Terms 129 ShortText --default_tokenizer TokenBigram\n" +
                "column_create Terms entry_body 2 Entry body\n")
  end

  def test_table_with_index_column
    body = "作成するテーブルを語彙表として使用する場合、" +
           "文字列を分割するトークナイザを指定します。"
    assert_dump("table_create Entry 0 ShortText\n" +
                "column_create Entry body 0 ShortText\n" +
                "table_create Terms 129 ShortText --default_tokenizer TokenBigram\n" +
                "column_create Terms entry_body 2 Entry body\n" +
                "load --table Entry\n[\n" +
                '{"_id":1,"_key":"gcc","body":"' + body + '"}' + "\n]\n")
  end

  def test_load
    assert_dump("table_create commands 1 ShortText\n" +
                "column_create commands body 0 ShortText\n" +
                "load --table commands\n[\n" +
                '{"_id":1,"_key":"gcc","body":"a compiler"}' + ",\n" +
                '{"_id":2,"_key":"bash","body":"a shell"}' + "\n]\n")
  end

  def test_int32_load
    assert_dump("table_create commands 1 ShortText\n" +
                "column_create commands body 0 Int32\n" +
                "load --table commands\n[\n" +
                '{"_id":1,"_key":"gcc","body":32}' + ",\n" +
                '{"_id":2,"_key":"bash","body":-2715}' + "\n]\n")
  end

  def test_vector_load
    assert_dump("table_create commands 1 ShortText\n" +
                "column_create commands body 1 ShortText\n" +
                "load --table commands\n[\n" +
                '{"_id":1,"_key":"gcc","body":["C","and","C++","Compiler"]}' +
                "\n]\n")
  end

  def test_vector_int32_load
    assert_dump("table_create commands 1 ShortText\n" +
                "column_create commands body 1 Int32\n" +
                "load --table commands\n[\n" +
                '{"_id":1,"_key":"gcc","body":[827,833,991,2716]}' +
                "\n]\n")
  end

  def test_load_with_reference_key
    assert_dump(<<EOGQTP)
table_create users 0 ShortText
table_create comments 1 ShortText
column_create comments text 0 ShortText
column_create comments author 0 users
load --table users
[
{"_id":1,"_key":"ryoqun"},
{"_id":2,"_key":"hayamiz"}
]
load --table comments
[
{"_id":1,"_key":"groonga","text":"it is fast","author":"ryoqun"},
{"_id":2,"_key":"ruby","text":"it is fun","author":"hayamiz"}
]
EOGQTP
  end

  def test_load_with_vector_reference_key
    assert_dump(<<EOGQTP)
table_create users 0 ShortText
table_create comments 1 ShortText
column_create comments text 0 ShortText
column_create comments author 1 users
load --table users
[
{"_id":1,"_key":"ryoqun"},
{"_id":2,"_key":"hayamiz"}
]
load --table comments
[
{"_id":1,"_key":"groonga","text":"it is fast","author":["ryoqun","hayamiz"]}
]
EOGQTP
  end

  private
  def dump
    run_groonga(@database_path, "dump")
  end

  def feed_commands(commands)
    IO.popen(construct_command_line("-n", @database_path), "w+") do |pipe|
      pipe.write(commands)
      pipe.write("shutdown\n")
      pipe.read
    end
  end

  def assert_dump(expected)
    feed_commands(expected)
    assert_equal(expected, dump)
  end
end
