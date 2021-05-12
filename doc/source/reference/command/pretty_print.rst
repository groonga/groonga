.. -*- rst -*-

Pretty print
============

Summary
-------

.. versionadded:: 5.1.0

Groonga supports pretty print when you choose JSON for
:doc:`output_format`.

Usage
-----

Just specify ``yes`` to ``output_pretty`` parameter::

  > status --output_pretty yes
  [
    [
      0,
      1448344438.43783,
      5.29289245605469e-05
    ],
    {
      "alloc_count": 233,
      "starttime": 1448344437,
      "start_time": 1448344437,
      "uptime": 1,
      "version": "5.0.9-135-g0763d91",
      "n_queries": 0,
      "cache_hit_rate": 0.0,
      "command_version": 1,
      "default_command_version": 1,
      "max_command_version": 2
    }
  ]

Here is a result without ``output_pretty`` parameter::

  > status
  [[0,1448344438.43783,5.29289245605469e-05],{"alloc_count":233,"starttime":1448344437,...}]
