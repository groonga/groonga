ARG FROM=almalinux:8
FROM ${FROM}

ARG DEBUG
ARG APACHE_ARROW_VERSION

RUN \
  quiet=$([ "${DEBUG}" = "yes" ] || echo "--quiet") && \
  dnf update -y ${quiet} && \
  dnf install -y ${quiet} \
    epel-release \
    'dnf-command(config-manager)' \
    https://packages.groonga.org/almalinux/8/groonga-release-latest.noarch.rpm && \
  dnf config-manager --set-enabled powertools && \
  dnf groupinstall -y ${quiet} "Development Tools" && \
  dnf install -y ${quiet} \
    # Normally, both arrow-devel and arrow-libs are installed automatically as dependency of groonga-devel.
    # However, when installed this way, the latest version(24.0.0) is also installed even if we pin groonga-devel to 14.0.6.
    # We want to restrict their versions to 16.1.0 to ensure compatibility.
    # We can install them at version 16.1.0 by pinning them to 16.1.0 when the packages are installed.
    arrow-devel-16.1.0-1.el8.x86_64 \
    arrow16-libs-16.1.0-1.el8.x86_64 \
    ccache \
    cmake \
    gcc-c++ \
    intltool \
    libedit-devel \
    libevent-devel \
    libstemmer-devel \
    libzstd-devel \
    lz4-devel \
    mecab-devel \
    msgpack-devel \
    openssl-devel \
    pcre-devel \
    php-devel \
    pkgconfig \
    python2-devel \
    rapidjson-devel \
    ruby \
    tar \
    xxhash-devel \
    zlib-devel && \
  dnf clean ${quiet} all
