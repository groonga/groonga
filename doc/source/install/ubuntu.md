# Ubuntu

This section describes how to install Groonga related deb packages on
Ubuntu. You can install them by `apt`.

We distribute both 32-bit and 64-bit packages but we strongly
recommend a 64-bit package for server. You should use a 32-bit package
just only for tests or development. You will encounter an out of
memory error with a 32-bit package even if you just process medium
size data.

## Register Groonga APT repository

### APT Repository (packages.groonga.org)

Groonga packages are distributed via our Groonga APT repository at
https://packages.groonga.org.

Install `groonga-apt-source` to enable Groonga APT repository.

```bash
sudo apt update
sudo apt install -y -V ca-certificates lsb-release wget
wget https://packages.groonga.org/ubuntu/groonga-apt-source-latest-$(lsb_release --codename --short).deb
sudo apt install -y -V ./groonga-apt-source-latest-$(lsb_release --codename --short).deb
sudo apt update
```

### PPA (Personal Package Archive)

```{note}
The PPA will be deprecated. We strongly recommend using our Groonga APT
repository (packages.groonga.org) because packages from that repository are built
with Apache Arrow enabled. This configuration unlocks extra features, such as
parallel offline index building.
```

The Groonga APT repository for Ubuntu uses PPA (Personal Package
Archive) on Launchpad. You can install Groonga by APT from the PPA.

Here are supported Ubuntu versions:

- 22.04 LTS Jammy Jellyfish
- 24.04 LTS Noble Numbat

Enable the universe repository to install Groonga:

```bash
sudo apt -V -y install software-properties-common
sudo add-apt-repository -y universe
```

Add the `ppa:groonga/ppa` PPA to your system:

```bash
sudo add-apt-repository -y ppa:groonga/ppa
sudo apt update
```

## `groonga` package

Install:

```bash
sudo apt -V -y install groonga
```

```{include} server-use.md

```

## `groonga-tokenizer-mecab` package

If you want to use [MeCab](https://taku910.github.io/mecab/) as a
tokenizer, install `groonga-tokenizer-mecab` package.

Install `groonga-tokenizer-mecab` package:

```bash
sudo apt -V -y install groonga-tokenizer-mecab
```

## `groonga-token-filter-stem` package

If you want to use `TokenFilterStem` as a token filter, install
`groonga-token-filter-stem` package.

Install groonga-token-filter-stem package:

```bash
sudo apt -V -y install groonga-token-filter-stem
```

## `groonga-munin-plugins` package

There is a package that provides [Munin](http://munin-monitoring.org/)
plugins. If you want to monitor Groonga status by Munin, install
`groonga-munin-plugins` package.

Install `groonga-munin-plugins` package:

```bash
sudo apt -V -y install groonga-munin-plugins
```

## `groonga-normalizer-mysql` package

There is a package that provides MySQL compatible normalizer as a
Groonga plugin. If you want to use that one, install
`groonga-normalizer-mysql` package.

Install `groonga-normalizer-mysql` package:

```bash
sudo apt -V -y install groonga-normalizer-mysql
```

## Build from source

Build from source is for developers.

See {doc}`/install/cmake` .

## Migration from Groonga PPA (ppa:groonga/ppa) to Groonga APT Repository (packages.groonga.org)

This section guides you through the migration from the deprecated Groonga PPA
(Personal Package Archive, ppa:groonga/ppa) to our new Groonga APT Repository
(packages.groonga.org).

By switching to the Groonga APT Repository, you'll receive packages built with
Apache Arrow enabled. Which unlocks extra features such as parallel offline
index building. We strongly recommend you to migrate to enjoy these enhancements.

If you are currently using the Groonga PPA, please follow the
steps below to switch to the new package source.

### Configure the Groonga APT Repository

#### Register Groonga APT Repository

To register the Groonga APT Repository, install the `groonga-apt-source` package
as follows.

```bash
sudo apt update
sudo apt install -y -V groonga-apt-source
sudo apt update
```

#### Remove the Existing Package Source

Remove the old Groonga PPA:

```bash
sudo add-apt-repository -y --remove ppa:groonga/ppa
sudo apt update
```

After these steps, your package source is now switched from Groonga PPA to
Groonga APT Repository.

### Upgrade Package

There are two approaches depending on your needs.

#### Option 1: Use Groonga APT Repository from the Next Release (Recommended)

Starting with the next package version, we strongly recommend using the Groonga
APT Repository. When you upgrade Groonga via `apt`, you will automatically receive
the next package version without having to specify a version explicitly as
follows. This option is simpler and is recommended for most users:

```bash
sudo apt upgrade package-name
```

For example, upgrade `groonga-bin` package:

```bash
sudo apt upgrade groonga-bin
```

#### Option 2: Use Groonga APT Repository Immediately Without Waiting for Next Release

If you wish to upgrade immediately without waiting for the next official release,
you can force an upgrade to a specific package version in the Groonga APT
Repository. In this case, you must upgrade the package by specifying the version.
This is important because even if the version numbers match, the package's
naming conventions between the Groonga PPA and the Groonga APT Repository differ,
and the Groonga PPA package takes precedence.

```bash
sudo apt upgrade package-name=version
```

```{note}
If the package has dependencies, you also need to upgrade dependent packages by
specifying the version.
```

For example, upgrade a specific version of `groonga-bin` package:

```bash
version=$(dpkg-query -W -f='${Version}' libgroonga0 | sed 's/\.ubuntu.*$//') && \
sudo apt upgrade -y -V --allow-downgrades \
  libgroonga0=$version \
  groonga-bin=$version
```

This command forces the upgrade to a specific package version from the new
repository. In this case, it's Groonga APT Repository.
