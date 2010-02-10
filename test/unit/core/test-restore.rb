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

class RestoreTest < Test::Unit::TestCase
  include GroongaLocalGQTPTestUtils

  def setup
    setup_local_database
  end

  def teardown
    teardown_local_database
  end

  def test_multiple_table_create
    assert_same_dump(<<-EOC)
table_create users 0 ShortText
table_create admin_users 0 users
EOC
  end

  def test_order_of_table_create
    assert_same_dump(('a'..'z').to_a.shuffle.collect do |letter|
                       "table_create #{letter} 0 ShortText\n"
                     end.join)
  end

  def test_column_create_short_text
    assert_same_dump(<<-EOC)
table_create Entry 0 ShortText
column_create Entry body 0 ShortText
EOC
  end

  def test_column_create_int32
    assert_same_dump(<<-EOC)
table_create Entry 0 ShortText
column_create Entry body 0 Int32
EOC
  end

  def test_scaler_column_create
    assert_same_dump(<<-EOC)
table_create Entry 0 ShortText
column_create Entry body 0 ShortText
EOC
  end

  def test_vector_column_create
    assert_same_dump(<<-EOC)
table_create Entry 1 ShortText
column_create Entry body 1 ShortText
EOC
  end

  def test_index_column_create
    assert_same_dump(<<-EOC)
table_create Entry 0 ShortText
column_create Entry body 0 ShortText
table_create Terms 129 ShortText --default_tokenizer TokenBigram
column_create Terms entry_body 2 Entry body
EOC
  end

  def test_table_with_index_column
    body = "作成するテーブルを語彙表として使用する場合、" +
           "文字列を分割するトークナイザを指定します。"
    assert_same_dump(<<-EOC)
table_create Entry 0 ShortText
column_create Entry body 0 ShortText
table_create Terms 129 ShortText --default_tokenizer TokenBigram
column_create Terms entry_body 2 Entry body
load --table Entry
[
["_key","body"],
["gcc","#{body}"]
]
EOC
  end

  def test_table_with_index_column_sorted_by_id
    body = "作成するテーブルを語彙表として使用する場合、" +
           "文字列を分割するトークナイザを指定します。"
    commands = <<-EOC
table_create Terms 129 ShortText --default_tokenizer TokenBigram
table_create Entry 0 ShortText
column_create Entry body 0 ShortText
column_create Terms entry_body 2 Entry body
load --table Entry
[
{"_key":"gcc","body":"#{body}"}
]
EOC

    assert_dump(<<-EOD, commands)
table_create Terms 129 ShortText --default_tokenizer TokenBigram
table_create Entry 0 ShortText
column_create Entry body 0 ShortText
column_create Terms entry_body 2 Entry body
load --table Entry
[
["_key","body"],
["gcc","#{body}"]
]
EOD
  end

  def test_table_with_multiple_index_column
    title = "default_tokenizer"
    body = "作成するテーブルを語彙表として使用する場合、" +
           "文字列を分割するトークナイザを指定します。"
    assert_same_dump(<<-EOC)
table_create Entry 0 ShortText
column_create Entry title 0 ShortText
column_create Entry body 0 ShortText
table_create Terms 129 ShortText --default_tokenizer TokenBigram
column_create Terms entry_body 2 Entry title,body
load --table Entry
[
["_key","title","body"],
["gcc","#{title}","#{body}"]
]
EOC
  end

  def test_load
    assert_same_dump(<<-EOC)
table_create commands 1 ShortText
column_create commands body 0 ShortText
load --table commands
[
["_key","body"],
["gcc","a compiler"],
["bash","a shell"]
]
EOC
  end

  def test_load_to_value_pseudo_column_of_hash_table
    assert_same_dump(<<-EOC)
table_create users 0 ShortText Int32
load --table users
[
["_key","_value"],
["ryoqun",1000],
["hayamiz",1001]
]
EOC
  end

  def test_load_to_value_pseudo_column_of_patricia_table
    assert_same_dump(<<-EOC)
