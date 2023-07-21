ARG FROM=debian:bullseye
FROM ${FROM}

RUN \
  echo "debconf debconf/frontend select Noninteractive" | \
    debconf-set-selections

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "-qq") && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    wget && \
  wget https://packages.groonga.org/debian/groonga-apt-source-latest-bullseye.deb && \
  apt install -y -V ${quiet} ./groonga-apt-source-latest-bullseye.deb && \
  rm ./groonga-apt-source-latest-bullseye.deb && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    build-essential \
    ccache \
    cmake \
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
    libthrift-dev \
    libxxhash-dev \
    libzmq3-dev \
    libzstd-dev \
    ninja-build \
    pkg-config \
    rapidjson-dev \
    zlib1g-dev && \
  if apt list | grep '^libarrow-dev/'; then \
    apt install -y -V ${quiet} libarrow-dev; \
  fi && \
  apt clean
