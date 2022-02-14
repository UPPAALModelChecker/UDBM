# This file is part of the Ruby binding to the UPPAAL DBM library.
#
# This binding is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#
# This binding is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this binding; if not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
# Boston, MA  02110-1301  USA.
#
# $Id: udbm-callback.rb,v 1.4 2005/11/04 14:28:59 adavid Exp $

module UDBM
  class Fed
    @@callback_procs = []
    @@change_methods = Hash.new

    def Fed.method_added(_id)
      Fed.register_method(_id.id2name)
    end

    def Fed.register_method(_name)
      # Modifying methods end with !, are not renamed to _old_*, and are not redefined
      if (_name =~ /!$/) && !(_name =~ /^_old_/) && !@@change_methods[_name]
        _rename = "_old_#{_name}"
        @@change_methods[_name] = _rename
        module_eval("\
          alias_method :#{_rename}, :#{_name}\n\
          def #{_name}(*arg)\n\
            result = #{_rename}(*arg)\n\
            @@callback_procs.each { |p| p.call(self,\"#{_name}\") }\n\
            result\n\
          end")
      end
    end

    # p is an object providing a method call(Fed,String), typically a Proc.
    def Fed.add_change_listener(p)
      @@callback_procs << p
    end
  end

# Bootstrap Fed if it was defined before.
  Fed.instance_methods.each { |m| Fed.register_method(m) }
end

