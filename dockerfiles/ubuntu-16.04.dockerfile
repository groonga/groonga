FROM ubuntu:16.04

RUN \
  echo "debconf debconf/frontend select Noninteractive" | \
    debconf-set-selections

RUN \
  echo 'APT::Install-Recommends "false";' > \
    /etc/apt/apt.conf.d/disable-install-recommends

RUN \
  apt update -qq && \
  apt install -y \
    bison \
    g++ \
    gcc \
    gdb \
    git \
    libevent-dev \
    liblz4-dev \
    libmecab-dev \
    libmsgpack-dev \
    libssl-dev \
    libstemmer-dev \
    libzmq-dev \
    libzstd-dev \
    make \
    mecab-naist-jdic \
    pkg-config \
    rapidjson-dev \
    rsync \
    ruby \
    ruby-dev \
    sudo \
    tzdata \
    zlib1g-dev

RUN \
  apt update -qq && \
  apt install -qq -y software-properties-common && \
  add-apt-repository -y ppa:cutter-testing-framework/ppa && \
  apt update -qq && \
  apt install -qq -y cutter-testing-framework

RUN \
  apt update -qq && \
  apt install -qq -y \
    lsb-release \
    wget && \
  wget https://apache.bintray.com/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-archive-keyring-latest-$(lsb_release --codename --short).deb && \
  apt install -y -V ./apache-arrow-archive-keyring-latest-$(lsb_release --codename --short).deb && \
  apt update -qq && \
  apt install -qq -y libarrow-dev

RUN \
  gem install \
    bundler \
    grntest \
    groonga-client \
    pkg-config \
    rake \
#    red-arrow

RUN \
  useradd --user-group --create-home groonga

RUN \
  echo "groonga ALL=(ALL:ALL) NOPASSWD:ALL" | \
    EDITOR=tee visudo -f /etc/sudoers.d/groonga

USER groonga

RUN mkdir -p /home/groonga/build
WORKDIR /home/groonga/build

COPY run.sh /home/groonga/build/
RUN sudo chown groonga: run.sh

CMD ./run.sh
