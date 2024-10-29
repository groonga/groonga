<!-- groonga-command -->
<!-- database: functions_language_model_vectorize -->

# `language_model_vectorize`

```{versionadded} 14.1.0

```

```{note}
This is an experimental feature. Currently, this feature is still not stable.
```

## Summary

`language_model_vectorize` generates a normalized embedding from the
given text.

See also {doc}`../language_model` how to prepare a language model.

You can use {ref}`generated-column` to automate embeddings generation.

To enable this function, register `functions/language_model` plugin by
the following command:

```shell
plugin_register functions/vector
```

## Syntax

`language_model_vectorize` requires two parameters:

```
language_model_vectorize(model_name, text)
```

`mode_name` is the name of language mode to be used. It's associated
with file name. If
`${PREFIX}/share/groonga/language_models/mistral-7b-v0.1.Q4_K_M.gguf`
exists, you can refer it by `mistral-7b-v0.1.Q4_K_M`. It's computed by
removing directory and `.gguf` extension.

`text` is the input text.

## Requirements

You need llama.cpp enabled Groonga. The official packages enable it.

You need enough CPU/memory resources to use this feature. Language
model related features require more resources than other features.

You can use GPU in the feature.

## Usage

You need to register `functions/language_model` plugin at first:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_vectorize/usage_register.md
plugin_register functions/language_model
```

Here is a schema definition and sample data.

Sample schema:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_vectorize/usage_setup_schema.md
table_create --name Memos --flags TABLE_NO_KEY
column_create \
  --table Memos \
  --name content \
  --flags COLUMN_SCALAR \
  --type ShortText
```

Sample data:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_vectorize/usage_setup_data.md
load --table Memos
[
{"content": "Groonga is fast and embeddable full text search engine."},
{"content": "PGroonga is a PostgreSQL extension that uses Groonga."},
{"content": "PostgreSQL is a RDBMS."}
]
```

Here is a schema that creates a {ref}`generated-column` that
generates embeddings of `Memos.content` automatically:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_vectorize/usage_setup_generated_column.md
column_create \
  --table Memos \
  --name content_embedding \
  --flags COLUMN_VECTOR \
  --type Float32 \
  --source content \
  --generator 'language_model_vectorize("mistral-7b-v0.1.Q4_K_M", content)'
```

You can re-rank matched records by using `distance_inner_product()`
not `distance_cosine()` because `language_model_vectorize()` returns a
normalized embedding. The following example uses all records instead
of filtered records to show this usage simply:

<!-- groonga-command -->

```{include} ../../example/reference/functions/language_model_vectorize/usage_rerank.md
select \
  --table Memos \
  --columns[similarity].stage filtered \
  --columns[similarity].flags COLUMN_SCALAR \
  --columns[similarity].types Float32 \
  --columns[similarity].value 'distance_inner_product(content_embedding, language_model_vectorize("mistral-7b-v0.1.Q4_K_M", "high performance FTS"))' \
  --output_columns content,similarity \
  --sort_keys -similarity
```

## Parameters

There are two required parameters.

### `model_name`

`mode_name` is the name of language mode to be used. It's associated
with file name. If
`${PREFIX}/share/groonga/language_models/mistral-7b-v0.1.Q4_K_M.gguf`
exists, you can refer it by `mistral-7b-v0.1.Q4_K_M`. It's computed by
removing directory and `.gguf` extension.

### `text`

`text` is the input text.

## Return value

`language_model_vectorize` returns `Float32` vector which as a
normalized embedding.
