# Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>
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

module RepositoryHelper
  private
  def repository_rsync_base_path
    "packages@packages.groonga.org:public"
  end

  def repository_gpg_key_ids
    @repository_gpg_key_ids ||= read_repository_gpg_key_ids
  end

  def read_repository_gpg_key_ids
    top_directory = File.join(__dir__, "..")
    [
      File.read(File.join(top_directory, "gpg_uid_rsa4096")).strip,
      File.read(File.join(top_directory, "gpg_uid")).strip,
    ]
  end
end
