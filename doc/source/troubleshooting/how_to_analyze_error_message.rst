.. -*- rst -*-

.. highlightlang:: none

How to analyze error messages
=============================

This section describes how to analyze Groonga error messages.


How to analyze socket errors
----------------------------

This subsection describes how to analyze socket errors with an example.


Example
^^^^^^^

The following is an example of an error message reported by Groonga, where xxxxx is an arbitrary number::

  socket error[xxxxx]: no buffer: accept


How to analyze
^^^^^^^^^^^^^^

First, grep Groonga source files for "SOERR" that is the name of a macro for socket errors.

Then, extract SOERRs whose argument contains "accept" from the grep output and you will find the following SOERRs::

  lib/com.c:      SOERR("listen - start accept");
  lib/com.c:      SOERR("listen - disable accept");
  lib/com.c:        SOERR("accept");

It is clear that the above error message is associated with the last line because the error message contains only "accept".

The source code around the line is as follows::

  grn_sock fd = accept(com->fd, NULL, NULL);
  if (fd == -1) {
    if (errno == EMFILE) {
      grn_com_event_stop_accept(ctx, ev);
    } else {
      SOERR("accept");
    }
    return;
  }

From the above source code, you can confirm that the error occurred due to accept.
Let's dig into the cause.

The error message provides hints for investigation::

  [10055]: no buffer

10055 is a Windows socket error code and "no buffer" is a message by Groonga given in SOERR.

Windows socket error codes are listed in the following page::

  https://msdn.microsoft.com/ja-jp/library/windows/desktop/ms740668(v=vs.85).aspx

10055 is assigned to WSAENOBUFS and its description is as follows::

  No buffer space available.

  An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.

From the above description, you can narrow down the causes.
The possible causes are the lack of memory and too many connections.
Finally, determine which one is appropriate for the situation when the error occurred.
