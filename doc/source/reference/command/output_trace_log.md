# Output trace log

## Summary

```{versionadded} 13.0.9
```

```{note}

This is an experimental feature. Currently, this feature is still not stable.

This feature requires {doc}`/reference/command/command_version` 3 or later.
```

Trace log is similar information to PostgreSQL's [`EXPLAIN (ANALYZE,
VERBOSE)`][postgresql-explain] information. It includes the followings:

- What was executed?
- Which index was used?
- How long did the operation take?
- What value was used internal?
- ...

Trace log contents may be insufficient for now because this feature
is still experimental. If you want more information, please report it
to [the GitHub issue][github-issue] with your use case.

## Usage

You can get trace log by specifying `yes` as `output_trace_log`. You
also need to specify `3` as :doc:`/reference/command/command_version`.

Command line style:

```text
select ... --output_trace_log yes --command_version 3
```

URI style:

```text
/d/select?...&output_trace_log=yes&command_version=3
```

## Format

Trace log is available only when you use JSON or Apache Arrow
{doc}`/reference/command/output_format`. If you don't specify output
format, JSON is used by default.

### JSON

You can specify JSON output format explicitly like the followings:

Command line style:

```text
select ... --output_trace_log yes --command_version 3 --output_format json
```

URI style:

```text
/d/select.json?...&output_trace_log=yes&command_version=3
```

or

```text
/d/select?...&output_trace_log=yes&command_version=3&output_format=json
```

Here is an output structure for JSON output format:

```json
{
  "header": {},
  "trace_log": {
    "columns": [
      {"name": "depth"},
      {"name": "sequence"},
      {"name": "name"},
      {"name": "value"},
      {"name": "elapsed_time"}
    ],
    "logs": [
      [1, 0, "ii.select.input", "Hello", 100],
      [2, 0, "ii.select.exact.n_hits", 2, 200],
      [1, 1, "ii.select.n_hits", 2, 210]
    ],
  ],
  "body": []
}
```

If `output_trace_log` is `yes`, `"trace_log": {...}` is added. It has
2 key-value pairs:

```{list-table}
:header-rows: 1

* - Key
  - Description
* - `columns`
  - An array of column metadata for each trace log entry
* - `logs`
  - An array of trace log entries
```

Each `columns` element has the following key-value pairs:

```{list-table}
:header-rows: 1

* - Key
  - Description
* - `name`
  - The name of column
```

Each `logs` element has the following elements:

```{list-table}
:header-rows: 1

* - Index
  - Name
  - Description
* - 0
  - `depth`
  - The execution level.

    Multiple trace logs may be outputted for one execution level. If there are
    multiple trace logs for one execution level, `sequence` is incremented for
    each trace logs in the same execution level.
* - 1
  - `sequence`
  - The sequence number in the same execution level.

    If this is `0`, it shows that new execution level is started.
* - 2
  - `name`
  - The name of this trace log.
* - 3
  - `value`
  - The value of this trace log.

    The value type must be one of the followings:
    * Integer
    * String
* - 4
  - `elapsed_time`
  - The elapsed time since the command is started. Unit is nano second.
```

`logs` is a flat array but it may be able to format as a tree like
PostgreSQL's `EXPLAIN (ANALYZE, VERBOSE)` output based on `depth` and
`sequence`. Groonga may have an option to do it in the future or we
may provide a tool for it.

Example:

```{literalinclude} ../../../../test/command/suite/select/fuzzy/output_trace_log/json.expected
:language: json
:start-after: select
```

### Apache Arrow

You can specify Apache Arrow output format explicitly like the
followings:

Command line style:

```text
select ... --output_trace_log yes --command_version 3 --output_format apache-arrow
```

URI style:

```text
/d/select.arrows?...&output_trace_log=yes&command_version=3
```

or

```text
/d/select?...&output_trace_log=yes&command_version=3&output_format=apache-arrow
```

Here is an output structure for Apache Arrow output format:

```text
----
Apache Arrow record batch (stream format)
Metadata:
  GROONGA:data_type: metadata
----
----
Apache Arrow record batch (stream format)
Metadata:
  GROONGA:data_type: trace_log
Schema:
  depth: uint16
  sequence: uint16
  name: string
  value: dense_union<0: uint32=0, 1: string=1>
  elapsed_time: uint64
----
Apache Arrow record batch (stream format)
Metadata:
  GROONGA:n_hits: N
----
```

The second record batch is a trace log. You can check
`GROONGA:data_type` metadata to detect a record batch for trace log.

Here is descriptions of each column of the record batch:

```{list-table}
:header-rows: 1

* - Name
  - Description
* - `depth`
  - The execution level.

    Multiple trace logs may be outputted for one execution level. If there are
    multiple trace logs for one execution level, `sequence` is incremented for
    each trace logs in the same execution level.
* - `sequence`
  - The sequence number in the same execution level.

    If this is `0`, it shows that new execution level is started.
* - `name`
  - The name of this trace log.
* - `value`
  - The value of this trace log.

    The value type must be one of the followings:
    * Integer
    * String
* - `elapsed_time`
  - The elapsed time since the command is started. Unit is nano second.
```

Example:

```{literalinclude} ../../../../test/command/suite/select/fuzzy/output_trace_log/apache_arrow.expected
:language: none
:start-at: depth
:end-before: ====
```

[postgresql-explain]:https://www.postgresql.org/docs/current/sql-explain.html

[github-issue]:https://github.com/groonga/groonga/issues/new?labels=Feature&template=feature_request.yml
