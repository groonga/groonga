ARG FROM=almalinux:8
FROM ${FROM}

ARG DEBUG
ARG APACHE_ARROW_VERSION

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} \
    epel-release \
    'dnf-command(config-manager)' \
    https://packages.groonga.org/almalinux/8/groonga-release-latest.noarch.rpm \
    https://packages.apache.org/artifactory/arrow/almalinux/$(cut -d: -f5 /etc/system-release-cpe | cut -d. -f1)/apache-arrow-release-latest.rpm && \
  dnf config-manager --set-enabled powertools && \
  dnf groupinstall -y ${quiet} "Development Tools" && \
  dnf install -y ${quiet} \
    arrow-compute-devel-${APACHE_ARROW_VERSION} \
    ccache \
    cmake \
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
    pkgconfig \
    python2-devel \
    rapidjson-devel \
    ruby \
    simdjson-devel \
    tar \
    xxhash-devel \
    zlib-devel && \
  dnf clean ${quiet} all
