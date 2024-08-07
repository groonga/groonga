ARG FROM=ubuntu:focal
FROM ${FROM}

RUN \
  echo "debconf debconf/frontend select Noninteractive" | \
    debconf-set-selections

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "-qq") && \
  apt update ${quiet} && \
  apt install -y -V --no-install-recommends ${quiet} \
    ca-certificates \
    gpg-agent \
    lsb-release \
    software-properties-common \
    wget && \
  wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
  apt install -y -V ${quiet} ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
  rm apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
  add-apt-repository -y ppa:groonga/ppa && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    autoconf-archive \
    build-essential \
    ccache \
    cmake \
    debhelper \
    devscripts \
    dh-apparmor \
    libarrow-dev \
    libedit-dev \
    libevent-dev \
    liblz4-dev \
    libmecab-dev \
    libmsgpack-dev \
    libjemalloc-dev \
    libpcre3-dev \
    libsimdjson-dev \
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
