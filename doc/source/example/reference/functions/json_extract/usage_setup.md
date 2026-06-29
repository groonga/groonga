Execution example:

```shell
table_create Data TABLE_NO_KEY
# [[0,1337566253.89858,0.000355720520019531],true]
column_create Data value COLUMN_SCALAR JSON
# [[0,1337566253.89858,0.000355720520019531],true]
load --table Data
[
{"value": "{\"value\": [[1, 10], [100]]}"},
{"value": "{\"value\": [[2], [20, 200]]}"},
{"value": "{\"value\": [[-1, -10], [-100]]}"}
]
# [[0,1337566253.89858,0.000355720520019531],3]
```
