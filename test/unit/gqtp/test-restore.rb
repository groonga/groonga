# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Ryo Onodera <onodera@clear-code.com>
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
table_create users TABLE_HASH_KEY ShortText
table_create admin_users TABLE_HASH_KEY users
EOC
  end

  def test_order_of_table_create
    assert_same_dump(('a'..'z').to_a.shuffle.collect do |letter|
                       "table_create #{letter} TABLE_HASH_KEY ShortText\n"
                     end.join)
  end

  def test_column_create_short_text
    assert_same_dump(<<-EOC)
table_create Entry TABLE_HASH_KEY ShortText
column_create Entry body COLUMN_SCALAR ShortText
EOC
  end

  def test_column_create_int32
    assert_same_dump(<<-EOC)
table_create Entry TABLE_HASH_KEY ShortText
column_create Entry body COLUMN_SCALAR Int32
EOC
  end

  def test_scaler_column_create
    assert_same_dump(<<-EOC)
table_create Entry TABLE_HASH_KEY ShortText
column_create Entry body COLUMN_SCALAR ShortText
EOC
  end

  def test_vector_column_create
    assert_same_dump(<<-EOC)
table_create Entry TABLE_PAT_KEY ShortText
column_create Entry body COLUMN_VECTOR ShortText
EOC
  end

  def test_index_column_create
    assert_same_dump(<<-EOC)
table_create Entry TABLE_HASH_KEY ShortText
column_create Entry body COLUMN_SCALAR ShortText
table_create Terms TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenBigram
column_create Terms entry_body COLUMN_INDEX Entry body
EOC
  end

  def test_table_with_index_column
    body = "作成するテーブルを語彙表として使用する場合、" +
           "文字列を分割するトークナイザを指定します。"
    assert_same_dump(<<-EOC)
table_create Entry TABLE_HASH_KEY ShortText
column_create Entry body COLUMN_SCALAR ShortText
table_create Terms TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenBigram
column_create Terms entry_body COLUMN_INDEX Entry body
load --table Entry
[
["_key","body"],
["gcc","#{body}"]
]
load --table Terms
[
["_key"],
["、"],
["。"],
["して"],
["しま"],
["す"],
["する"],
["て使"],
["とし"],
["ます"],
["るテ"],
["るト"],
["る場"],
["を分"],
["を指"],
["を語"],
["イザ"],
["クナ"],
["ザを"],
["テー"],
["トー"],
["ナイ"],
["ブル"],
["ルを"],
["ーク"],
["ーブ"],
["作成"],
["使用"],
["分割"],
["列を"],
["割す"],
["合"],
["場合"],
["字列"],
["定し"],
["彙表"],
["成す"],
["指定"],
["文字"],
["用す"],
["表と"],
["語彙"]
]
EOC
  end

  def test_table_with_key_index_column
    assert_same_dump(<<-EOC)
table_create Bookmarks TABLE_HASH_KEY ShortText
table_create Terms TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenBigram
column_create Terms bookmarks_key COLUMN_INDEX Bookmarks _key
EOC
  end

  def test_no_tokenizer_table_with_index_column
    assert_same_dump(<<-EOC)
table_create People TABLE_HASH_KEY ShortText
column_create People name COLUMN_SCALAR ShortText
table_create Bookmarks TABLE_HASH_KEY ShortText
column_create Bookmarks title COLUMN_SCALAR ShortText
column_create Bookmarks people COLUMN_VECTOR People
column_create People bookmarks COLUMN_INDEX Bookmarks people
load --table People
[
["_key","name"],
["morita","Daijiro MORI"],
["gunyara-kun","Tasuku SUENAGA"]
]
load --table Bookmarks
[
["_key","people","title"],
["http://groonga.org/",["morita"],"groonga"]
]
EOC
  end

  def test_table_with_index_column_sorted_by_id
    body = "作成するテーブルを語彙表として使用する場合、" +
           "文字列を分割するトークナイザを指定します。"
    commands = <<-EOC
table_create Terms TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenBigram
table_create Entry TABLE_HASH_KEY ShortText
column_create Entry body COLUMN_SCALAR ShortText
column_create Terms entry_body COLUMN_INDEX Entry body
load --table Entry
[
{"_key":"gcc","body":"#{body}"}
]
EOC

    assert_dump(<<-EOD, commands)
