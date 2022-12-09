ARG FROM=centos:7
FROM ${FROM}

ARG DEBUG
ARG APACHE_ARROW_VERSION

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  yum install -y https://packages.groonga.org/centos/7/groonga-release-latest.noarch.rpm && \
  yum update -y ${quiet} && \
  yum install -y ${quiet} epel-release && \
  yum install -y ${quiet} centos-release-scl && \
  yum groupinstall -y ${quiet} "Development Tools" && \
  yum remove -y ${quiet} gcc gcc-c++ && \
  yum install -y ${quiet} \
    arrow-devel-${APACHE_ARROW_VERSION} \
    ccache \
    devtoolset-8 \
    intltool \
    libcurl-devel \
    libedit-devel \
    libevent-devel \
    libstemmer-devel \
    libzstd-devel \
    lz4-devel \
    mecab \
    mecab-devel \
    msgpack-devel \
    openssl-devel \
    pcre-devel \
    php-devel \
    pkgconfig \
    python2-devel \
    ruby \
    tar \
    which \
    xxhash-devel \
    zeromq3-devel \
    zlib-devel && \
  yum clean ${quiet} all && \
  source /opt/rh/devtoolset-8/enable
