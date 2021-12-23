.. -*- rst -*-

.. groonga-command
.. database: commands_thread_dump

.. groonga-command
.. thread_dump

``thread_dump``
===============

Summary
-------

.. versionadded:: 11.1.1

.. note::

   Currently, this command works only on Windows.

``thread_dump`` has the following a features:

  * ``thread_dump`` puts a backtrace of all threads into a log as logs of
    NOTICE level at the time of running this command as below.

    For example, ``thread_dump`` puts a backtrace in a log as below.

    .. code-block::

       2021-12-22 11:10:33.518000|n| -- Thread 00008052 --
       2021-12-22 11:10:33.582000|n| (unknown):0:0: ZwGetContextThread(): <ntdll>: <C:\Windows\SYSTEM32\ntdll.dll>
       2021-12-22 11:10:33.591000|n| (unknown):0:0: ??_C@_0BA@PIIKBMGH@grn_thread_dump@(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.591000|n| (unknown):0:0: ??_C@_0CC@GMBIJECC@D?3?2a?2groonga?2groonga?2lib?2thread@(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.591000|n| (unknown):0:0: (unknown)(): <(unknown)>: <(unknown)>
       2021-12-22 11:10:33.591000|n| (unknown):0:0: (unknown)(): <(unknown)>: <(unknown)>
       2021-12-22 11:10:33.591000|n| (unknown):0:0: (unknown)(): <(unknown)>: <(unknown)>
       2021-12-22 11:10:33.591000|n| (unknown):0:0: ??_C@_0BG@NHAMHJPM@?9?9?9?9?9?9?9?9?9?9?9?9?9?9?9?9?9?9?9?9?9@(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.592000|n| D:\a\groonga\groonga\lib\thread.c:148:0: grn_thread_dump(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.593000|n| D:\a\groonga\groonga\lib\proc\proc_thread.c:87:0: command_thread_dump(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.593000|n| D:\a\groonga\groonga\lib\expr.c:1624:0: grn_proc_call(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.594000|n| D:\a\groonga\groonga\lib\command.c:193:0: grn_command_run(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.594000|n| D:\a\groonga\groonga\lib\expr.c:1660:0: grn_expr_exec(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.594000|n| D:\a\groonga\groonga\lib\ctx.c:1716:0: grn_ctx_qe_exec(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.597000|n| D:\a\groonga\groonga\lib\ctx.c:1828:0: grn_ctx_send(): <libgroonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\libgroonga.dll>
       2021-12-22 11:10:33.597000|n| D:\a\groonga\groonga\src\groonga.c:574:0: do_alone(): <groonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\groonga.exe>
       2021-12-22 11:10:33.597000|n| D:\a\groonga\groonga\src\groonga.c:4652:22: main(): <groonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\groonga.exe>
       2021-12-22 11:10:33.601000|n| d:\a01\_work\6\s\src\vctools\crt\vcstartup\src\startup\exe_common.inl:288:34: __scrt_common_main_seh(): <groonga>: <C:\Users\aaa\groonga-11.1.1-8988854-x64-vs2019-with-vcruntime\bin\groonga.exe>
       2021-12-22 11:10:33.601000|n| (unknown):0:0: BaseThreadInitThunk(): <KERNEL32>: <C:\Windows\System32\KERNEL32.DLL>
       2021-12-22 11:10:33.601000|n| (unknown):0:0: RtlUserThreadStart(): <ntdll>: <C:\Windows\SYSTEM32\ntdll.dll>
       2021-12-22 11:10:33.603000|n| ---------------------
       2021-12-22 11:10:33.603000|n| -- Thread 00007860 --
       2021-12-22 11:10:33.607000|n| (unknown):0:0: NtWaitForWorkViaWorkerFactory(): <ntdll>: <C:\Windows\SYSTEM32\ntdll.dll>
       2021-12-22 11:10:33.610000|n| (unknown):0:0: TpReleaseCleanupGroupMembers(): <ntdll>: <C:\Windows\SYSTEM32\ntdll.dll>
       2021-12-22 11:10:33.610000|n| (unknown):0:0: BaseThreadInitThunk(): <KERNEL32>: <C:\Windows\System32\KERNEL32.DLL>
       2021-12-22 11:10:33.610000|n| (unknown):0:0: RtlUserThreadStart(): <ntdll>: <C:\Windows\SYSTEM32\ntdll.dll>
       2021-12-22 11:10:33.611000|n| ---------------------

Syntax
------

This command has not parameter::

  thread_dump

Usage
-----

We can get a backtrace of all threads into a log as logs of
NOTICE level at the time of running this command.

.. groonga-command
.. include:: ../../example/reference/commands/thread_dump/usage.log
.. thread_dump

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

The command returns `true` as body on success such as::

  [HEADER, true]

If ``thread_dump`` fails, error details are in HEADER.

See :doc:`/reference/command/output_format` for ``HEADER``.
