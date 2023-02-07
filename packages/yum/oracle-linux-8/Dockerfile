ARG FROM=oraclelinux:8
FROM ${FROM}

ARG DEBUG
ARG APACHE_ARROW_VERSION

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} \
    epel-release \
    'dnf-command(config-manager)' \
    https://packages.groonga.org/oracle-linux/8/groonga-release-latest.noarch.rpm \
    https://apache.jfrog.io/artifactory/arrow/almalinux/$(cut -d: -f5 /etc/system-release-cpe | cut -d. -f1)/apache-arrow-release-latest.rpm && \
  dnf groupinstall -y ${quiet} "Development Tools" && \
  dnf install --enablerepo=ol8_codeready_builder -y ${quiet} \
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
