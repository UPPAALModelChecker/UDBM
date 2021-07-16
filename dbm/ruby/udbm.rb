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
# $Id: udbm.rb,v 1.9 2005/11/05 21:23:08 adavid Exp $

module UDBM

  INF=0x3fffffff

  class Constraint
    include Comparable
    attr_reader :bound

    def initialize(b,s = false)
      self.bound = b
      self.strict = s
    end

    def strict?
      @strict
    end

    def bound=(b)
      raise 'bound '+b.to_s+' out of range' if b > INF || b <= -INF
      @bound = b
      @strict = true if @bound == INF
    end

    def strict=(s)
      @strict = s || @bound == INF
    end

    def to_s
      (@strict ? '<' : '<=')+(@bound == INF ? 'INF' : @bound.to_s)
    end

    def raw
      (@bound << 1) | (@strict ? 0 : 1)
    end

    def <=>(c)
      raw <=> c.raw
    end
  end

  class Matrix
    include Enumerable

    def initialize(*a)
      @arr = *a
      @arr = [] if !@arr
    end

    def <(b)
      self << Constraint.new(b,true)
    end

    def <=(b)
      self << Constraint.new(b,false)
    end

    def <<(c)
      raise 'Constraint expected' if !c.kind_of?(Constraint)
      @arr << c
      self
    end

    def dim
      Math.sqrt(@arr.size).to_i
    end

    def size
      @arr.size
    end

    def to_s
      cols = dim
      r = c = 0
      s = '[ '
      @arr.each { |i|
        s += i.to_s
        c += 1
        if c == cols
	  r += 1
          s += r != cols ? "\n  " : ' '
          c = 0
        else
          s += "\t"
        end
      }
      s + ']'
    end

    def inspect
      "\n" + to_s
    end

    def to_a
      @arr
    end

    def [](i,j)
      @arr[i*dim+j]
    end

    def set(i,j,c)
      raise 'Constraint expected' if !c.kind_of?(Constraint)
      @arr[i*dim+j] = c
      self
    end

    def each(&b)
      @arr.each(&b)
    end
  end

  class Relation
    def ==(r)
      to_i == r.to_i
    end
  end

  class Fed
    def <<(f)   # sugar to be similar to Array::<<
      union!(f)
    end
    def dim=(d)
      set_dim!(d)
    end
  end

  def matrix
    UDBM::Matrix.new
  end

  def Fed(dim)
    UDBM::Fed.new(dim, block_given? ? yield : nil)
  end

end # module UDBM

require 'UDBM_lib' # bridge to fed_t
