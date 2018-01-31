FROM debian

RUN dpkg --add-architecture i386
RUN apt update && \
    apt install -V -y \
      build-essential \
      devscripts \
      autoconf \
      libtool \
      cmake \
      pkg-config \
      mingw-w64 \
      wine-binfmt \
      wine32 \
      wine64 \
      rsync \
      ruby && \
   apt clean

RUN gem install rake

ARG WORK_UID=${WORK_UID}
ARG WORK_GID=${WORK_GID}
ARG WORK_USER=${WORK_USER}
ARG WORK_GROUP=${WORK_GROUP}

RUN groupadd \
      --gid ${WORK_GID} \
      ${WORK_GROUP}
RUN useradd \
      --uid ${WORK_UID} \
      --gid ${WORK_GID} \
      --create-home \
      ${WORK_USER}

WORKDIR /home/${WORK_USER}

CMD ["/build/build.sh"]
