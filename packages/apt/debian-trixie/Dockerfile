ARG FROM=debian:trixie
FROM ${FROM}

RUN \
  echo "debconf debconf/frontend select Noninteractive" | \
    debconf-set-selections

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "-qq") && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    ca-certificates \
    lsb-release \
    wget && \
  wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
  apt install -y -V ${quiet} ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
  rm apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
  wget https://packages.groonga.org/debian/groonga-apt-source-latest-$(lsb_release --codename --short).deb && \
  apt install -y -V ${quiet} ./groonga-apt-source-latest-$(lsb_release --codename --short).deb && \
  rm groonga-apt-source-latest-$(lsb_release --codename --short).deb && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    build-essential \
    ccache \
    cmake \
    debhelper \
    devscripts \
    libarrow-compute-dev \
    libarrow-dev \
    libblas-dev \
    libcurl4-openssl-dev \
    libedit-dev \
    libevent-dev \
    liblapack-dev \
    liblz4-dev \
    libmecab-dev \
    libmsgpack-dev \
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
    zlib1g-dev

RUN \
  if [ $(arch) != "x86_64" ]; then \
    echo "DEBUILD_LINTIAN=no" >> /etc/devscripts.conf; \
  fi
