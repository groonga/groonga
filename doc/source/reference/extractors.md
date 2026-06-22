<!-- groonga-command -->
<!-- database: extractors -->

# Extractors

## Summary

```{versionadded} 16.0.3

```

```{note}
This is an experimental feature. Currently, this feature is still not stable.
```

Groonga has extractor module that extracts plain text or values from
structured data such as HTML and JSON. It is used before tokenizing
and indexing a value. For example, {doc}`extractors/extractor_html`
extracts only the text content from HTML by removing tags, so that
markup such as `<p>` isn't indexed as a token.

Extractor module can be added as a plugin. You can customize value
extraction by registering your extractor plugins to Groonga.

An extractor module is attached to a table. The table is normally a
lexicon for an index. A table can have zero or more extractor
modules. You can attach extractor modules to a table by the
{ref}`table-create-extractors` option in
{doc}`/reference/commands/table_create`.

Here is an example `table_create` that uses the
{doc}`extractors/extractor_html` extractor module:

<!-- groonga-command -->

```{include} ../example/reference/extractors/example_table_create.md
table_create Terms TABLE_PAT_KEY ShortText --extractors ExtractorHTML
```

When extractors are set to a lexicon, they are applied automatically
when an index of the lexicon is updated. The extracted value is
tokenized and indexed instead of the original value. The original
value is still stored as-is in its data column. So you can search
against the extracted content while keeping the original structured
data.

If a table has multiple extractors, they are applied in order. The
output of an extractor is passed to the next extractor as its
input. This is useful when you need to combine extractors. For
example, you can extract a string from JSON by
{doc}`extractors/extractor_json` and then remove HTML tags in the
string by {doc}`extractors/extractor_html`.

You can use the {doc}`/reference/commands/extract` command to check
how extractors process a value. The {doc}`/reference/commands/extract`
command applies the specified extractors to the given value and
returns the extracted value. It doesn't need a table:

<!-- groonga-command -->

```{include} ../example/reference/extractors/example_extract.md
extract \
  --extractors 'ExtractorHTML' \
  --value "<html><body>He&lt;ll&gt;o</body></html>"
```

The `extract` command is useful to confirm the result of extractors
before you attach them to a lexicon.

## Built-in extractors

Here is a list of built-in extractors:

```{toctree}
:maxdepth: 1
:glob:

extractors/*
```

## See also

- {doc}`/reference/commands/extract`
- {doc}`/reference/commands/table_create`
