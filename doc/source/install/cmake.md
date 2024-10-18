# Others: Build with CMake

This document describes how to build Groonga from source with CMake.

## Install depended software

Here is depended software for GNU/Linux, UNIX and Windows.

### GNU/Linux or UNIX

#### Debian GNU/Linux, Ubuntu

Execute `setup.sh` included in the source code.

If `groonga-14.0.9.tar.gz` is expanded, it is `groonga-14.0.9/setup.sh`.

#### Amazon Linux 2023

You can run the following commands to install the required packages.

```console
$ sudo dnf groupinstall -y "Development Tools"
$ sudo dnf install -y \
    cmake \
    intltool \
    libedit-devel \
    libevent-devel \
    libstemmer-devel \
    libzstd-devel \
    lz4-devel \
    openssl-devel \
    pkgconfig \
    ruby \
    tar \
    wget \
    which \
    xxhash-devel \
    zlib-devel
```

You can install Apache Arrow following to [the official installation procedure](https://arrow.apache.org/install/).

#### AlmaLinux 9

You can run the following commands to install the required packages.

```console
$ sudo dnf install -y epel-release 'dnf-command(config-manager)'
$ sudo dnf config-manager --set-enabled crb
$ sudo dnf groupinstall -y "Development Tools"
$ sudo dnf install -y \
    gcc-toolset-12 \
    ccache \
    cmake \
    intltool \
    mecab-devel \
    libedit-devel \
    libevent-devel \
    libstemmer-devel \
    libzstd-devel \
    lz4-devel \
    msgpack-devel \
    openssl-devel \
    php-devel \
    pkgconfig \
    ruby \
    simdjson-devel \
    tar \
    wget \
    which \
    xxhash-devel \
    zlib-devel
```

You can install Apache Arrow following to [the official installation procedure](https://arrow.apache.org/install/).

#### macOS

Install [Xcode](https://developer.apple.com/xcode/).

Execute `setup.sh` included in the source code.

If `groonga-14.0.9.tar.gz` is expanded, it is `groonga-14.0.9/setup.sh`.

### Windows

> - [Microsoft Visual Studio Community](https://visualstudio.microsoft.com/vs/community/)

> - [CMake](http://www.cmake.org/)

## Download source

You can get the latest source from [packages.groonga.org](https://packages.groonga.org/source/groonga).

### GNU/Linux or UNIX

```console
$ wget https://packages.groonga.org/source/groonga/groonga-14.0.9.tar.gz
$ tar xvzf groonga-14.0.9.tar.gz
```

### Windows

Download the latest zipped source from packages.groonga.org.

> - https://packages.groonga.org/source/groonga/groonga-14.0.9.zip

Then extract it.

(cmake-run)=

## Run `cmake`

You need to generate build files such as `Makefile` for your environment.

You can custom your build configuration by passing options to `cmake`.

Command example for GNU/Linux or UNIX.

```console
$ cmake -S <Groonga source code directory path> -B <Build directory path> --preset=release-maximum
```

- `-S` option

  - Specify the path of the Groonga source code directory

  - Specify the directory from which you downloaded and extracted the files

- `-B` option

  - Specify the directory to be used for build

  - Specify a build-only directory outside of Groonga's source code directory

(cmake-presets)=

### CMake presets

Using CMake version 3.21.0 or higher, some presets for various build configurations are provided. We have provided a combination of frequently used CMake options, so you can basically use this presets. Use CMake options only if you want to make custom settings. You can get a list of the available presets using `cmake --list-presets` .

```console
$ cmake --list-presets
Available configure presets:

  "debug-default"                   - Optional features may not be enabled (debug build)
  "release-default"                 - Optional features may not be enabled (release build)
  "release-with-debug-info-default" - Optional features may not be enabled (release build with debug info)
  "debug-maximum"                   - Enable all features (debug build)
  "release-maximum"                 - Enable all features (release build)
  "release-with-debug-info-maximum" - Enable all features (release build with debug info)
  "doc"                             - For documentation
  "memory-debug"                    - For memory debug
  "benchmark"                       - For benchmark
```

(cmake-options)=

### CMake options

This section describes important options of CMake.

#### `-G GENERATOR`

Specify a generator.

The default is depending on the system.

You can check the default generator and available generators by `cmake --help`.

```console
$ cmake --help
...
The following generators are available on this platform (* marks default):
  Green Hills MULTI            = Generates Green Hills MULTI files
                                (experimental, work-in-progress).
* Unix Makefiles               = Generates standard UNIX makefiles.
  Ninja                        = Generates build.ninja files.
  Ninja Multi-Config           = Generates build-<Config>.ninja files.
  Watcom WMake                 = Generates Watcom WMake makefiles.
  CodeBlocks - Ninja           = Generates CodeBlocks project files.
  CodeBlocks - Unix Makefiles  = Generates CodeBlocks project files.
  CodeLite - Ninja             = Generates CodeLite project files.
  CodeLite - Unix Makefiles    = Generates CodeLite project files.
  Eclipse CDT4 - Ninja         = Generates Eclipse CDT 4.0 project files.
  Eclipse CDT4 - Unix Makefiles= Generates Eclipse CDT 4.0 project files.
  Kate - Ninja                 = Generates Kate project files.
  Kate - Unix Makefiles        = Generates Kate project files.
  Sublime Text 2 - Ninja       = Generates Sublime Text 2 project files.
  Sublime Text 2 - Unix Makefiles
```

Here is an example how to specify `Unix Makefiles` on GNU/Linux or UNIX.

```console
$ cmake . -G "Unix Makefiles"
```

Here is an example how to specify `Visual Studio 17 2022 x64` as a generator on Windows.
You can specify a target platform name (architecture) with the `-A` option.

```pwsh-session
> cmake . -G "Visual Studio 17 2022" -A x64
```

#### `-DCMAKE_INSTALL_PREFIX`

Specify a directory to install Groonga.

The default is depending on the system, e.g. `/usr/local` or `C:/Program Files/groonga`.

Here is an example how to specify `/tmp/local/` as an install directory on GNU/Linux or UNIX.

```console
$ cmake . -DCMAKE_INSTALL_PREFIX="/tmp/local/"
```

Here is an example how to specify `C:\Groonga` as an install directory on Windows.

```pwsh-session
> cmake . -DCMAKE_INSTALL_PREFIX="C:\Groonga"
```

#### `-DGRN_WITH_MRUBY`

Enables mruby support.

You can use the {doc}`/reference/sharding` plugin and {doc}`/reference/commands/ruby_eval`
with the mruby support.

The default is `OFF`.

Groonga builds bundled mruby if the mruby support is enabled. In order to build mruby, you must
install some required libraries. See the [mruby compile guide](https://github.com/mruby/mruby/blob/master/doc/guides/compile.md)
for more details.

Here is an example how to enable the mruby support.

```console
$ cmake . -DGRN_WITH_MRUBY=ON
```

#### `-DGRN_WITH_DEBUG`

Enables debug options for C/C++ compiler. It's useful for debugging on debugger such as GDB and LLDB.

The default is `OFF`.

Here is an example how to enable debug options.

```console
$ cmake . -DGRN_WITH_DEBUG=ON
```

#### `-DGRN_WITH_APACHE_ARROW`

Enables Apache Arrow support.

In addition to using Apache Arrow IPC streaming format output, you can also use multithreading processing that is used in {ref}`select-n-workers`
and {doc}`/reference/functions/query_parallel_or` with the Apache Arrow support.

The default is `OFF`.

You can install Apache Arrow following to [the official installation procedure](https://arrow.apache.org/install/).

Here is an example how to enable the Apache Arrow support.

```console
$ cmake . -DGRN_WITH_APACHE_ARROW=ON
```

```{note}

If you install Apache Arrow manually, you need to use the {ref}`cmake-options-cmake-prefix-path` option.

```

(cmake-options-cmake-prefix-path)=

#### `-DCMAKE_PREFIX_PATH=PATHS`

Adds search paths for `.cmake` files.

You can specify multiple path separating them with `:` on GNU/Linux or UNIX, `;` on Windows.

In case of using libraries installed via a package manager, you do not need to specify this
parameter. It is because `.cmake` files for those libraries are in the default search paths of CMake.

In case of using libraries installed in non-system directories such as `/usr`, you need to specify `.cmake` file paths of those libraries by this parameter.

Here is an example how to specify a `.cmake` file path for `/tmp/local/lib/cmake/Arrow/ArrowConfig.cmake` on GNU/Linux or UNIX.

```console
$ cmake . -DCMAKE_PREFIX_PATH="/tmp/local"
```

Here is an example how to specify a `.cmake` file path for `C:\arrow\lib\cmake\Arrow\ArrowConfig.cmake` on Windows.

```pwsh-session
> cmake . -DCMAKE_PREFIX_PATH="C:\arrow"
```

(cmake-build-and-install)=

## Build and install Groonga

Now, you can build Groonga.

### GNU/Linux or UNIX

Here is a command line to build and install Groonga.

```console
$ cmake --build -B <Build directory path>
$ sudo cmake --install -B <Build directory path>
```

### Windows

Here is how it is to do it when Visual Studio is specified as the generator, like `-G "Visual Studio 17 2022"` .

You can use Visual Studio or `cmake --build`.

Here is a command line to build and install Groonga by `cmake --build`.

```pwsh-session
> cmake --build . --config Release
> cmake --build . --config Release --target Install
```

You should specify `--config Debug` instead of `--config Release` when debugging.
