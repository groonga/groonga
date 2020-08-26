ARG FROM=centos:8
FROM ${FROM}

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} epel-release 'dnf-command(config-manager)' && \
  dnf config-manager --set-enabled PowerTools && \
  dnf groupinstall -y ${quiet} "Development Tools" && \
  dnf install -y ${quiet} \
    gcc-c++ \
    intltool \
    libedit-devel \
    libevent-devel \
    libstemmer-devel \
    libzstd-devel \
    lz4-devel \
    msgpack-devel \
    openssl-devel \
    pcre-devel \
    php-devel \
    pkgconfig \
    python2-devel \
    ruby \
    tar \
    zlib-devel && \
  dnf install -y ${quiet} https://packages.groonga.org/centos/8/groonga-release-latest.noarch.rpm && \
  dnf install -y ${quiet} mecab-devel && \
  dnf clean ${quiet} all