table_create users 1 ShortText Int32
load --table users
[
["_key","_value"],
["ryoqun",1000],
["hayamiz",1001]
]
EOC
  end

  def test_load_to_value_pseudo_column_of_array_table
    assert_same_dump(<<-EOC)
table_create users 3 --value_type Int32
load --table users
[
["_id","_value"],
[1,1000],
[2,1001]
]
EOC
  end

  def test_load_reference_key_to_value_pseudo_column
    assert_same_dump(<<-EOC)
table_create groups 0 ShortText
table_create users 0 ShortText groups
load --table groups
[
["_key"],
["admin"],
["end_user"]
]
load --table users
[
["_key","_value"],
["ryoqun","admin"],
["hayamiz","end_user"]
]
EOC
  end

  def test_load_reference_id_to_value_pseudo_column
    assert_same_dump(<<-EOC)
table_create groups 3
column_create groups name 0 ShortText
table_create users 0 ShortText groups
load --table groups
[
["_id","name"],
[1,"admin"],
[2,"end_user"]
]
load --table users
[
["_key","_value"],
["ryoqun",1],
["hayamiz",2]
]
EOC
  end

  def test_load_to_array
    assert_same_dump(<<-EOC)
table_create commands 3
column_create commands body 0 ShortText
load --table commands
[
["_id","body"],
[1,"a compiler"],
[2,"a shell"]
]
EOC
  end

  def test_int32_load
    assert_same_dump(<<-EOC)
table_create commands 1 ShortText
column_create commands body 0 Int32
load --table commands
[
["_key","body"],
["gcc",32],
["bash",-2715]
]
EOC
  end

  def test_vector_empty_load
    assert_same_dump(<<-EOC)
table_create commands 1 ShortText
column_create commands body 1 ShortText
load --table commands
[
["_key","body"],
["gcc",[]]
]
EOC
  end

  def test_vector_string_load
    assert_same_dump(<<-EOC)
table_create commands 1 ShortText
column_create commands body 1 ShortText
load --table commands
[
["_key","body"],
["gcc",["C","and","C++","Compiler"]]
]
EOC
  end

  def test_vector_int32_load
    assert_same_dump(<<-EOC)
table_create commands 1 ShortText
column_create commands body 1 Int32
load --table commands
[
["_key","body"],
["gcc",[827,833,991,2716]]
]
EOC
  end

  def test_load_with_test_reference_key
    assert_same_dump(<<EOGQTP)
table_create users 0 ShortText
table_create comments 1 ShortText
column_create comments text 0 ShortText
column_create comments author 0 users
load --table users
[
["_key"],
["ryoqun"],
["hayamiz"]
]
load --table comments
[
["_key","text","author"],
["groonga","it is fast","ryoqun"],
["ruby","it is fun","hayamiz"]
]
EOGQTP
  end

  def test_load_with_vector_text_reference_key
    assert_same_dump(<<EOGQTP)
table_create users 0 ShortText
table_create comments 1 ShortText
column_create comments text 0 ShortText
column_create comments author 1 users
load --table users
[
["_key"],
["ryoqun"],
["hayamiz"]
]
load --table comments
[
["_key","text","author"],
["groonga","it is fast",["ryoqun","hayamiz"]]
]
EOGQTP
  end

  def test_load_with_int32_reference_key
    assert_same_dump(<<EOGQTP)
table_create users 0 Int32
column_create users name 0 ShortText
table_create comments 1 ShortText
column_create comments text 0 ShortText
column_create comments author 0 users
load --table users
[
["_key","name"],
[1000,"ryoqun"],
[1001,"hayamiz"]
]
load --table comments
[
["_key","text","author"],
["groonga","it is fast",1000],
["ruby","it is fun",1001]
]
EOGQTP
  end

  def test_load_with_reference_id
    assert_same_dump(<<EOGQTP)
