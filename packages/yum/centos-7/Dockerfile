ARG FROM=centos:7
FROM ${FROM}

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  yum install -y https://packages.groonga.org/centos/7/groonga-release-latest.noarch.rpm && \
  yum update -y ${quiet} && \
  yum install -y ${quiet} epel-release && \
  yum groupinstall -y ${quiet} "Development Tools" && \
  yum install -y ${quiet} \
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
    rapidjson-devel \
    ruby \
    tar \
    xxhash-devel \
    zeromq3-devel \
    zlib-devel && \
  yum clean ${quiet} all
