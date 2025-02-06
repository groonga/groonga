<!-- groonga-command -->
<!-- database: n_workers -->

# Parallel execution

## Summary

Groonga executes serially by default.
However, by specifying in the option you can execute in parallel.

The next section shows you how to set up for parallel execution.
Please read the notes before using this option.

## How to use

### Set per Groonga command

Only works for the command to be executed.

#### Specified by `--n_workers` option of Groonga command

Groonga command example:

```
load --table Data --n_workers -1
[
{"_key", "value"}
]
```

### Set the default value

If you set a default value, you do not need to specify it for each Groonga command.
The default value is used for all Groonga commands.

#### Specified by `--default-n-workers` option of `groonga` execution file

Execution example:

```console
$ groonga --default-n-workers -1 DB_PATH status
```

#### Specified by environment variable `GRN_N_WORKERS_DEFAULT`

Execution example:

```console
$ GRN_N_WORKERS_DEFAULT=-1 groonga DB_PATH status
```

### Available Values

You can set the number of parallels.
If you specify `-1` or `2` or more, it will execute in parallel.

```{eval-rst}
.. list-table::
   :header-rows: 1

   * - n_workers
     - Behavior
   * - When specifying ``0`` or ``1``
     - Execute in serial.
   * - When specifying ``2`` or more
     - Execute in parallel with at most the specified number of threads.
   * - When specifying ``-1``
     - Execute in parallel with the threads of at most the number of CPU cores.
```

## Check the settings

You can check it by the value of `n_workers` and `default_n_workers` in the status {doc}`/reference/commands/status` command.

<!-- groonga-command -->

```{include} ../../example/reference/command/n_workers/status.md
status
```

`n_workers` is per Groonga command value. `default_n_workers` is the default value.

## Notes

### Apache Arrow is required

This feature requires that Apache Arrow is enabled in Groonga.

It depends on package provider whether Apache Arrow is enabled or not.

To check whether Apache Arrow is enabled, you can use {doc}`/reference/commands/status` command that show the result of  `apache_arrow` is `true` or not.

### For use as a daemon process

For example, consider using {doc}`/reference/executables/groonga-server-http` on a system with 6 CPUs.

{doc}`/reference/executables/groonga-server-http` allocates 1 thread (= 1CPU) for each request.

When the average number of concurrent connections is 6, there are no free CPU resources because 6 CPUs are already in use.
All the CPU is used to process each request.

When the average number of concurrent connections is 2, there are 4 free CPU resources because only 2 CPUs are already in use.
When specifying `2` for `n_workers`, it uses at most 3 CPUs, including the thread for processing requests.
Therefore, if two requests to Groonga process with `2` specified for `n_workers` are requested at the same time,
they will use at most 6 CPUs in total and will be processed fastly by using all of the resources.
When specifying greater than `2`, the degree of parallelism can be higher than the CPU resources, so it may actually slow down the execution time.

## Parallel execution support

* {ref}`offline-index-construction`
* {doc}`/reference/commands/load`
* {ref}`select <select-n-workers>`
