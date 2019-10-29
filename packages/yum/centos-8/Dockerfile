ARG FROM=centos:8
FROM ${FROM}

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  yum install -y https://packages.groonga.org/centos/groonga-release-latest.noarch.rpm && \
  yum update -y ${quiet} && \
  yum install -y ${quiet} epel-release yum-utils && \
  yum config-manager --set-enabled PowerTools && \
  yum groupinstall -y ${quiet} "Development Tools" && \
  yum install -y ${quiet} \
    gcc-c++ \
    intltool \
    libedit-devel \
    libevent-devel \
    libstemmer-devel \
    libzstd-devel \
    lz4-devel \
    openssl-devel \
    pcre-devel \
    php-devel \
    pkgconfig \
    python2-devel \
    ruby \
    tar \
    zlib-devel && \
  yum clean ${quiet} all