table_create Terms TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenBigram
table_create Entry TABLE_HASH_KEY ShortText
column_create Entry body COLUMN_SCALAR ShortText
column_create Terms entry_body COLUMN_INDEX Entry body
load --table Terms
[
["_key"],
["、"],
["。"],
["して"],
["しま"],
["す"],
["する"],
["て使"],
["とし"],
["ます"],
["るテ"],
["るト"],
["る場"],
["を分"],
["を指"],
["を語"],
["イザ"],
["クナ"],
["ザを"],
["テー"],
["トー"],
["ナイ"],
["ブル"],
["ルを"],
["ーク"],
["ーブ"],
["作成"],
["使用"],
["分割"],
["列を"],
["割す"],
["合"],
["場合"],
["字列"],
["定し"],
["彙表"],
["成す"],
["指定"],
["文字"],
["用す"],
["表と"],
["語彙"]
]
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
table_create Entry TABLE_HASH_KEY ShortText
column_create Entry body COLUMN_SCALAR ShortText
column_create Entry title COLUMN_SCALAR ShortText
table_create Terms TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenBigram
column_create Terms entry_body COLUMN_INDEX Entry title,body
load --table Entry
[
["_key","body","title"],
["gcc","#{body}","#{title}"]
]
load --table Terms
[
["_key"],
["_"],
["default"],
["tokenizer"],
["、"],
["。"],
["して"],
["しま"],
["す"],
["する"],
["て使"],
["とし"],
["ます"],
["るテ"],
["るト"],
["る場"],
["を分"],
["を指"],
["を語"],
["イザ"],
["クナ"],
["ザを"],
["テー"],
["トー"],
["ナイ"],
["ブル"],
["ルを"],
["ーク"],
["ーブ"],
["作成"],
["使用"],
["分割"],
["列を"],
["割す"],
["合"],
["場合"],
["字列"],
["定し"],
["彙表"],
["成す"],
["指定"],
["文字"],
["用す"],
["表と"],
["語彙"]
]
EOC
  end

  def test_load
    assert_same_dump(<<-EOC)
table_create commands TABLE_PAT_KEY ShortText
column_create commands body COLUMN_SCALAR ShortText
load --table commands
[
["_key","body"],
["bash","a shell"],
["gcc","a compiler"]
]
EOC
  end

  def test_load_to_value_pseudo_column_of_hash_table
    assert_same_dump(<<-EOC)
table_create users TABLE_HASH_KEY ShortText Int32
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
table_create users TABLE_PAT_KEY ShortText Int32
load --table users
[
["_key","_value"],
["hayamiz",1001],
["ryoqun",1000]
]
EOC
  end

  def test_load_to_value_pseudo_column_of_array_table
    assert_same_dump(<<-EOC)
table_create users TABLE_NO_KEY --value_type Int32
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
table_create groups TABLE_HASH_KEY ShortText
table_create users TABLE_HASH_KEY ShortText groups
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
table_create groups TABLE_NO_KEY
column_create groups name COLUMN_SCALAR ShortText
table_create users TABLE_HASH_KEY ShortText groups
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
table_create commands TABLE_NO_KEY
column_create commands body COLUMN_SCALAR ShortText
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
table_create commands TABLE_PAT_KEY ShortText
column_create commands body COLUMN_SCALAR Int32
load --table commands
[
["_key","body"],
["bash",-2715],
["gcc",32]
]
EOC
  end

  def test_vector_empty_load
    assert_same_dump(<<-EOC)
table_create commands TABLE_PAT_KEY ShortText
column_create commands body COLUMN_VECTOR ShortText
load --table commands
[
["_key","body"],
["gcc",[]]
]
EOC
  end

  def test_vector_string_load
    assert_same_dump(<<-EOC)
table_create commands TABLE_PAT_KEY ShortText
column_create commands body COLUMN_VECTOR ShortText
load --table commands
[
["_key","body"],
["gcc",["C","and","C++","Compiler"]]
]
EOC
  end

  def test_vector_int32_load
    assert_same_dump(<<-EOC)
table_create commands TABLE_PAT_KEY ShortText
column_create commands body COLUMN_VECTOR Int32
load --table commands
[
["_key","body"],
["gcc",[827,833,991,2716]]
]
EOC
  end

  def test_load_with_text_reference_key
    assert_same_dump(<<EOGQTP)
table_create users TABLE_HASH_KEY ShortText
table_create comments TABLE_PAT_KEY ShortText
column_create comments text COLUMN_SCALAR ShortText
column_create comments author COLUMN_SCALAR users
load --table users
[
["_key"],
["ryoqun"],
["hayamiz"]
]
load --table comments
[
["_key","author","text"],
["groonga","ryoqun","it is fast"],
["ruby","hayamiz","it is fun"]
]
EOGQTP
  end

  def test_load_with_vector_text_reference_key
    assert_same_dump(<<EOGQTP)
