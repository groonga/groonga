<!-- groonga-command -->
<!-- database: string_truncate -->

# `string_truncate`

```{versionadded} 16.0.9

```

## Summary

`string_truncate` truncates a string to at most the specified number of characters.
If the string is truncated, the tail of the kept characters is replaced with an omission mark so that the result,
including the omission mark, is `length` characters long, in the same way as Ruby on Rails' `String#truncate`.

To enable this function, register `functions/string` plugin with the following command:

```
plugin_register functions/string
```

## Syntax

`string_truncate` requires two or three parameters.

```
string_truncate(target, length[, options])
```

`options` uses the following format. All key-value pairs are optional:

```
{
  "omission": omission
}
```

## Usage

Here are a schema definition and sample data to show usage.

Sample schema:

<!-- groonga-command -->

```{include} ../../example/reference/functions/string_truncate/usage_setup_schema.md
plugin_register functions/string
table_create Memos TABLE_HASH_KEY ShortText
```

Sample data:

<!-- groonga-command -->

```{include} ../../example/reference/functions/string_truncate/usage_setup_data.md
load --table Memos
[
{"_key": "Groonga is a full text search engine"}
]
```

Here is a simple example.
Because the string is longer than the specified `length`, it's truncated and `"..."` is appended so that the result is 15 characters long.

<!-- groonga-command -->

```{include} ../../example/reference/functions/string_truncate/usage_basic.md
select Memos \
  --output_columns '_key, string_truncate(_key, 15)'
```

If the string isn't longer than `length`, it's returned as-is without an omission mark.

<!-- groonga-command -->

```{include} ../../example/reference/functions/string_truncate/usage_no_truncation.md
select Memos \
  --output_columns '_key, string_truncate(_key, 100)'
```

The following example specifies a custom `omission` in `options`.

<!-- groonga-command -->

```{include} ../../example/reference/functions/string_truncate/usage_omission.md
select Memos \
  --output_columns '_key, string_truncate(_key, 15, { "omission" : "＊＊＊" })'
```

When truncation occurs and `length` is smaller than the number of characters in `omission`, the result is only `omission` even if `omission` is longer than `length`.

<!-- groonga-command -->

```{include} ../../example/reference/functions/string_truncate/usage_omission_only.md
select Memos \
  --output_columns '_key, string_truncate(_key, 2)'
```

You can specify a string literal instead of a column.

<!-- groonga-command -->

```{include} ../../example/reference/functions/string_truncate/usage_string_literal.md
select Memos \
  --output_columns 'string_truncate("Groonga is a fast fulltext search engine", 15)'
```

## Parameters

### Required parameters

#### `target`

Specify a string literal or a string type column.

#### `length`

Specify the maximum number of characters of the result, including the `omission` mark when truncation occurs.
If the number of characters in `target` is less than or equal to `length`, `target` is returned as-is.

If `target` is longer than `length`, `string_truncate` subtracts the number of characters in `omission` from `length`, extracts that many characters from the beginning of `target`, and appends `omission`.
If `omission` is longer than `length` or `length` is negative, the result is `omission`.

### Optional parameters

#### `options`

Specify the following key.

- `omission`
  - Specify a string to be appended to the truncated string to show that it was truncated.
  - The default is `"..."`.

## Return value

`string_truncate` returns `target` truncated. When truncation occurs, the result (kept part + `omission`) is `length` characters long unless `omission` is longer than `length` or `length` is negative, in which case the result is `omission`.

## See also

- {doc}`snippet`
- {doc}`string_substring`
