# Migration from PPA (Personal Package Archive) to Groonga APT Repository (packages.groonga.org)

## Changing the Package Source

This document guides you through the migration from the deprecated PPA to our
new Groonga APT Repository(packages.groonga.org).

By switching to the APT Repository, you'll receive packages built with Apache
Arrow enabled. Which unlocks extra features such as parallel offline index
building. We strongly encourage you to migrate to enjoy these enhancements.

If you are currently using the old PPA (ppa:groonga/ppa), please follow the
steps below to switch to the new package source.

### Register Groonga APT Repository (packages.groonga.org)

Groonga packages are distributed via our Groonga APT repository at
https://packages.groonga.org.

To enable the Groonga APT repository, install the `groonga-apt-source package`
as follows:

```bash
sudo apt update
sudo apt install -y -V ca-certificates lsb-release wget
wget https://packages.groonga.org/ubuntu/groonga-apt-source-latest-$(lsb_release --codename --short).deb
sudo apt install -y -V ./groonga-apt-source-latest-$(lsb_release --codename --short).deb
sudo apt update
```

### Remove the Existing Package Source

Remove the old Groonga PPA:

```bash
sudo add-apt-repository --remove ppa:groonga/ppa
```

After these steps, your package source is now switched from PPA
(Personal Package Archive) to APT Repository (packages.groonga.org).

## Package Installation

There are two approaches depending on your needs.

### Option 1: Use the New Package Version at Groonga APT Repository (Recommended)

Starting with the next release, we strongly recommend using the Groonga APT
Repository. When you install Groonga via APT, you will automatically receive the
latest package version without having to specify a version explicitly as follows.
This option is simpler and is recommended for most users:

```bash
sudo apt install groonga
```

### Option 2: Use the Existing Package Version at Groonga APT Repository

If you wish to continue using the same package version as before, you must
ensure that the package versions are aligned. This is important because even if
the version numbers match, the naming conventions between the PPA and the
Groonga APT repository differ, and the PPA package may take precedence.

For example, to force the installation of a specific package version from the
new repository:

```bash
sudo apt install groonga
```

However, if you wish to continue using the same package version as before, you
must ensure that the package versions are aligned. This is important because
even if the version numbers match, the package naming conventions differ between
the PPA and the Groonga APT repository. In such cases, the PPA package take
precedence.

For example:

```bash
sudo apt install -V groonga=15.0.3-1
```

This command forces the installation of a specific package version from the new
repository. In this case, it's Groonga APT repository.
