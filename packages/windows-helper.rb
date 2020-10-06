# Copyright(C) 2020  Horimoto Yasuhiro <horimoto@clear-code.com>
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

module WindowsHelper
  private
  def enable_windows?
    true
  end

  def windows_dir
    "windows"
  end

  def windows_targets
    return [] unless enable_windows?

    targets = (ENV["WINDOWS_TARGETS"] || "").split(",")
    return targets unless targets.empty?

    windows_targets_default
  end

  def windows_targets_default
    [
      "x86",
      "x64",
      "x86-exe",
      "x64-exe",
      "x86-vs2019",
      "x86-vs2019-with-vcruntime",
      "x64-vs2019",
      "x64-vs2019-with-vcruntime",
      "x86-vs2017",
      "x86-vs2017-with-vcruntime",
      "x64-vs2017",
      "x64-vs2017-with-vcruntime",
    ]
  end
end
