ARG FROM=centos:8
FROM ${FROM}

ARG DEBUG
ARG APACHE_ARROW_VERSION
ARG APACHE_ARROW_NEXT_VERSION

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} \
    epel-release \
    'dnf-command(config-manager)' \
    https://packages.groonga.org/centos/8/groonga-release-latest.noarch.rpm && \
  dnf config-manager --set-enabled powertools && \
  dnf groupinstall -y ${quiet} "Development Tools" && \
  dnf install -y ${quiet} \
    arrow-devel-${APACHE_ARROW_VERSION} \
    ccache \
    gcc-c++ \
    intltool \
    libedit-devel \
    libevent-devel \
    libstemmer-devel \
    libzstd-devel \
    lz4-devel \
    mecab-devel \
    msgpack-devel \
    openssl-devel \
    pcre-devel \
    php-devel \
    pkgconfig \
    python2-devel \
    ruby \
    tar \
    xxhash-devel \
    zlib-devel && \
  dnf clean ${quiet} all
