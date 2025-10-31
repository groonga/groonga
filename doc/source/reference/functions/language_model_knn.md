<!-- groonga-command -->
<!-- database: functions_language_model_knn -->

# `language_model_knn`

```{versionadded} 15.1.8

```

```{note}
This is an experimental feature. Currently, this feature is still not stable.
```

## Summary

`language_model_knn` is a function for semantic search.

Semantic search uses the k-Nearest Neighbors (k-NN) algorithm.

You must use it with {doc}`../tokenizers/token_language_model_knn`.

It can be used as a condition for `--filter` and as a sort key for `--sort_keys`.

To enable this function, register `language_model/knn` plugin by the following command:

```shell
plugin_register language_model/knn
```

## Syntax

`language_model_knn` requires two parameters:

```
language_model_knn(column, query)
```

`column` is the search target column. It must be a column with an index.

`query` is a search query.

## Requirements

You need Faiss enabled Groonga. The official packages enable it.

## Usage

You need to register `language_model/knn` plugin at first:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_knn/usage_register.md
plugin_register language_model/knn
```

Here is a schema definition and sample data.

Sample schema:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_knn/usage_setup_schema.md
table_create --name Memos --flags TABLE_NO_KEY
column_create \
  --table Memos \
  --name content \
  --flags COLUMN_SCALAR \
  --type ShortText
```

Sample data:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_knn/usage_setup_data.md
load --table Memos
[
{"content": "I am a boy."},
{"content": "This is an apple."},
{"content": "Groonga is a full text search engine."}
]
```

You need to store embedding information for each record. Here is how to create that column.

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_knn/column_create.md
column_create Memos embedding_code COLUMN_SCALAR ShortBinary
```

Create an index for semantic search.

Specify {doc}`../tokenizers/token_language_model_knn` as the tokenizer.
The tokenizer's arguments are `model` and `code_column`.
Specify the model to use for `model`, and specify the column to store the generated embedding information for `code_column`.

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_knn/index_column_create.md
table_create Centroids TABLE_HASH_KEY ShortBinary \
  --default_tokenizer \
    'TokenLanguageModelKNN("model", "hf:///groonga/all-MiniLM-L6-v2-Q4_K_M-GGUF", \
                           "code_column", "embedding_code")'

column_create Centroids data_content COLUMN_INDEX Memos content
```

This enables semantic search.
When you `load` data into `Memos.content`, Groonga automatically generates embeddings.
Users do not need to generate embeddings.

Here is an example of semantic search:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_knn/filter.md
select Memos \
  --filter 'language_model_knn(content, "male child")' \
  --output_columns content
```

`language_model_knn` function can also be used as a sort key.
Specify `language_model_knn` for `--sort_keys`.
Since you likely need to fetch results in descending order of similarity, you add a `-` prefix to fetch them in descending order.

Here is an example of filtering by `_id` and then sorting by similarity:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_knn/sort_keys.md
select Memos \
  --filter '_id < 3' \
  --sort_keys '-language_model_knn(content, "male child")' \
  --output_columns content
```

## Parameters

There are two required parameters.

### `column`

`column` is the search target column. It must be a column with an index.

### `query`

`query` is a search query.

## Return value

This function is worked as a selector. It means that this function can be executable effectively.

## See also

- {doc}`../tokenizers/token_language_model_knn`
