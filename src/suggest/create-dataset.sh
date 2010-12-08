#!/bin/sh
# Making table-set for groonga suggestion function.
# (c) Brazil, Inc.

if [ 2 != $# ]; then
  echo "usage: $0 dbpath dataset_name"
  exit 1
fi

DBPATH=$1
DATASET=$2

if [ ! -f ${DBPATH} ]; then
  echo "quit" | groonga -n ${DBPATH}
fi

groonga ${DBPATH} <<_EOT_
register suggest/suggest
table_create event_type TABLE_HASH_KEY ShortText
table_create bigram TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenBigram
table_create kana TABLE_PAT_KEY|KEY_NORMALIZE ShortText

table_create item_${DATASET} TABLE_PAT_KEY|KEY_NORMALIZE ShortText --default_tokenizer TokenDelimit
column_create bigram item_${DATASET}_key COLUMN_INDEX|WITH_POSITION item_${DATASET} _key
column_create item_${DATASET} kana COLUMN_VECTOR kana
column_create kana item_${DATASET}_kana COLUMN_INDEX item_${DATASET} kana
column_create item_${DATASET} freq COLUMN_SCALAR Int32
column_create item_${DATASET} last COLUMN_SCALAR Time
column_create item_${DATASET} boost COLUMN_SCALAR Int32
column_create item_${DATASET} freq2 COLUMN_SCALAR Int32
column_create item_${DATASET} buzz COLUMN_SCALAR Int32

table_create pair_${DATASET} TABLE_HASH_KEY UInt64
column_create pair_${DATASET} pre COLUMN_SCALAR item_${DATASET}
column_create pair_${DATASET} post COLUMN_SCALAR item_${DATASET}
column_create pair_${DATASET} freq0 COLUMN_SCALAR Int32
column_create pair_${DATASET} freq1 COLUMN_SCALAR Int32
column_create pair_${DATASET} freq2 COLUMN_SCALAR Int32
column_create item_${DATASET} co COLUMN_INDEX pair_${DATASET} pre

table_create sequence_${DATASET} TABLE_HASH_KEY ShortText
table_create event_${DATASET} TABLE_NO_KEY
column_create sequence_${DATASET} events COLUMN_VECTOR|RING_BUFFER event_${DATASET}
column_create event_${DATASET} type COLUMN_SCALAR event_type
column_create event_${DATASET} time COLUMN_SCALAR Time
column_create event_${DATASET} item COLUMN_SCALAR item_${DATASET}
column_create event_${DATASET} sequence COLUMN_SCALAR sequence_${DATASET}
_EOT_
