.. -*- rst -*-

.. highlightlang:: none

Return code
===========

Summary
-------

Return code is used to show whether a processing is succeeded or
not. If the processing is not succeeded, return code shows error type.

Return code is used in C API and query API. You can check return code
via ``grn_ctx_t::rc`` in C API. You can check return code by looking
the header element in query API. See :doc:`output_format` about the
header element in query API.

List
----

Here is a list of return codes. ``GRN_SUCCESS`` (= 0) means that the
processing is succeeded. Return codes that have negative value show
error type. ``GRN_END_OF_DATA`` is a special return code. It is used
only C API. It is not showen in query API.

* 0: ``GRN_SUCCESS``
* 1: ``GRN_END_OF_DATA``
* -1: ``GRN_UNKNOWN_ERROR``
* -2: ``GRN_OPERATION_NOT_PERMITTED``
* -3: ``GRN_NO_SUCH_FILE_OR_DIRECTORY``
* -4: ``GRN_NO_SUCH_PROCESS``
* -5: ``GRN_INTERRUPTED_FUNCTION_CALL``
* -6: ``GRN_INPUT_OUTPUT_ERROR``
* -7: ``GRN_NO_SUCH_DEVICE_OR_ADDRESS``
* -8: ``GRN_ARG_LIST_TOO_LONG``
* -9: ``GRN_EXEC_FORMAT_ERROR``
* -10: ``GRN_BAD_FILE_DESCRIPTOR``
* -11: ``GRN_NO_CHILD_PROCESSES``
* -12: ``GRN_RESOURCE_TEMPORARILY_UNAVAILABLE``
* -13: ``GRN_NOT_ENOUGH_SPACE``
* -14: ``GRN_PERMISSION_DENIED``
* -15: ``GRN_BAD_ADDRESS``
* -16: ``GRN_RESOURCE_BUSY``
* -17: ``GRN_FILE_EXISTS``
* -18: ``GRN_IMPROPER_LINK``
* -19: ``GRN_NO_SUCH_DEVICE``
* -20: ``GRN_NOT_A_DIRECTORY``
* -21: ``GRN_IS_A_DIRECTORY``
* -22: ``GRN_INVALID_ARGUMENT``
* -23: ``GRN_TOO_MANY_OPEN_FILES_IN_SYSTEM``
* -24: ``GRN_TOO_MANY_OPEN_FILES``
* -25: ``GRN_INAPPROPRIATE_I_O_CONTROL_OPERATION``
* -26: ``GRN_FILE_TOO_LARGE``
* -27: ``GRN_NO_SPACE_LEFT_ON_DEVICE``
* -28: ``GRN_INVALID_SEEK``
* -29: ``GRN_READ_ONLY_FILE_SYSTEM``
* -30: ``GRN_TOO_MANY_LINKS``
* -31: ``GRN_BROKEN_PIPE``
* -32: ``GRN_DOMAIN_ERROR``
* -33: ``GRN_RESULT_TOO_LARGE``
* -34: ``GRN_RESOURCE_DEADLOCK_AVOIDED``
* -35: ``GRN_NO_MEMORY_AVAILABLE``
* -36: ``GRN_FILENAME_TOO_LONG``
* -37: ``GRN_NO_LOCKS_AVAILABLE``
* -38: ``GRN_FUNCTION_NOT_IMPLEMENTED``
* -39: ``GRN_DIRECTORY_NOT_EMPTY``
* -40: ``GRN_ILLEGAL_BYTE_SEQUENCE``
* -41: ``GRN_SOCKET_NOT_INITIALIZED``
* -42: ``GRN_OPERATION_WOULD_BLOCK``
* -43: ``GRN_ADDRESS_IS_NOT_AVAILABLE``
* -44: ``GRN_NETWORK_IS_DOWN``
* -45: ``GRN_NO_BUFFER``
* -46: ``GRN_SOCKET_IS_ALREADY_CONNECTED``
* -47: ``GRN_SOCKET_IS_NOT_CONNECTED``
* -48: ``GRN_SOCKET_IS_ALREADY_SHUTDOWNED``
* -49: ``GRN_OPERATION_TIMEOUT``
* -50: ``GRN_CONNECTION_REFUSED``
* -51: ``GRN_RANGE_ERROR``
* -52: ``GRN_TOKENIZER_ERROR``
* -53: ``GRN_FILE_CORRUPT``
* -54: ``GRN_INVALID_FORMAT``
* -55: ``GRN_OBJECT_CORRUPT``
* -56: ``GRN_TOO_MANY_SYMBOLIC_LINKS``
* -57: ``GRN_NOT_SOCKET``
* -58: ``GRN_OPERATION_NOT_SUPPORTED``
* -59: ``GRN_ADDRESS_IS_IN_USE``
* -60: ``GRN_ZLIB_ERROR``
* -61: ``GRN_LZO_ERROR``
* -62: ``GRN_STACK_OVER_FLOW``
* -63: ``GRN_SYNTAX_ERROR``
* -64: ``GRN_RETRY_MAX``
* -65: ``GRN_INCOMPATIBLE_FILE_FORMAT``
* -66: ``GRN_UPDATE_NOT_ALLOWED``
* -67: ``GRN_TOO_SMALL_OFFSET``
* -68: ``GRN_TOO_LARGE_OFFSET``
* -69: ``GRN_TOO_SMALL_LIMIT``
* -70: ``GRN_CAS_ERROR``
* -71: ``GRN_UNSUPPORTED_COMMAND_VERSION``
* -72: ``GRN_NORMALIZER_ERROR``
* -73: ``GRN_TOKEN_FILTER_ERROR``
* -74: ``GRN_COMMAND_ERROR``
* -75: ``GRN_PLUGIN_ERROR``
* -76: ``GRN_SCORER_ERROR``
* -77: ``GRN_CANCEL``
* -78: ``GRN_WINDOW_FUNCTION_ERROR``
* -79: ``GRN_ZSTD_ERROR``
* -80: ``GRN_CONNECTION_RESET``

See also
--------

* :doc:`output_format` shows where return code is appeared in query
  API response.
* :doc:`/spec/gqtp`: GQTP protocol also uses return code as status but
  it uses 2byte unsigned integer. So return codes that have negative
  value are statuses that have positive value in GQTP protocol. You
  can convert status value in GQTP protocol to return code by
  handling it as 2byte signed integer.
