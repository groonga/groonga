<!-- groonga-command -->
<!-- database: extractors_extractor_json -->

(extractor-json)=

# `ExtractorJSON`

## Summary

```{versionadded} 16.0.6

```

```{note}
This is an experimental feature. Currently, this feature is still not stable.
```

`ExtractorJSON` extracts values from JSON data by a JSONPath
expression. You can use this extractor to index only the values you
need from JSON without indexing the whole JSON text.

The extracted values keep their JSON types. For example, a string in
JSON is extracted as a text and an integer in JSON is extracted as an
integer. So you can use `ExtractorJSON` for both full text search
against strings and range search against numbers.

```{note}
`ExtractorJSON` requires the
[jsoncons](https://danielaparker.github.io/jsoncons/) library. It isn't
available if Groonga is built without jsoncons.
```

## Syntax

`ExtractorJSON` has a required `path` parameter:

```
ExtractorJSON("path", "JSONPath")
```

## Usage

Here is an example that extracts all elements of the `tags` array by
the `$.tags[*]` JSONPath. The `title` value isn't extracted because it
doesn't match the JSONPath:

<!-- groonga-command -->

```{include} ../../example/reference/extractors/extractor_json/string.md
extract \
  --extractors 'ExtractorJSON("path", "$.tags[*]")' \
  --value '{"tags": ["groonga", "search", "engine"], "title": "ignored"}'
```

A JSONPath can match nested values. The following example extracts all
elements of nested arrays by the `$.values[*][*]` JSONPath:

<!-- groonga-command -->

```{include} ../../example/reference/extractors/extractor_json/nested.md
extract \
  --extractors 'ExtractorJSON("path", "$.values[*][*]")' \
  --value '{"values": [[1, 10], [100, 1000]]}'
```

When you attach `ExtractorJSON` to a lexicon, the lexicon indexes the
extracted values. The original JSON is kept in the data column.

The following example indexes integers in a JSON column. The lexicon
key type is `Int32` because the extracted values are integers. The
index is used automatically when the JSON column is loaded, so you can
search the original records by the extracted values:

<!-- groonga-command -->

```{include} ../../example/reference/extractors/extractor_json/index.md
table_create Data TABLE_NO_KEY
column_create Data value COLUMN_SCALAR JSON

table_create Numbers TABLE_PAT_KEY Int32 \
  --extractors 'ExtractorJSON("path", "$.value[*][*]")'
column_create Numbers data_value COLUMN_INDEX Data value

load --table Data
[
{"value": "{\"value\": [[1, 10], [100]]}"},
{"value": "{\"value\": [[2], [20, 200]]}"},
{"value": "{\"value\": [[-1, -10], [-100]]}"}
]

select Data --filter 'between(Numbers.data_value, 10, 20)'
```

The records whose extracted values are between `10` and `20` are
selected even though the values are stored in nested arrays in JSON.

## Parameters

### Required parameter

(extractor-json-path)=

#### `path`

Specifies a JSONPath expression that matches the values to extract.

A JSONPath expression begins with `$`, which means the root of the
JSON value. For example, `$.tags[*]` matches all elements of the
`tags` array, and `$.values[*][*]` matches all elements of nested
arrays under `values`.

This parameter is required. `ExtractorJSON` reports an error if this
parameter is missing.

## See also

- {doc}`/reference/extractors`
- {doc}`/reference/commands/table_create`
