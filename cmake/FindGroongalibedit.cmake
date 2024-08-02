# Copyright(C) 2024  Shizuo Fujita <fujita@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

find_package(PkgConfig)
if(PkgConfig_FOUND)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
    pkg_check_modules(Groongalibedit_pkg_libedit IMPORTED_TARGET
                      "libedit${pkg_check_modules_version}")
  else()
    pkg_check_modules(Groongalibedit_pkg_libedit IMPORTED_TARGET GLOBAL
                      "libedit${pkg_check_modules_version}")
  endif()
  set(Groongalibedit_FOUND ${Groongalibedit_pkg_libedit_FOUND})
  if(Groongalibedit_FOUND)
    set(GRN_LIBEDIT_TARGET PkgConfig::Groongalibedit_pkg_libedit)
  endif()
endif()
