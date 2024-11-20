# Copyright(C) 2024  Horimoto Yasuhiro <horimoto@clear-code.com>
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

require_relative "blog-task"

class GroongaBlogTask < BlogTask
  def initialize
    super("groonga")
  end

  def document_repository_path
    env_var("GROONGA_ORG_DIR")
  end

  def detect_version
    version_env = ENV["VERSION"]
    return version_env if version_env

    base_version_file = File.join(__dir__,
                                  "base_version")
    version = File.read(base_version_file).chomp
  end
end
