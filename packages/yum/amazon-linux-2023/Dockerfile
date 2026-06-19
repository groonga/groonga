ARG FROM=amazonlinux:2023
FROM ${FROM}

ARG DEBUG
ARG APACHE_ARROW_VERSION

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} \
    https://apache.jfrog.io/artifactory/arrow/amazon-linux/$(cut -d: -f6 /etc/system-release-cpe | cut -d. -f1)/apache-arrow-release-latest.rpm && \
  dnf groupinstall -y ${quiet} "Development Tools" && \
  dnf install -y ${quiet} \
    arrow-compute-devel-${APACHE_ARROW_VERSION} \
    blas-devel \
    cmake \
    curl-devel \
    intltool \
    lapack-devel \
    libedit-devel \
    libevent-devel \
    libstemmer-devel \
    libzstd-devel \
    lz4-devel \
    openssl-devel \
    pkgconfig \
    ruby \
    tar \
    which \
    xxhash-devel \
    zlib-devel && \
  dnf clean ${quiet} all
