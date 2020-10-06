ARG FROM=centos:6
FROM ${FROM}

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  yum install -y https://packages.groonga.org/centos/6/groonga-release-latest.noarch.rpm && \
  yum update -y ${quiet} && \
  yum install -y ${quiet} epel-release centos-release-scl && \
  yum groupinstall -y ${quiet} "Development Tools" && \
  yum install -y ${quiet} \
    devtoolset-6-gcc-c++ \
    gcc-c++ \
    intltool \
    libedit-devel \
    libevent-devel \
    libzstd-devel \
    lz4-devel \
    mecab \
    mecab-devel \
    openssl-devel \
    pcre-devel \
    php-devel \
    pkgconfig \
    python2-devel \
    ruby \
    scl-utils \
    tar \
    xxhash-devel \
    zeromq3-devel \
    zlib-devel && \
  yum clean ${quiet} all
