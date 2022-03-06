ARG FROM=amazonlinux:2
FROM ${FROM}

ARG DEBUG
ARG APACHE_ARROW_VERSION

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  amazon-linux-extras install -y ${quiet} epel && \
  yum install -y ${quiet} ca-certificates && \
  yum install -y ${quiet} \
    https://packages.groonga.org/amazon-linux/2/groonga-release-latest.noarch.rpm && \
  yum update -y ${quiet} && \
  yum groupinstall -y ${quiet} "Development Tools" && \
  yum install -y ${quiet} \
    arrow-devel-${APACHE_ARROW_VERSION} \
    ccache \
    gcc-c++ \
    intltool \
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
  yum clean ${quiet} all
