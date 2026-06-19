Execution example:

```shell
table_create Centroids TABLE_HASH_KEY ShortBinary \
  --default_tokenizer \
    'TokenLanguageModelKNN("model", "hf:///groonga/all-MiniLM-L6-v2-Q4_K_M-GGUF", \
                           "code_column", "embedding_code")'
# [[0,1337566253.89858,0.000355720520019531],true]
column_create Centroids data_content COLUMN_INDEX Memos content
# [[0,1337566253.89858,0.000355720520019531],true]
```
