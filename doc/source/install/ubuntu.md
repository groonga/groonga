# Ubuntu

This section describes how to install Groonga related deb packages on
Ubuntu. You can install them by `apt`.

We distribute both 32-bit and 64-bit packages but we strongly
recommend a 64-bit package for server. You should use a 32-bit package
just only for tests or development. You will encounter an out of
memory error with a 32-bit package even if you just process medium
size data.

## PPA (Personal Package Archive)

The Groonga APT repository for Ubuntu uses PPA (Personal Package
Archive) on Launchpad. You can install Groonga by APT from the PPA.

Here are supported Ubuntu versions:

  * 20.04 LTS Focal Fossa
  * 22.04 LTS Jammy Jellyfish
  * 23.04 Lunar Lobster

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

Install:

```bash
sudo apt -V -y install groonga
```

```{include} server-use.md
```

If you want to use [MeCab](https://taku910.github.io/mecab/) as a
tokenizer, install `groonga-tokenizer-mecab` package.

Install `groonga-tokenizer-mecab` package:

```bash
sudo apt -V -y install groonga-tokenizer-mecab
```

If you want to use `TokenFilterStem` as a token filter, install
`groonga-token-filter-stem` package.

Install groonga-token-filter-stem package:

```bash
sudo apt -V -y install groonga-token-filter-stem
```

There is a package that provides [Munin](http://munin-monitoring.org/)
plugins. If you want to monitor Groonga status by Munin, install
`groonga-munin-plugins` package.

Install `groonga-munin-plugins` package:

```bash
sudo apt -V -y install groonga-munin-plugins
```

There is a package that provides MySQL compatible normalizer as a
Groonga plugin.  If you want to use that one, install
`groonga-normalizer-mysql` package.

Install `groonga-normalizer-mysql` package:

```bash
sudo apt -V -y install groonga-normalizer-mysql
```

## Build from source

Build from source is for developers.

See {doc}`/install/cmake` .
