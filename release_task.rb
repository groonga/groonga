# Copyright (C) 2024  Kodama Takuya <otegami@clear-code.com>
# Copyright (C) 2024  Horimoto Yasuhiro <horimoto@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

class ReleaseTask
  include Rake::DSL

  def initialize(package)
    @package = package
  end

  def define
    define_generate_announce_task
  end

  private

  def define_generate_announce_task
    namespace :release do
      namespace :blog do
        namespace :announce do
          desc "Generate release announces from a release note"
          task :generate do
            puts "TODO: Generate release announce for #{@package}"
          end
        end
      end
    end
  end
end
