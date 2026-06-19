#!/bin/bash

if [ $# -ne 3 ]; then
  echo "Usage: $0 DOCKER_TAG GROONGA_VERSION LOG"
  echo " e.g.: $0 debian:bookworm 13.0.9 /tmp/download/groonga.log"
  exit 1
fi

set -eux

GROONGA_REPOSITORY="$(cd $(dirname $0)/.. && pwd)"
DOCKER_TAG="$1"
GROONGA_VERSION="$2"
LOG="$3"

: ${PGROONGA_VERSION:=}
: ${POSTGRESQL_VERSION:=}

case "${DOCKER_TAG}" in
  centos:*)
    INSTALL_RUBY="yum install -y ruby"
    ;;
  almalinux:*)
    INSTALL_RUBY="dnf install -y ruby"
    ;;
  debian:*|ubuntu:*|arm64v8/debian:*|arm64v8/ubuntu:*)
    INSTALL_RUBY="apt update && apt install -y -V ruby"
    ;;
esac

docker \
  run \
  -it \
  --rm \
  --volume "${GROONGA_REPOSITORY}:/groonga" \
  --volume "$(dirname ${LOG}):/log" \
  ${DOCKER_TAG} \
  bash -c " \
    ${INSTALL_RUBY} && \
    /groonga/tools/parse-backtrace.rb \
      --version ${GROONGA_VERSION} \
      --pgroonga-version ${PGROONGA_VERSION} \
      --postgresql-version ${POSTGRESQL_VERSION} \
      /log/$(basename ${LOG}) \
  "
