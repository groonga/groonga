Execution example:

```shell
column_create \
  --table Memos \
  --name content_embedding \
  --flags COLUMN_VECTOR \
  --type Float32 \
  --source content \
  --generator 'language_model_vectorize("mistral-7b-v0.1.Q4_K_M", content)'
# [[0,1337566253.89858,0.000355720520019531],true]
```
