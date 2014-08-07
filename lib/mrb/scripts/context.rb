module Groonga
  class Context
    class RC
      attr_reader :name
      def initialize(name, code)
        @name = name
        @code = code
      end

      def to_i
        @code
      end

      SUCCESS                             = new("success", 0)
      END_OF_DATA                         = new("end-of-data", 1)
      UNKNOWN_ERROR                       = new("unknown-error", -1)
      OPERATION_NOT_PERMITTED             = new("operation-not-permitted", -2)
      NO_SUCH_FILE_OR_DIRECTORY           = new("no-such-file-or-directory", -3)
      NO_SUCH_PROCESS                     = new("no-such-process", -4)
      INTERRUPTED_FUNCTION_CALL           = new("interrupted-function-call", -5)
      INPUT_OUTPUT_ERROR                  = new("input-output-error", -6)
      NO_SUCH_DEVICE_OR_ADDRESS           = new("no-such-device-or-address", -7)
      ARG_LIST_TOO_LONG                   = new("arg-list-too-long", -8)
      EXEC_FORMAT_ERROR                   = new("exec-format-error", -9)
      BAD_FILE_DESCRIPTOR                 = new("bad-file-descriptor", -10)
      NO_CHILD_PROCESSES                  = new("no-child-processes", -11)
      RESOURCE_TEMPORARILY_UNAVAILABLE    = new("resource-temporarily-unavailable", -12)
      NOT_ENOUGH_SPACE                    = new("not-enough-space", -13)
      PERMISSION_DENIED                   = new("permission-denied", -14)
      BAD_ADDRESS                         = new("bad-address", -15)
      RESOURCE_BUSY                       = new("resource-busy", -16)
      FILE_EXISTS                         = new("file-exists", -17)
      IMPROPER_LINK                       = new("improper-link", -18)
      NO_SUCH_DEVICE                      = new("no-such-device", -19)
      NOT_A_DIRECTORY                     = new("not-a-directory", -20)
      IS_A_DIRECTORY                      = new("is-a-directory", -21)
      INVALID_ARGUMENT                    = new("invalid-argument", -22)
      TOO_MANY_OPEN_FILES_IN_SYSTEM       = new("too-many-open-files-in-system", -23)
      TOO_MANY_OPEN_FILES                 = new("too-many-open-files", -24)
      INAPPROPRIATE_I_O_CONTROL_OPERATION = new("inappropriate-i-o-control-operation", -25)
      FILE_TOO_LARGE                      = new("file-too-large", -26)
      NO_SPACE_LEFT_ON_DEVICE             = new("no-space-left-on-device", -27)
      INVALID_SEEK                        = new("invalid-seek", -28)
      READ_ONLY_FILE_SYSTEM               = new("read-only-file-system", -29)
      TOO_MANY_LINKS                      = new("too-many-links", -30)
      BROKEN_PIPE                         = new("broken-pipe", -31)
      DOMAIN_ERROR                        = new("domain-error", -32)
      RESULT_TOO_LARGE                    = new("result-too-large", -33)
      RESOURCE_DEADLOCK_AVOIDED           = new("resource-deadlock-avoided", -34)
      NO_MEMORY_AVAILABLE                 = new("no-memory-available", -35)
      FILENAME_TOO_LONG                   = new("filename-too-long", -36)
      NO_LOCKS_AVAILABLE                  = new("no-locks-available", -37)
      FUNCTION_NOT_IMPLEMENTED            = new("function-not-implemented", -38)
      DIRECTORY_NOT_EMPTY                 = new("directory-not-empty", -39)
      ILLEGAL_BYTE_SEQUENCE               = new("illegal-byte-sequence", -40)
      SOCKET_NOT_INITIALIZED              = new("socket-not-initialized", -41)
      OPERATION_WOULD_BLOCK               = new("operation-would-block", -42)
      ADDRESS_IS_NOT_AVAILABLE            = new("address-is-not-available", -43)
      NETWORK_IS_DOWN                     = new("network-is-down", -44)
      NO_BUFFER                           = new("no-buffer", -45)
      SOCKET_IS_ALREADY_CONNECTED         = new("socket-is-already-connected", -46)
      SOCKET_IS_NOT_CONNECTED             = new("socket-is-not-connected", -47)
      SOCKET_IS_ALREADY_SHUTDOWNED        = new("socket-is-already-shutdowned", -48)
      OPERATION_TIMEOUT                   = new("operation-timeout", -49)
      CONNECTION_REFUSED                  = new("connection-refused", -50)
      RANGE_ERROR                         = new("range-error", -51)
      TOKENIZER_ERROR                     = new("tokenizer-error", -52)
      FILE_CORRUPT                        = new("file-corrupt", -53)
      INVALID_FORMAT                      = new("invalid-format", -54)
      OBJECT_CORRUPT                      = new("object-corrupt", -55)
      TOO_MANY_SYMBOLIC_LINKS             = new("too-many-symbolic-links", -56)
      NOT_SOCKET                          = new("not-socket", -57)
      OPERATION_NOT_SUPPORTED             = new("operation-not-supported", -58)
      ADDRESS_IS_IN_USE                   = new("address-is-in-use", -59)
      ZLIB_ERROR                          = new("zlib-error", -60)
      LZO_ERROR                           = new("lzo-error", -61)
      STACK_OVER_FLOW                     = new("stack-over-flow", -62)
      SYNTAX_ERROR                        = new("syntax-error", -63)
      RETRY_MAX                           = new("retry-max", -64)
      INCOMPATIBLE_FILE_FORMAT            = new("incompatible-file-format", -65)
      UPDATE_NOT_ALLOWED                  = new("update-not-allowed", -66)
      TOO_SMALL_OFFSET                    = new("too-small-offset", -67)
      TOO_LARGE_OFFSET                    = new("too-large-offset", -68)
      TOO_SMALL_LIMIT                     = new("too-small-limit", -69)
      CAS_ERROR                           = new("cas-error", -70)
      UNSUPPORTED_COMMAND_VERSION         = new("unsupported-command-version", -71)
      NORMALIZER_ERROR                    = new("normalizer-error", -72)
    end

    class ErrorLevel
      attr_reader :name
      def initialize(name, level)
        @name  = name
        @level = level
      end

      def to_i
        @level
      end

      EMERGENCY = new("emergency", 1)
      ALERT     = new("alert",     2)
      CRITICAL  = new("critical",  3)
      ERROR     = new("error",     4)
      WARNING   = new("warning",   5)
    end
  end
end
