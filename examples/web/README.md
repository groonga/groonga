# Groonga on a Web browser

This example downloads a Groonga database by HTTP, puts it into an
in-memory file system and runs commands by `libgroonga.wasm`.

A Web browser doesn't provide WASI nor a file system. So
`lib/libgroonga-wasi.mjs` implements the WASI functions that
`libgroonga.wasm` needs with an in-memory file system.

## How to run

Build Groonga for WASI and install it:

```console
$ cmake -S . -B ../groonga.build --preset=release-wasi
$ cmake --build ../groonga.build
$ cmake --install ../groonga.build --prefix /tmp/local
```

`serve.mjs` serves `/lib/` from the installed Groonga. So we don't
need to copy `libgroonga.wasm` into the Groonga source.

Create a database in `examples/web/database/`. A file of it is
downloaded when it's used at first:

```console
$ mkdir -p examples/web/database
$ /tmp/local/bin/groonga.mjs -n examples/web/database/db \
    "table_create Memos TABLE_HASH_KEY ShortText"
```

Start the HTTP server and open the printed URL:

```console
$ node examples/web/serve.mjs --prefix /tmp/local
http://localhost:8080/
```

## Limitations

The file system is in memory. A change isn't saved. The database is
downloaded again when you reload the page.

Groonga opens a database for writing even when you only read it. So
the in-memory file system must be writable. This is why we can't map
the downloaded files read-only.
