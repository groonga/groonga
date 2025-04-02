:orphan:

# Migration from PPA (Personal Package Archive) to Groonga APT Repository (packages.groonga.org)

This document guides you through the migration from the deprecated PPA (Personal
Package Archive) to our new Groonga APT Repository (packages.groonga.org).

By switching to the APT Repository, you'll receive packages built with Apache
Arrow enabled. Which unlocks extra features such as parallel offline index
building. We strongly recommend you to migrate to enjoy these enhancements.

If you are currently using the old PPA (ppa:groonga/ppa), please follow the
steps below to switch to the new package source.

## Configure the Groonga APT Repository

### Register Groonga APT Repository (packages.groonga.org)

To register the Groonga APT repository, install the `groonga-apt-source package`
as follows.

```bash
sudo apt update
sudo apt install -y -V groonga-apt-source
sudo apt update
```

### Remove the Existing Package Source

Remove the old Groonga PPA (ppa:groonga/ppa):

```bash
sudo add-apt-repository -y --remove ppa:groonga/ppa
sudo apt update
```

After these steps, your package source is now switched from PPA
(ppa:groonga/ppa) to APT Repository (packages.groonga.org).

## Package Installation

There are two approaches depending on your needs.

### Option 1: Use Groonga APT Repository to Install the Next Package Version (Recommended)

Starting with the next package version, we strongly recommend using the Groonga
APT Repository. When you install Groonga via APT, you will automatically receive
the next package version without having to specify a version explicitly as
follows. This option is simpler and is recommended for most users:

```bash
sudo apt install package-name
```

For example, install `groonga-bin` package:

```bash
sudo apt install groonga-bin
```

### Option 2: Use Groonga APT Repository to Install the Existing Package Version

If you wish to continue using the same package version as before, you must
install the package by specifying the version. This is important because even if
the version numbers match, the package's naming conventions between the PPA and
the Groonga APT repository differ, and the PPA package takes precedence.

```bash
sudo apt install package-name=version
```

```{note}
If the package has dependencies, you must also need to install  dependent
packages by specifying the version.
```

For example, install a specific version of `groonga-bin` package:

```bash
version=$(dpkg-query -W -f='${Version}' libgroonga0 | sed 's/\.ubuntu.*$//') && \
sudo apt install -y -V --allow-downgrades \
  libgroonga0=$version \
  groonga-bin=$version
```

This command forces the installation of a specific package version from the new
repository. In this case, it's Groonga APT repository.
