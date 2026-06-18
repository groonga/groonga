<!-- groonga-command -->
<!-- database: commands_extract -->

# `extract`

## Summary

```{versionadded} 16.0.3

```

```{note}
This is an experimental feature. Currently, this feature is still not stable.
```

`extract` command extracts plain text or values from structured data
such as HTML and JSON by the specified extractors.

There is no need to create a table to use `extract` command. It is
useful for you to check the results of extractors before you attach
them to a lexicon by the {ref}`table-create-extractors` option of
{doc}`table_create`.

See {doc}`/reference/extractors` for details of extractors.

## Syntax

This command takes two parameters.

Both `extractors` and `value` are required:

```
extract extractors
        value
```

## Usage

Here is an example that extracts text content from HTML by
{doc}`/reference/extractors/extractor_html`. It removes HTML tags and
expands character references:

<!-- groonga-command -->

```{include} ../../example/reference/commands/extract/html.md
extract \
  --extractors 'ExtractorHTML' \
  --value "<html><body>He&lt;ll&gt;o</body></html>"
```

Here is an example that extracts values from JSON by
{doc}`/reference/extractors/extractor_json`. The `$.tags[*]` JSONPath
matches all elements of the `tags` array:

<!-- groonga-command -->

```{include} ../../example/reference/commands/extract/json.md
extract \
  --extractors 'ExtractorJSON("path", "$.tags[*]")' \
  --value '{"tags": ["groonga", "search", "engine"], "title": "ignored"}'
```

## Parameters

This section describes parameters of `extract`.

### Required parameters

There are required parameters, `extractors` and `value`.

(extract-extractors)=

#### `extractors`

Specifies extractors separated by `,`. `extract` command applies the
extractors to `value` in order. The output of an extractor is passed
to the next extractor as its input.

See {doc}`/reference/extractors` for all extractors.

(extract-value)=

#### `value`

Specifies the value that you want to extract plain text or values
from.

If you want to include spaces in `value`, you need to quote `value` by
single quotation (`'`) or double quotation (`"`).

## Return value

```
[HEADER, {"extracted": EXTRACTED_VALUE}]
```

`HEADER`
: See {doc}`/reference/command/output_format` about `HEADER`.

`EXTRACTED_VALUE`
: The value extracted by the specified extractors. It's a single value
when the extractors return a single value such as
{doc}`/reference/extractors/extractor_html`. It's an array when the
extractors return multiple values such as
{doc}`/reference/extractors/extractor_json`.

## See also

- {doc}`/reference/extractors`
- {doc}`table_create`
- {doc}`/reference/command/output_format`
