<!-- groonga-command -->
<!-- database: extractors_extractor_html -->

(extractor-html)=

# `ExtractorHTML`

## Summary

```{versionadded} 16.0.3

```

```{note}
This is an experimental feature. Currently, this feature is still not stable.
```

`ExtractorHTML` extracts text content from HTML. It removes HTML tags
and expands character references (HTML entities such as `&lt;`) by
default. You can use this extractor to index only the text in HTML
without markup.

`ExtractorHTML` does nothing for a value that isn't a text type such
as `ShortText`, `Text` and `LongText`. The value is returned as-is in
this case.

## Syntax

`ExtractorHTML` has optional parameters.

No options:

```
ExtractorHTML
```

Specify options:

```
ExtractorHTML("remove_tag", true)

ExtractorHTML("expand_character_reference", true)
```

## Usage

Here is an example that extracts text content from HTML by the default
parameters. HTML tags (`<html>`, `<body>` and so on) are removed and
character references (`&lt;` and `&gt;`) are expanded:

<!-- groonga-command -->

```{include} ../../example/reference/extractors/extractor_html/default.md
extract \
  --extractors ExtractorHTML \
  --value "<html><body>He&lt;ll&gt;o</body></html>"
```

You can keep HTML tags by setting {ref}`extractor-html-remove-tag` to
`false`. Character references are still expanded in this case:

<!-- groonga-command -->

```{include} ../../example/reference/extractors/extractor_html/remove_tag.md
extract \
  --extractors 'ExtractorHTML("remove_tag", false)' \
  --value "<html><body>He&lt;ll&gt;o</body></html>"
```

You can keep character references as-is by setting
{ref}`extractor-html-expand-character-reference` to `false`. HTML tags
are still removed in this case:

<!-- groonga-command -->

```{include} ../../example/reference/extractors/extractor_html/expand_character_reference.md
extract \
  --extractors 'ExtractorHTML("expand_character_reference", false)' \
  --value "<html><body>He&lt;ll&gt;o</body></html>"
```

When you attach `ExtractorHTML` to a lexicon, the lexicon indexes the
extracted text content. The original HTML is kept in the data column:

<!-- groonga-command -->

```{include} ../../example/reference/extractors/extractor_html/index.md
table_create Contents TABLE_NO_KEY
column_create Contents html COLUMN_SCALAR Text

table_create Terms TABLE_PAT_KEY ShortText \
  --default_tokenizer TokenBigram \
  --normalizers NormalizerNFKC \
  --extractors ExtractorHTML
column_create Terms contents_html COLUMN_INDEX|WITH_POSITION Contents html

load --table Contents
[
{"html": "<p>Groonga is a <b>fast</b> full text search engine.</p>"}
]

select Contents \
  --match_columns html \
  --query "fast" \
  --output_columns html
select Contents \
  --match_columns html \
  --query "<b>" \
  --output_columns html
```

The query `fast` matches but the query `<b>` doesn't match because the
indexed token comes from the extracted text `Groonga is a fast full
text search engine.` instead of the raw HTML.

## Parameters

### Optional parameters

(extractor-html-remove-tag)=

#### `remove_tag`

Specifies whether HTML tags are removed.

If this is `true`, HTML tags such as `<p>` are removed. If this is
`false`, HTML tags are kept as-is.

The default value is `true`.

(extractor-html-expand-character-reference)=

#### `expand_character_reference`

Specifies whether character references (HTML entities) are expanded.

If this is `true`, both named character references such as `&lt;` and
numeric character references such as `&#x3042;` are expanded to the
corresponding characters. If this is `false`, character references are
kept as-is.

The default value is `true`.

## See also

- {doc}`/reference/extractors`
- {doc}`/reference/commands/table_create`
