.. -*- rst -*-

.. program:: groonga-suggest-httpd

.. groonga-command
.. database: groonga-suggest-httpd
.. % killall lt-groonga-suggest-learner || :
.. % killall lt-groonga-suggest-httpd || :

``groonga-suggest-httpd``
=========================

Summary
-------

``groonga-suggest-httpd`` is a program that provides HTTP interface
for the following features:

  * Returning :doc:`/reference/suggest` execution result
  * Saving logs for learning

``groonga-suggest-httpd`` provides suggest feature like
:doc:`/reference/commands/suggest` command. Note that some parameter
names are different of them.

Syntax
------

``groonga-suggest-httpd`` requires database path::

  groonga-suggest-httpd [options] DATABASE_PATH

Usage
-----

You need to create one or more datasets to use
``groonga-suggest-httpd``. A dataset consists of tables and
columns. You can define them by :doc:`groonga-suggest-create-dataset`.

You need to use :doc:`groonga-suggest-learner` to learn suggestion
data from user inputs. You doesn't need to use
:doc:`groonga-suggest-learner` when you create suggestion data by
hand. See :doc:`/reference/suggest` and sub documents about creating
suggestion data by hand.

You can use ``groonga-suggest-httpd`` via HTTP after you create one or
more datasets.

The following sections describes the followings:

  * How to set up a dataset
  * How to use ``groonga-suggest-httpd`` with :doc:`groonga-suggest-learner`
  * How to use ``groonga-suggest-httpd`` for retrieving suggestions.

Setup
^^^^^

You need to create a dataset by :doc:`groonga-suggest-create-dataset`.

Here is an example that creates ``query`` dataset:

.. groonga-command
.. include:: ../../example/reference/executables/groonga-suggest-httpd/setup.log
.. % groonga-suggest-create-dataset ${DB_PATH} query

:doc:`groonga-suggest-create-dataset` outputs executed commands. You
can confirm that what tables and columns are created for the new
dataset.

Launch ``groonga-suggest-learner``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can choose whether you use learned suggestion data immediately or
not.

There are two ways to use learned suggestion data immediately:

  * Both of ``groonga-suggest-httpd`` and
    :doc:`groonga-suggest-learner` use the same database
  * ``groonga-suggest-httpd`` receives learned suggestion data from
    :doc:`groonga-suggest-learner`

In the former case, you must run both ``groonga-suggest-httpd`` and
:doc:`groonga-suggest-learner` on the same host.

In the latter case, you can run ``groonga-suggest-httpd`` and
:doc:`groonga-suggest-learner` on different hosts.

If you don't need to use learned suggestion data immediately, you need
to apply learned suggestion data from database that is used by
:doc:`groonga-suggest-learner` to database that is used by
``groonga-suggest-httpd`` by hand. Normally, this usage is
recommended. Because learned suggestion data may have garbage data by
inputs from evil users.

In this document, learned suggestion data are used immediately by
receiving learned suggestion data from
:doc:`groonga-suggest-learner`. Both ``groonga-suggest-httpd`` and
:doc:`groonga-suggest-learner` are running on the same host. Because
it's easy to explain.

Here is an example that launches :doc:`groonga-suggest-learner`. You
need to specify database that has ``query`` dataset. This document
omits the instruction for creating ``query`` dataset:

.. groonga-command
.. database: groonga-suggest-httpd-learner
.. % groonga-suggest-create-dataset ${DB_PATH} query

.. groonga-command
.. include:: ../../example/reference/executables/groonga-suggest-httpd/launch-lerner.log
.. % groonga-suggest-learner --daemon ${DB_PATH}

The ``groonga-suggest-learner`` process opens two endpoints at
``1234`` port and ``1235`` port:

  * ``1234`` port: Endpoint that accepts user input data from
    ``groonga-suggest-httpd``
  * ``1235`` port: Endpoint that sends learned suggestion data to
    ``groonga-suggest-httpd``

Launch ``groonga-suggest-httpd``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You need to launch ``groonga-suggest-httpd`` for the following proposes:

  * Learning suggestion data from user inputs
  * Providing suggestion result to clients

Here is an example that launches ``groonga-suggest-httpd`` that
communicates :doc:`groonga-suggest-learner`:

.. groonga-command
.. database: groonga-suggest-httpd

.. groonga-command
.. include:: ../../example/reference/executables/groonga-suggest-httpd/launch.log
.. % groonga-suggest-httpd --send-endpoint 'tcp://127.0.0.1:1234' --receive-endpoint 'tcp://127.0.0.1:1235' --daemon ${DB_PATH}

