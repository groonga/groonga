<!-- groonga-command -->
<!-- database: functions_html_extract -->

# `html_extract`

```{versionadded} 16.1.6

```

```{note}
This is an experimental feature. Currently, this feature is still not stable.
```

## Summary

`html_extract` extracts text content from HTML.
It removes HTML tags and expands character references such as `&lt;`.

`html_extract` behaves the same as {ref}`extractor-html`.
You can use `html_extract` to get the text content in HTML as a value.
For example, you can output the main content of an HTML by {ref}`select-output-columns` or store it to a {ref}`generated-column`.

## Syntax

`html_extract` requires one parameter. `options` is optional:

```
html_extract(html)
html_extract(html, options)
```

`options` uses the following format.
All of key-value pairs are optional:

```
{
  "selector": selector
}
```

## Usage

Here are a schema definition and sample data to show usage.

<!-- groonga-command -->

```{include} ../../example/reference/functions/html_extract/usage_setup.md
table_create Pages TABLE_NO_KEY
column_create Pages html COLUMN_SCALAR Text

load --table Pages
[
{"html": "<html><body>He&lt;ll&gt;o</body></html>"},
{"html": "<nav>Menu</nav><article>Hello</article><footer>Footer</footer>"}
]
```

Here is an example that extracts text content from HTML.
HTML tags (`<html>`, `<body>` and so on) are removed and character references (`&lt;` and `&gt;`) are expanded:

<!-- groonga-command -->

```{include} ../../example/reference/functions/html_extract/usage_default.md
select Pages --output_columns 'html_extract(html)'
```

You can extract only the content in the specified HTML tag by {ref}`html-extract-selector`.
This is useful when you want to extract only the main content of a page.
The first record doesn't have `<article>`.
So an empty text is returned for it:

<!-- groonga-command -->

```{include} ../../example/reference/functions/html_extract/usage_selector.md
select Pages --output_columns 'html_extract(html, {"selector": "article"})'
```

You can use `html_extract` in a {ref}`generated-column`.
The following example stores the main content of each HTML to the `content` column automatically:

<!-- groonga-command -->

```{include} ../../example/reference/functions/html_extract/usage_generated_column.md
column_create Pages content COLUMN_SCALAR Text html \
  --generator 'html_extract(html, {"selector": "article"})'
select Pages --output_columns content
```

## Parameters

There is one required parameter.
There is one optional parameter.

### Required parameters

#### `html`

`html` is the target HTML.
It must be a value of a text type such as `ShortText`, `Text` and `LongText`.

`html_extract` reports an error if this parameter isn't a text.

### Optional parameters

#### `options`

`options` can be passed as a JSON object.

(html-extract-selector)=

##### `selector`

Specifies elements to extract text content from.
Text content out of the elements is ignored.

This is the same as {ref}`extractor-html-selector` of {ref}`extractor-html`.
It has the same limitations too.
See {ref}`extractor-html-selector` for details.

The default value is no selector.
It means that all text content is extracted.

## Return value

`html_extract` returns the extracted text content as a `Text`.

If {ref}`html-extract-selector` is specified and no element matches it, `html_extract` returns an empty text.

## See also

- {ref}`extractor-html`
- {doc}`html_untag`
- {doc}`/reference/commands/select`
