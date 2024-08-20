ARG FROM=ubuntu:focal
FROM ${FROM}

RUN \
  echo "debconf debconf/frontend select Noninteractive" | \
    debconf-set-selections

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "-qq") && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    autoconf-archive \
    build-essential \
    ccache \
    cmake \
    debhelper \
    devscripts \
    dh-apparmor \
    libedit-dev \
    libevent-dev \
    liblz4-dev \
    libmecab-dev \
    libmsgpack-dev \
    libjemalloc-dev \
    libpcre3-dev \
    libssl-dev \
    libstemmer-dev \
    libthrift-dev \
    libxxhash-dev \
    libzmq3-dev \
    libzstd-dev \
    lsb-release \
    ninja-build \
    pkg-config \
    rapidjson-dev \
    zlib1g-dev && \
  apt clean