The ``groonga-suggest-httpd`` process accepts HTTP requests on
``8080`` port.

If you want to save requests into log file, use
:option:`--log-base-path` option.

Here is an example to save log files under ``logs`` directory with
``log`` prefix for each file::

  % groonga-suggest-httpd --log-base-path logs/log ${DB_PATH}

``groonga-suggest-httpd`` creates log files such as
``logYYYYmmddHHMMSS-00`` under ``logs`` directory.

Learn from user inputs
^^^^^^^^^^^^^^^^^^^^^^

You can learn suggestion data from user inputs.

You need to specify the following parameters to learn suggestion data:

  * ``i``: The ID of the user (You may use IP address of client)
  * ``l``: The dataset name
  * ``s``: The timestamp of the input in seconds
  * ``t``: The query type (It's optional. You must specify ``submit`` only when the user input is submitted.)
  * ``q``: The user input

Here are example requests to learn user input "Groonga" in ``query``
dataset::

.. groonga-command
.. include:: ../../example/reference/executables/groonga-suggest-httpd/learn.log
.. % curl 'http://localhost:8080/?i=127.0.0.1&l=query&s=92619&q=G'
.. % curl 'http://localhost:8080/?i=127.0.0.1&l=query&s=93850&q=Gr'
.. % curl 'http://localhost:8080/?i=127.0.0.1&l=query&s=94293&q=Gro'
.. % curl 'http://localhost:8080/?i=127.0.0.1&l=query&s=94734&q=Groo'
.. % curl 'http://localhost:8080/?i=127.0.0.1&l=query&s=95147&q=Grooon'
.. % curl 'http://localhost:8080/?i=127.0.0.1&l=query&s=95553&q=Groonga'
.. % curl 'http://localhost:8080/?i=127.0.0.1&l=query&s=95959&t=submit&q=Groonga'

Inputting data must not use ``t=submit`` parameter. In the above example, you just learn user inputs but you can learn and get complete candidates at once. It's described at the next section.

Submitted data must use ``t=submit`` parameter.

Use suggested response
^^^^^^^^^^^^^^^^^^^^^^

You can get suggested result from ``groonga-suggest-httpd``.

You need to specify the following parameters to get suggested result:

  * ``n``: The dataset name
  * ``t``: The query type (``complete``, ``correct`` and/or ``suggest``)
  * ``q``: The user input

You can also specify parameters for :doc:`/reference/commands/suggest` as option.

Here is an example that gets :doc:`/reference/suggest/completion` result. The result is computed by using learned data at the previous section. ``frequency_threshold=1`` parameter is used because this is an example. The parameter enables input data that are occurred one or more times. Normally, you should not use the parameter for production. The parameter will increase noises:

.. groonga-command
.. include:: ../../example/reference/executables/groonga-suggest-httpd/complete.log
.. % curl 'http://localhost:8080/?n=query&t=complete&q=G&frequency_threshold=1'

You can combine completion and learning by specifying parameters for both:

.. groonga-command
.. include:: ../../example/reference/executables/groonga-suggest-httpd/learn-and-complete.log
.. % curl 'http://localhost:8080/?i=127.0.0.1&l=query&s=96000&q=G&n=query&t=complete&frequency_threshold=1'

Command line parameters
-----------------------

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

``DATABASE_PATH``
"""""""""""""""""

Specifies the path to a Groonga database. This database must have one
or more datasets. Each dataset must be created by
:doc:`groonga-suggest-create-dataset`.

Optional parameters
^^^^^^^^^^^^^^^^^^^

.. option:: -p, --port

   Specify HTTP server port number.

   The default port number is ``8080``.

.. option:: -t, --n-threads

   Specify number of threads.

   This option accepts ``128`` as the max value, but use the number of
   CPU cores for performance.

   The default value is the number of CPU cores.

.. option:: -s, --send-endpoint

   Specify endpoint URI of :doc:`groonga-suggest-learner` for sending
   user inputs.

   The format is ``tcp://${HOST}:${PORT}`` such as
   ``tcp://192.168.0.1:2929``.

   The default value is none.

.. option:: -r, --receive-endpoint

   Specify endpoint URI of :doc:`groonga-suggest-learner` for
   receiving learned suggestion data.

   The format is ``tcp://${HOST}:${PORT}`` such as
   ``tcp://192.168.0.1:2929``.

   The default value is none.

.. option:: -l, --log-base-path

   Specify path prefix of log.

   The default value is none.

.. option:: --n-lines-per-log-file

   Specify the max number of lines in a log file.

   The default value is ``1000000``.

.. option:: -d, --daemon

   Specify this option to daemonize.

   Don't daemonize by default.

.. option:: --disable-max-fd-check

   Specify this option to disable checking the max number of file
   descriptors on start.

   Check by default.

GET parameters
--------------

``groonga-suggest-httpd`` accepts some GET parameters.

There are required parameters which depend on query type.

In ``complete``, ``correct`` or ``suggest`` query type, unhandled
parameters are passed through :doc:`/reference/commands/suggest`. It
means that you can use parameters of
:doc:`/reference/commands/suggest`.

.. _groonga-suggest-httpd-required-parameters:

Required parameters
^^^^^^^^^^^^^^^^^^^

You must specify the following parameters.

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Note
   * - ``q``
     - Input by user. It must be UTF-8 encoded string.
     -

.. _groonga-suggest-httpd-required-parameters-learning:

Required parameters for learning
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You must specify the following parameters when you specify
:option:`--send-endpoint`.

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Note
   * - ``s``
     - Elapsed time since ``1970-01-01T00:00:00Z``.
     - The unit is millisecond.
   * - ``i``
     - Unique ID to distinct each user
     - Session ID, IP address and so on will be usable
       for this value.
   * - ``l``
     - One or more learn target dataset names. You need to use ``|``
       as separator such as ``dataset1|dataset2|dataset3``.
     - Dataset name is the name that you specify to
       :doc:`groonga-suggest-create-dataset`.

.. _groonga-suggest-httpd-required-parameters-suggestion:

Required parameters for suggestion
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You must specify the following parameters when you specify one of
``complete``, ``correct`` and ``suggest`` to ``t`` parameter.

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Note
   * - ``n``
     - The dataset name to use computing suggestion result.
     - Dataset name is the name that you specify to
       :doc:`groonga-suggest-create-dataset`.
   * - ``t``
     - The query type.

       Available values are ``complete``, ``correct``, ``suggest``.

     - You can specify multiple types. You need to use ``|`` as
       separator such as ``complete|correct``.

.. _groonga-suggest-httpd-optional-parameters:

Optional parameters
^^^^^^^^^^^^^^^^^^^

Here are optional parameters.

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Note
   * - ``callback``
     - Function name for JSONP
     -

.. _groonga-suggest-httpd-optional-parameters-learning:

Optional parameters for learning
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Here are optional parameters when you specify
:option:`--send-endpoint`.

.. list-table::
   :header-rows: 1

   * - Key
     - Description
     - Note
   * - ``t``
     - The query type.

       Available value is only ``submit``.

     - You must specify ``submit`` when user submits the input
       specified as ``q``.

       You must not specify ``submit`` for user inputs that aren't
       submitted yet. You can use suggestion by specifying
       ``complete``, ``correct`` and/or ``suggest`` to ``t`` when you
       doesn't specify ``submit``. See
       :ref:`groonga-suggest-httpd-required-parameters-suggestion` for
       details about these values.


Return value
------------

``groonga-suggest-httpd`` returns the following format response. It's
the same format as body of :doc:`/reference/commands/suggest`::

  {
    TYPE1: [
      [CANDIDATE_1, SCORE_1],
      [CANDIDATE_2, SCORE_2],
      ...,
      [CANDIDATE_N, SCORE_N]
    ],
    TYPE2: [
      [CANDIDATE_1, SCORE_1],
      [CANDIDATE_2, SCORE_2],
      ...,
      [CANDIDATE_N, SCORE_N]
    ],
    ...
  }

Here is the response when ``t`` is ``submit``::

  {}

``TYPE``
^^^^^^^^

One of ``complete``, ``correct`` and ``suggest``.

``CANDIDATE_N``
^^^^^^^^^^^^^^^

The string of candidate in UTF-8.

``SCORE_N``
^^^^^^^^^^^

The score of the candidate.

Candidates are sorted by score descendant.

See also
--------

  * :doc:`/reference/commands/suggest`

.. groonga-command
.. % killall lt-groonga-suggest-learner || :
.. % killall lt-groonga-suggest-httpd || :
.. % killall -KILL lt-groonga-suggest-learner || :
.. % killall -KILL lt-groonga-suggest-httpd || :
