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
    libreadline-dev \
    libstemmer-dev \
    libzmq3-dev \
    libzstd-dev \
    mecab-ipadic-utf8 \
    pkg-config \
    rapidjson-dev \
    zlib1g-dev \
    wget

RUN : "Enable Cutter repository" && { \
  echo "deb [signed-by=/usr/share/keyrings/cutter-archive-keyring.gpg] http://iij.dl.osdn.jp/storage/g/c/cu/cutter/debian/ stretch main"; \
  echo "deb-src [signed-by=/usr/share/keyrings/cutter-archive-keyring.gpg] http://iij.dl.osdn.jp/storage/g/c/cu/cutter/debian/ stretch main"; \
} | tee /etc/apt/sources.list.d/cutter.list

RUN wget -O /usr/share/keyrings/cutter-archive-keyring.gpg https://cutter.osdn.jp/debian/cutter-archive-keyring.gpg

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "-qq") && \
  apt update ${quiet} && \
  apt install -y -V ${quiet} \
    cutter-testing-framework && \
  apt clean && \
  rm -rf /var/lib/apt/lists/*

RUN \
  git clone --depth 1 https://github.com/sstephenson/rbenv.git ${HOME}/.rbenv && \
  git clone --depth 1 https://github.com/sstephenson/ruby-build.git ${HOME}/.rbenv/plugins/ruby-build && \
  CONFIGURE_OPTS="--disable-install-rdoc" ${HOME}/.rbenv/bin/rbenv install 2.6.5 && \
  ${HOME}/.rbenv/bin/rbenv global 2.6.5
