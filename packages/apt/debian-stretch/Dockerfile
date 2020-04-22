ARG FROM=debian:stretch
FROM ${FROM}

RUN \
  echo "debconf debconf/frontend select Noninteractive" | \
    debconf-set-selections

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "-qq") && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    autoconf \
    autoconf-archive \
    automake \
    autotools-dev \
    bison \
    build-essential \
    debhelper \
    devscripts \
    gettext \
    git \
    g++ \
    lsb-release \
    libevent-dev \
    liblz4-dev \
    libmecab-dev \
    libmsgpack-dev \
    libjemalloc-dev \
    libjs-underscore \
    libjs-jquery \
    libpcre3-dev \
    libssl-dev \
    libstemmer-dev \
    libtool \
    libzmq3-dev \
    libzstd-dev \
    make \
    mecab-jumandic-utf8 \
    mecab-naist-jdic \
    munin-node \
    pkg-config \
    rapidjson-dev \
    rake \
    ruby \
    ruby-dev \
    wget \
    zlib1g-dev && \
  apt clean && \
  rm -rf /var/lib/apt/lists/*
