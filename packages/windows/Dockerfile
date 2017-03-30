FROM debian

RUN dpkg --add-architecture i386
RUN apt update
RUN apt install -V -y \
    build-essential \
    devscripts \
    autoconf \
    libtool \
    cmake \
    pkg-config \
    mingw-w64 \
    wine \
    wine-binfmt \
    rsync \
    ruby

RUN gem install rake

CMD ["/build/build.sh"]
