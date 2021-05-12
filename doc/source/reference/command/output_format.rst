.. -*- rst -*-

Output format
=============

Summary
-------

Commands output their result as JSON, MessagePack, XML or TSV format.

JSON and MessagePack output have the same structure. XML and TSV are
their original structure.

JSON or MessagePack is recommend format. XML is useful for visual
result check. TSV is just for special use. Normally you doesn't need
to use TSV.

JSON and MessagePack
--------------------

This secsion describes the structure of command result on JSON and
MessagePack format. JSON is used to show structure because MessagePack
is binary format. Binary format isn't proper for documenataion.

JSON and MessagePack uses the following structure::

  [HEADER, BODY]

For example::

  [
    [
      0,
      1337566253.89858,
      0.000355720520019531
    ],
    [
      [
        [
          1
        ],
        [
          [
            "_id",
            "UInt32"
          ],
          [
            "_key",
            "ShortText"
          ],
          [
            "content",
            "Text"
          ],
          [
            "n_likes",
            "UInt32"
          ]
        ],
        [
          2,
          "Groonga",
          "I started to use groonga. It's very fast!",
          10
        ]
      ]
    ]
  ]

In the example, the following part is ``HEADER``::

  [
    0,
    1337566253.89858,
    0.000355720520019531
  ]

The following part is ``BODY``::

  [
    [
      [
        1
      ],
      [
        [
          "_id",
          "UInt32"
        ],
        [
          "_key",
          "ShortText"
        ],
        [
          "content",
          "Text"
        ],
        [
          "n_likes",
          "UInt32"
        ]
      ],
      [
        2,
        "Groonga",
        "I started to use groonga. It's very fast!",
        10
      ]
    ]
  ]

``HEADER``
^^^^^^^^^^

``HEADER`` is an array. The content of ``HEADER`` has some patterns.

Success case
++++++++++++

``HEADER`` has three elements on success::

  [0, UNIX_TIME_WHEN_COMMAND_IS_STARTED, ELAPSED_TIME]

The first element is always ``0``.

``UNIX_TIME_WHEN_COMMAND_IS_STARTED`` is the number of seconds
since 1970-01-01 00:00:00 UTC when the command is started
processing. ``ELAPSED_TIME`` is the elapsed time for processing the
command in seconds. Both ``UNIX_TIME_WHEN_COMMAND_IS_STARTED`` and
``ELAPSED_TIME`` are float value. The precision of them are
nanosecond.

Error case
++++++++++

``HEADER`` has four or five elements on error::

  [
    RETURN_CODE,
    UNIX_TIME_WHEN_COMMAND_IS_STARTED,
    ELAPSED_TIME,
    ERROR_MESSAGE,
    ERROR_LOCATION
  ]

``ERROR_LOCATION`` may not be included in ``HEADER`` but other four
elements are always included.

``RETURN_CODE`` is non 0 value. See :doc:`return_code` about available
return codes.

``UNIX_TIME_WHEN_COMMAND_IS_STARTED`` and ``ELAPSED_TIME`` are the
same as success case.

``ERROR_MESSAGE`` is an error message in string.

``ERROR_LOCATION`` is optional. If error location is collected,
``ERROR_LOCATION`` is included. ``ERROR_LOCATION`` is an
array. ``ERROR_LOCATION`` has one ore two elements::

  [
    LOCATION_IN_GROONGA,
    LOCATION_IN_INPUT
  ]

``LOCATION_IN_GROONGA`` is the source location that error is occurred
in groonga. It is useful for groonga developers but not useful for
users. ``LOCATION_IN_GROONGA`` is an array. ``LOCATION_IN_GROONGA`` has
three elements::

  [
    FUNCTION_NAME,
    SOURCE_FILE_NAME,
    LINE_NUMBER
  ]

``FUNCTION_NAME`` is the name of function that error is occurred.

``SOURCE_FILE_NAME`` is the name of groonga's source file that error is
occurred.

``LINE_NUMBER`` is the line number of ``SOURCE_FILE_NAME`` that error
is occurred.

``LOCATION_IN_INPUT`` is optional. ``LOCATION_IN_INPUT`` is included
when the location that error is occurred in input file is
collected. Input file can be specified by ``--file`` command line
option for ``groonga`` command. ``LOCATION_IN_GROONGA`` is an
array. ``LOCATION_IN_GROONGA`` has three elements::

  [
    INPUT_FILE_NAME,
    LINE_NUMBER,
    LINE_CONTENT
  ]

``INPUT_FILE_NAME`` is the input file name that error is occurred.

``LINE_NUMBER`` is the line number of ``INPUT_FILE_NAME`` that error
is occurred.

``LINE_CONTENT`` is the content at ``LINE_NUMBER`` in
``INPUT_FILE_NAME``.

``BODY``
^^^^^^^^

``BODY`` content depends on the executed command. It may be omitted.

``BODY`` may be an error message on error case.

XML
---

TODO

TSV
---

TODO

See also
--------

* :doc:`return_code` describes about return code.

