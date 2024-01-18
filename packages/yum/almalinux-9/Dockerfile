ARG FROM=almalinux:9
FROM ${FROM}

ENV \
  SCL=gcc-toolset-12

ARG DEBUG
ARG APACHE_ARROW_VERSION

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} \
    epel-release \
    'dnf-command(config-manager)' \
    https://packages.groonga.org/almalinux/9/groonga-release-latest.noarch.rpm \
    https://apache.jfrog.io/artifactory/arrow/almalinux/$(cut -d: -f5 /etc/system-release-cpe | cut -d. -f1)/apache-arrow-release-latest.rpm && \
  dnf config-manager --set-enabled crb && \
  dnf groupinstall -y ${quiet} "Development Tools" && \
  dnf install -y ${quiet} \
    ${SCL} \
    arrow-devel-${APACHE_ARROW_VERSION} \
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
    pcre-devel \
    php-devel \
    pkgconfig \
    ruby \
    tar \
    which \
    xsimd-devel \
    xxhash-devel \
    zlib-devel && \
  dnf clean ${quiet} all