table_create users 3
column_create users name 0 ShortText
table_create comments 1 ShortText
column_create comments text 0 ShortText
column_create comments author 0 users
load --table users
[
["_id","name"],
[1,"ryoqun"],
[2,"hayamiz"]
]
load --table comments
[
["_key","text","author"],
["groonga","it is fast",1],
["ruby","it is fun",2]
]
EOGQTP
  end

  def test_load_with_vector_int32_reference_key
    assert_same_dump(<<EOGQTP)
table_create users 0 Int32
column_create users name 0 ShortText
table_create comments 1 ShortText
column_create comments text 0 ShortText
column_create comments author 1 users
load --table users
[
["_key","name"],
[1000,"ryoqun"],
[1001,"hayamiz"]
]
load --table comments
[
["_key","text","author"],
["groonga","it is fast",[1000,1001]]
]
EOGQTP
  end

  def test_load_with_vector_reference_id
    assert_same_dump(<<EOGQTP)
table_create users 3
column_create users name 0 ShortText
table_create comments 1 ShortText
column_create comments text 0 ShortText
column_create comments author 1 users
load --table users
[
["_id","name"],
[1,"ryoqun"],
[2,"hayamiz"]
]
load --table comments
[
["_key","text","author"],
["groonga","it is fast",[1,2]]
]
EOGQTP
  end

  def test_load_chained_subtables
    assert_same_dump(<<EOGQTP)
table_create words 0 ShortText
table_create japanese 0 words
table_create noun 0 japanese
load --table words
[
["_key"],
["file"],
["ファイル"],
["寝る"]
]
load --table japanese
[
["_key"],
["ファイル"],
["寝る"]
]
load --table noun
[
["_key"],
["ファイル"]
]
EOGQTP
  end

  def test_delete_by_key
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0]]
[[0],3]
true
[[0],[[2],["_id","_key"],[1,"hayamiz"],[3,"mori"]]]
[[0]]
EXPECTED
table_create users 0 ShortText
load --table users
[
{"_key":"hayamiz"},
{"_key":"ryoqun"},
{"_key":"mori"}
]
delete users --key ryoqun
select users
COMMANDS
  end

  def test_delete_by_id
    assert_commands(<<EXPECTED, <<COMMANDS)
[[0]]
[[0]]
[[0],3]
true
[[0],[[2],["_id","name"],[1,"hayamiz"],[3,"mori"]]]
[[0]]
EXPECTED
table_create users 3
column_create users name 0 ShortText
load --table users
[
{"name":"hayamiz"},
{"name":"ryoqun"},
{"name":"mori"}
]
delete users --id 2
select users
COMMANDS
  end

  def test_load_unsequential_array_table
    assert_same_dump(<<COMMANDS)
table_create users 3
column_create users name 0 ShortText
table_create blog_entries 3
column_create blog_entries body 0 ShortText
column_create blog_entries author 0 users
load --table users
[
["_id","name"],
[1,"hayamiz"],
[],
[],
[],
[5,"mori"],
[],
[],
[8,"ryoqun"]
]
delete --table users --id 2
delete --table users --id 3
delete --table users --id 4
delete --table users --id 6
delete --table users --id 7
load --table blog_entries
[
["_id","body","author"],
[1,"Today was very chilly.",1],
[2,"Taiyaki is very yummy.",8],
[3,"I was programming.",5]
]
COMMANDS

    result = feed_commands(<<COMMANDS)
select blog_entries --output_columns author.name
COMMANDS

    assert_equal('[[0],[[3],["author.name"],["hayamiz"],["ryoqun"],["mori"]]]' + "\n" +
                 "[[0]]\n",
                 result)
  end

  def test_view
    assert_same_dump(<<COMMANDS)
table_create View 4
table_create FreePrograms 1 ShortText
table_create NonFreePrograms 1 ShortText
view_add View FreePrograms
view_add View NonFreePrograms
load --table FreePrograms
[
["_key"],
["gnash"],
["poppler"]
]
load --table NonFreePrograms
[
["_key"],
["Windows"],
["Nvidia Video Driver"]
]
COMMANDS
  end
end
