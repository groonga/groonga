.. -*- rst -*-

Log
===

Groonga has two log files. They are process log and query
log. Process log is for all of :doc:`executables/groonga`
works. Query log is just for query processing.

.. _process-log:

Process log
-----------

Process log is enabled by default. Log path can be customized by
:option:`groonga --log-path` option. Each log has its log level. If a
log is smaller than Groonga process' log level, it's not logged. Log
level can be customized by :option:`groonga -l` or
:doc:`commands/log_level`.

Format
^^^^^^

Process log uses the following format::

  #{TIME_STAMP}|#{L}| #{MESSAGE}

Some multi-process based applications such as
:doc:`/reference/executables/groonga-httpd` and `PGroonga
<https://pgroonga.github.io/>`_ use the following format::

  #{TIME_STAMP}|#{L}|#{PID}: #{MESSAGE}

TIME_STAMP
  It's time stamp uses the following format::

    YYYY-MM-DD hh:mm:ss.SSSSSS

  YYYY
    Year with four digits.

  MM
    Month with two digits.

  DD
    Day with two digits.

  hh
    Hour with two digits.

  mm
    Minute with two digits.

  ss
    Second with two digits.

  SSSSSS
    Microsecond with six digits.

  Example::

    2011-07-05 06:25:18.345734

L
  Log level with a character. Here is a character and log
  level map.

  E
    Emergency

  A
    Alert

  C
    Critical

  e
    Error

  w
    Warning

  n
    Notification

  i
    Information

  d
    Debug

  \-
    Dump

  Example::

    E

PID
  The process ID.

  Example::

    1129

MESSAGE
  Details about the log with free format.

  Example::

    log opened.

Example::

  2011-07-05 08:35:09.276421|n| grn_init
  2011-07-05 08:35:09.276553|n| RLIMIT_NOFILE(4096,4096)

Example with PID::

  2011-07-05 08:35:09.276421|n|1129: grn_init
  2011-07-05 08:35:09.276553|n|1129: RLIMIT_NOFILE(4096,4096)

.. _query-log:

Query log
---------

Query log is disabled by default. It can be enabled by
:option:`groonga --query-log-path` option.

Format
^^^^^^

Query log uses the following formats::

  #{TIME_STAMP}|#{MESSAGE}
  #{TIME_STAMP}|#{ID}|>#{QUERY}
  #{TIME_STAMP}|#{ID}|:#{ELAPSED_TIME} #{PROGRESS}
  #{TIME_STAMP}|#{ID}|<#{ELAPSED_TIME} #{RETURN_CODE}

TIME_STAMP
  It's time stamp uses the following format::

    YYYY-MM-DD hh:mm:ss.SSSSSS

  YYYY
    Year with four digits.

  MM
    Month with two digits.

  DD
    Day with two digits.

  hh
    Hour with two digits.

  mm
    Minute with two digits.

  ss
    Second with two digits.

  SSSSSS
    Microsecond with six digits.

  Example::

    2011-07-05 06:25:18.345734

ID
  ID of a thread. Groonga process creates threads to process
  requests concurrently. Each thread outputs some logs for a
  request. This ID can be used to extract a log sequence by
  a thread.

  Example::

    45ea3034

>
  A character that indicates query is started.

:
  A character that indicates query is processing.

<
  A character that indicates query is finished.

MESSAGE
  Details about the log with free format.

  Example::

    query log opened.

QUERY
  A query to be processed.

  Example::

    select users --match_columns hobby --query music

ELAPSED_TIME
  Elapsed time in nanoseconds since query is started.

  Example::

    000000000075770
    (It means 75,770 nanoseconds.)

PROGRESS
  A processed work at the time.

  Example::

    select(313401)
    (It means that 'select' is processed and 313,401 records are remained.)

RETURN_CODE
  A return code for the query.

  Example::

    rc=0
    (It means return code is 0. 0 means GRN_SUCCESS.)

Example::

  2011-07-05 06:25:19.458756|45ea3034|>select Properties --limit 0
  2011-07-05 06:25:19.458829|45ea3034|:000000000072779 select(19)
  2011-07-05 06:25:19.458856|45ea3034|:000000000099998 output(0)
  2011-07-05 06:25:19.458875|45ea3034|<000000000119062 rc=0
  2011-07-05 06:25:19.458986|45ea3034|>quit
