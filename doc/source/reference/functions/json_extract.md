<!-- groonga-command -->
<!-- database: functions_json_extract -->

# `json_extract`

```{versionadded} 16.0.6

```

```{note}
This is an experimental feature. Currently, this feature is still not stable.
```

## Summary

`json_extract` extracts values from a JSON by a JSONPath expression.

The extracted values keep their JSON types. For example, a string in
JSON is extracted as a text and an integer in JSON is extracted as an
integer. So you can use `json_extract` for both full text search
against strings and range search against numbers.

`json_extract` is often used in combination with the {ref}`select-filter`
option of {doc}`/reference/commands/select`. For example, you can
combine `json_extract` with {doc}`between` to do a range search against
values that are stored in a JSON.

```{note}
`json_extract` requires the
[simdjson](https://github.com/simdjson/simdjson) and
[jsoncons](https://danielaparker.github.io/jsoncons/) libraries. It
isn't available if Groonga is built without them.
```

## Syntax

`json_extract` requires two parameters:

```
json_extract(json, jsonpath)
```

## Usage

Here are a schema definition and sample data to show usage. The `value`
column uses the `JSON` type and stores nested arrays:

<!-- groonga-command -->

```{include} ../../example/reference/functions/json_extract/usage_setup.md
table_create Data TABLE_NO_KEY
column_create Data value COLUMN_SCALAR JSON

load --table Data
[
{"value": "{\"value\": [[1, 10], [100]]}"},
{"value": "{\"value\": [[2], [20, 200]]}"},
{"value": "{\"value\": [[-1, -10], [-100]]}"}
]
```

The following example extracts all elements of the nested arrays by the
`$.value[*][*]` JSONPath and selects the records that have a value
between `10` and `20`:

<!-- groonga-command -->

```{include} ../../example/reference/functions/json_extract/usage_select.md
select Data --filter 'between(json_extract(value, "$.value[*][*]"), 10, 20)'
```

The records whose extracted values are between `10` and `20` are
selected even though the values are stored in nested arrays in JSON.

## Parameters

There are two required parameters.

### Required parameters

#### `json`

`json` is the target JSON. It must be a parsed JSON, such as a value of
a `JSON` typed column, or a JSON string.

`json_extract` reports an error if this parameter isn't a parsed JSON
nor a JSON string.

#### `jsonpath`

`jsonpath` is a JSONPath expression that matches the values to extract.

A JSONPath expression begins with `$`, which means the root of the JSON
value. For example, `$.value[*]` matches all elements of the `value`
array, and `$.value[*][*]` matches all elements of nested arrays under
`value`.

`json_extract` reports an error if this parameter isn't a valid
JSONPath string.

## Return value

`json_extract` returns a vector that contains all values matched by the
JSONPath expression. The extracted values keep their JSON types.

## See also

- {doc}`between`
- {ref}`extractor-json`
- {doc}`/reference/commands/select`
