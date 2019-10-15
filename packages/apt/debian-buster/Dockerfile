ARG FROM=debian:buster
FROM ${FROM}

RUN \
  echo "debconf debconf/frontend select Noninteractive" | \
    debconf-set-selections

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "-qq") && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    autotools-dev \
    build-essential \
    debhelper \
    devscripts \
    lsb-release \
    libevent-dev \
    liblz4-dev \
    libmecab-dev \
    libmsgpack-dev \
    libjemalloc-dev \
    libpcre3-dev \
    libssl-dev \
    libstemmer-dev \
    libzmq3-dev \
    libzstd-dev \
    pkg-config \
    rapidjson-dev \
    zlib1g-dev && \
  apt clean && \
  rm -rf /var/lib/apt/lists/*
