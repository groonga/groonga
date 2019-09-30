ARG FROM=centos:6
FROM ${FROM}

ARG DEBUG

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  yum update -y ${quiet} && \
  yum install -y ${quiet} epel-release centos-release-scl && \
  yum install -y https://packages.groonga.org/centos/groonga-release-latest.noarch.rpm && \
  yum install -y http://iij.dl.osdn.jp/storage/g/c/cu/cutter/centos/cutter-release-1.3.0-1.noarch.rpm && \
  yum update -y ${quiet} && \
  yum groupinstall -y ${quiet} "Development Tools" && \
  yum install -y ${quiet} \
    cutter \
    devtoolset-6-gcc-c++ \
    gcc-c++ \
    git \
    intltool \
    libedit-devel \
    libevent-devel \
    libzstd-devel \
    lz4-devel \
    mecab \
    mecab-devel \
    mecab-ipadic \
    openssl-devel \
    pcre-devel \
    php-devel \
    pkgconfig \
    python2-devel \
    scl-utils \
    tar \
    zeromq3-devel \
    zlib-devel && \
  yum clean ${quiet} all

RUN \
  git clone --depth 1 https://github.com/rbenv/rbenv.git ${HOME}/.rbenv && \
  export PATH="${HOME}/.rbenv/bin:${PATH}" && \
  mkdir -p "$(rbenv root)"/plugins && \
  git clone --depth 1 https://github.com/rbenv/ruby-build.git "$(rbenv root)/plugins/ruby-build" && \
  CONFIGURE_OPTS="--disable-install-rdoc" rbenv install -v 2.6.5 && \
  rbenv global 2.6.5
