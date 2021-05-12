.. -*- rst -*-

How to avoid mmap Cannot allocate memory error
==============================================

Example
-------

There is a case following mmap error in log file:

  2013-06-04 08:19:34.835218|A|4e86e700|mmap(4194304,551,432017408)=Cannot allocate memory <13036498944>

Note that <13036498944> means total size of mmap (almost 12GB) in this case.

Solution
--------

So you need to confirm following point of views.

* Are there enough free memory?
* Are maximum number of mappings exceeded?

To check there are enough free memory, you can use vmstat command.

To check whether maximum number of mappings are exceeded, you can investigate the value of vm.max_map_count.

If this issue is fixed by modifying the value of vm.max_map_count, it's exactly the reason.

As groonga allocates memory chunks each 256KB, you can estimate the size of database you can handle by following formula:

  (database size) = vm.max_map_count * (memory chunks)

If you want to handle over 16GB groonga database, you must specify at least 65536 as the value of vm.max_map_count:

  database size (16GB) = vm.max_map_count (65536) * memory chunks (256KB)

You can modify vm.max_map_count temporary by sudo sysctl -w vm.max_map_count=65536.

Then save the configuration value to ``/etc/sysctl.conf`` or ``/etc/sysctl.d/*.conf``.

See :doc:`/reference/tuning` documentation about tuning related parameters.


