# Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# We use the Node.js image because we run groonga.wasm by Node.js. The
# WebAssembly exception handling proposal is enabled by default since
# Node.js 24.
ARG NODE_VERSION
FROM node:${NODE_VERSION}-trixie

RUN \
  quiet="-o=Dpkg::Use-Pty=0" && \
  apt update ${quiet} -o="APT::Acquire::Retries=3" && \
  apt install -y -V ${quiet} -o="APT::Acquire::Retries=3" \
    ccache \
    cmake \
    ninja-build && \
  apt clean && \
  rm -rf /var/lib/apt/lists/*

# The C++ standard library that supports C++ exceptions is provided
# since wasi-sdk 33. The LLVM based toolchains in Debian and Ubuntu
# don't provide it.
ARG WASI_SDK_VERSION
RUN \
  case $(arch) in \
    x86_64) wasi_sdk_arch=x86_64 ;; \
    aarch64) wasi_sdk_arch=arm64 ;; \
    *) echo "unsupported architecture: $(arch)"; exit 1 ;; \
  esac && \
  base_name=wasi-sdk-${WASI_SDK_VERSION}.0-${wasi_sdk_arch}-linux && \
  curl \
    --fail \
    --location \
    --silent \
    --show-error \
    https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/${base_name}.tar.gz | \
    tar xzf - -C /opt && \
  mv /opt/${base_name} /opt/wasi-sdk

ENV WASI_SDK_PATH=/opt/wasi-sdk

USER node
WORKDIR /home/node
