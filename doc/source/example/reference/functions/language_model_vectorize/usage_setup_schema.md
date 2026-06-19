Execution example:

```shell
table_create --name Memos --flags TABLE_NO_KEY
# [[0,1337566253.89858,0.000355720520019531],true]
column_create \
  --table Memos \
  --name content \
  --flags COLUMN_SCALAR \
  --type ShortText
# [[0,1337566253.89858,0.000355720520019531],true]
```
