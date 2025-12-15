# C API

The C API documentation is auto-generated from header files using
[Doxygen](https://www.doxygen.nl/).
You can view the generated documentation at
[C API Reference](https://groonga.org/docs/reference/api/index.html).

## How to contribute

To contribute to the C API documentation, add Doxygen-style comments
to the header files in `include/groonga/`.

Example:

```c
/**
 * \brief Short description of the function
 *
 * Detailed description here.
 *
 * \param ctx The context
 * \param arg Description of the argument
 * \return Description of the return value
 */
GRN_API grn_rc grn_function(grn_ctx *ctx, int arg);
```

### Generate and preview

For instructions on how to generate documentation and preview it,
see {doc}`introduction`.

The generated C API HTML documentation will be output to
`../groonga.doc/doc/en/html/reference/api/`.