table_create users TABLE_HASH_KEY ShortText
table_create comments TABLE_PAT_KEY ShortText
column_create comments text COLUMN_SCALAR ShortText
column_create comments author COLUMN_VECTOR users
load --table users
[
["_key"],
["ryoqun"],
["hayamiz"]
]
load --table comments
[
["_key","author","text"],
["groonga",["ryoqun","hayamiz"],"it is fast"]
]
EOGQTP
  end

  def test_load_with_int32_reference_key
    assert_same_dump(<<EOGQTP)
table_create users TABLE_HASH_KEY Int32
column_create users name COLUMN_SCALAR ShortText
table_create comments TABLE_PAT_KEY ShortText
column_create comments text COLUMN_SCALAR ShortText
column_create comments author COLUMN_SCALAR users
load --table users
[
["_key","name"],
[1000,"ryoqun"],
[1001,"hayamiz"]
]
load --table comments
[
["_key","author","text"],
["groonga",1000,"it is fast"],
["ruby",1001,"it is fun"]
]
EOGQTP
  end

  def test_load_with_reference_id
    assert_same_dump(<<EOGQTP)
table_create users TABLE_NO_KEY
column_create users name COLUMN_SCALAR ShortText
table_create comments TABLE_PAT_KEY ShortText
column_create comments text COLUMN_SCALAR ShortText
column_create comments author COLUMN_SCALAR users
load --table users
[
["_id","name"],
[1,"ryoqun"],
[2,"hayamiz"]
]
load --table comments
[
["_key","author","text"],
["groonga",1,"it is fast"],
["ruby",2,"it is fun"]
]
EOGQTP
  end

  def test_load_with_vector_int32_reference_key
    assert_same_dump(<<EOGQTP)
table_create users TABLE_HASH_KEY Int32
column_create users name COLUMN_SCALAR ShortText
table_create comments TABLE_PAT_KEY ShortText
column_create comments text COLUMN_SCALAR ShortText
column_create comments author COLUMN_VECTOR users
load --table users
[
["_key","name"],
[1000,"ryoqun"],
[1001,"hayamiz"]
]
load --table comments
[
["_key","author","text"],
["groonga",[1000,1001],"it is fast"]
]
EOGQTP
  end

  def test_load_with_vector_reference_id
    assert_same_dump(<<EOGQTP)
table_create users TABLE_NO_KEY
column_create users name COLUMN_SCALAR ShortText
table_create comments TABLE_PAT_KEY ShortText
column_create comments text COLUMN_SCALAR ShortText
column_create comments author COLUMN_VECTOR users
load --table users
[
["_id","name"],
[1,"ryoqun"],
[2,"hayamiz"]
]
load --table comments
[
["_key","author","text"],
["groonga",[1,2],"it is fast"]
]
EOGQTP
  end

  def test_load_chained_subtables
    assert_same_dump(<<EOGQTP)
table_create words TABLE_HASH_KEY ShortText
table_create japanese TABLE_HASH_KEY words
table_create noun TABLE_HASH_KEY japanese
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
[[0,0.0,0.0],true]
[[0,0.0,0.0],3]
[[0,0.0,0.0],true]
[[0,0.0,0.0],[[[2],[["_id","UInt32"],["_key","ShortText"]],[1,"hayamiz"],[3,"mori"]]]]
[[0,0.0,0.0],true]
EXPECTED
table_create users TABLE_HASH_KEY ShortText
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
[[0,0.0,0.0],true]
[[0,0.0,0.0],true]
[[0,0.0,0.0],3]
[[0,0.0,0.0],true]
[[0,0.0,0.0],[[[2],[["_id","UInt32"],["name","ShortText"]],[1,"hayamiz"],[3,"mori"]]]]
[[0,0.0,0.0],true]
EXPECTED
table_create users TABLE_NO_KEY
column_create users name COLUMN_SCALAR ShortText
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
table_create users TABLE_NO_KEY
column_create users name COLUMN_SCALAR ShortText
table_create blog_entries TABLE_NO_KEY
column_create blog_entries body COLUMN_SCALAR ShortText
column_create blog_entries author COLUMN_SCALAR users
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
["_id","author","body"],
[1,1,"Today was very chilly."],
[2,8,"Taiyaki is very yummy."],
[3,5,"I was programming."]
]
COMMANDS

    result = feed_commands(<<COMMANDS)
select blog_entries --output_columns author.name
COMMANDS

    assert_equal(<<-EXPECTED, result)
[[0,0.0,0.0],[[[3],[["author.name","ShortText"]],["hayamiz"],["ryoqun"],["mori"]]]]
[[0,0.0,0.0],true]
EXPECTED
  end

  def test_view
    assert_same_dump(<<COMMANDS)
table_create View TABLE_VIEW
table_create FreePrograms TABLE_PAT_KEY ShortText
table_create NonFreePrograms TABLE_PAT_KEY ShortText
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
["Nvidia Video Driver"],
["Windows"]
]
COMMANDS
  end
end
