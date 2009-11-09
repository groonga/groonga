# -*- coding: utf-8 -*-
#
# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

module GroongaConstants
  module Result
    SUCCESS = 0
    END_OF_DATA = 1
    UNKNOWN_ERROR = -1
    OPERATION_NOT_PERMITTED = -2
    NO_SUCH_FILE_OR_DIRECTORY = -3
    NO_SUCH_PROCESS = -4
    INTERRUPTED_FUNCTION_CALL = -5
    INPUT_OUTPUT_ERROR = -6
    NO_SUCH_DEVICE_OR_ADDRESS = -7
    ARG_LIST_TOO_LONG = -8
    EXEC_FORMAT_ERROR = -9
    BAD_FILE_DESCRIPTOR = -10
    NO_CHILD_PROCESSES = -11
    RESOURCE_TEMPORARILY_UNAVAILABLE = -12
    NOT_ENOUGH_SPACE = -13
    PERMISSION_DENIED = -14
    BAD_ADDRESS = -15
    RESOURCE_BUSY = -16
    FILE_EXISTS = -17
    IMPROPER_LINK = -18
    NO_SUCH_DEVICE = -19
    NOT_A_DIRECTORY = -20
    IS_A_DIRECTORY = -21
    INVALID_ARGUMENT = -22
    TOO_MANY_OPEN_FILES_IN_SYSTEM = -23
    TOO_MANY_OPEN_FILES = -24
    INAPPROPRIATE_I_O_CONTROL_OPERATION = -25
    FILE_TOO_LARGE = -26
    NO_SPACE_LEFT_ON_DEVICE = -27
    INVALID_SEEK = -28
    READ_ONLY_FILE_SYSTEM = -29
    TOO_MANY_LINKS = -30
    BROKEN_PIPE = -31
    DOMAIN_ERROR = -32
    RESULT_TOO_LARGE = -33
    RESOURCE_DEADLOCK_AVOIDED = -34
    NO_MEMORY_AVAILABLE = -35
    FILENAME_TOO_LONG = -36
    NO_LOCKS_AVAILABLE = -37
    FUNCTION_NOT_IMPLEMENTED = -38
    DIRECTORY_NOT_EMPTY = -39
    ILLEGAL_BYTE_SEQUENCE = -40
    SOCKET_NOT_INITIALIZED = -41
    OPERATION_WOULD_BLOCK = -42
    ADDRESS_IS_NOT_AVAILABLE = -43
    NETWORK_IS_DOWN = -44
    NO_BUFFER = -45
    SOCKET_IS_ALREADY_CONNECTED = -46
    SOCKET_IS_NOT_CONNECTED = -47
    SOCKET_IS_ALREADY_SHUTDOWNED = -48
    OPERATION_TIMEOUT = -49
    CONNECTION_REFUSED = -50
    RANGE_ERROR = -51
    TOKENIZER_ERROR = -52
    FILE_CORRUPT = -53
    INVALID_FORMAT = -54
    OBJECT_CORRUPT = -55
    TOO_MANY_SYMBOLIC_LINKS = -56
    NOT_SOCKET = -57
    OPERATION_NOT_SUPPORTED = -58
    ADDRESS_IS_IN_USE = -59
    ZLIB_ERROR = -60
    LZO_ERROR = -61
    STACK_OVER_FLOW = -62
    SYNTAX_ERROR = -63
    RETRY_MAX = -64
    INCOMPATIBLE_FILE_FORMAT = -65
  end

  module Table
    HASH_KEY = 0x0
    PAT_KEY = 0x1
    NO_KEY = 0x3
    VIEW = 0x04
  end

  module Key
    WITH_SIS = 0x40
    NORMALIZE = 0x80
  end

  module Column
    SCALAR = 0x0
    VECTOR = 0x1
    INDEX = 0x2
  end

  module Flag
    WITH_SECTION = 0x80
    WITH_WEIGHT = 0x100
    WITH_POSITION = 0x200
  end
end
