ARG FROM=almalinux:10
FROM ${FROM}

ARG DEBUG
ARG APACHE_ARROW_VERSION

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} \
    epel-release \
    'dnf-command(config-manager)' \
    https://packages.groonga.org/almalinux/10/groonga-release-latest.noarch.rpm \
    https://packages.apache.org/artifactory/arrow/almalinux/$(cut -d: -f5 /etc/system-release-cpe | cut -d. -f1)/apache-arrow-release-latest.rpm && \
  dnf config-manager --set-enabled crb && \
  dnf groupinstall -y ${quiet} "Development Tools" && \
  dnf install -y ${quiet} \
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
    pkgconfig \
    ruby \
    tar \
    which \
    xxhash-devel \
    zlib-devel && \
  dnf clean ${quiet} all
