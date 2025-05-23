# Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>
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

FROM archlinux

RUN \
  pacman --sync --noconfirm --refresh --sysupgrade && \
  pacman --sync --noconfirm \
    binutils \
    ccache \
    debugedit \
    # mecab-git must have this but it doesn't have it.
    # So we install this in base environment.
    diffutils \
    fakeroot \
    # mecab-git must have this but it doesn't have it.
    # So we install this in base environment.
    gcc \
    git \
    # mecab-git must have this but it doesn't have it.
    # So we install this in base environment.
    make \
    sudo

RUN \
  useradd --user-group --create-home groonga

RUN \
  echo "groonga ALL=(ALL:ALL) NOPASSWD:ALL" | \
    EDITOR=tee visudo -f /etc/sudoers.d/groonga

USER groonga
WORKDIR /home/groonga

CMD /source/ci/arch-linux/build.sh
