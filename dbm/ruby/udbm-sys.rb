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
# Support for systems of constraints.
# $Id: udbm-sys.rb,v 1.11 2005/11/04 14:28:59 adavid Exp $

require 'udbm'

# Hook to treat transparently Fixnum, Clock, or Formula.
class Fixnum
  def clock_id
    0
  end
  def offset
    self
  end
  def plus_clock
    nil
  end
  def minus_clock
    nil
  end
end

module UDBM

  # Simplified inspect for our own classes.
  module Inspect
    def inspect
      "#<#{self.class} #{self}>"
    end
  end

  # Check that objects have the same Context.
  module CheckContext
    def check(c)
      raise "Different contexts (#{self.context} != #{c.context})" if !c.kind_of?(Fixnum) && !self.context.equal?(c.context)
    end
  end

  # A context has clocks. Sets are defined within a context.
  class Context
    include Inspect
    @@All = Hash.new
    attr_reader :name, :clock_names, :set_class, :context_id, :short_names

    # Create a (uniquely named) context with a set of symbols (clock names).
    def Context.create(name, *symbols)
      raise "Context_#{name} already defined!" if @@All[name]
      UDBM.module_eval("class Context_#{name} < Context\ndef initialize(name,*symbols)\nsuper(name,*symbols)\nend\nend")
      @@All[name] = module_eval("Context_#{name}").new(name, *symbols)
    end

    # Get a context previously defined, or nil.
    def Context.get(name)
      @@All[name]
    end

    # Context is never initialized directly but it is called from its
    # children classes Context_#{name}. The symbols are added dynamically
    # as accessor methods for the defined clocks. The class handling sets
    # is also created and augmented with methods x= for updating clocks x.
    def initialize(name, *symbols)
      raise "Do not use 'new', use create instead." if self.class == Context
      self.class.module_eval("class Set_#{name} < Set\ndef initialize(set_inst)\nsuper(set_inst)\nend\nend")
      @set_class = instance_eval("Set_#{name}")
      @name = name
      @clock_names = []
      @clocks = []
      @update_cache = Hash.new
      @short_names = []
      symbols.each { |s|
        raise "Clock #{s} already defined!" if @clock_names.include?("#{name}.#{s}")
        self.class.module_eval("def #{s}\n@clocks[#{@clocks.size}]\nend")            # Context_#{name}::#{s} = clock #{s}
        @clock_names << "#{name}.#{s}"                                               # @clock_names[index] = clock symbol
        @clocks << Clock.new(self, @clocks.size)                                     # clock index = index in @clocks
        @set_class.module_eval("def #{s}=(x)\nassign_clock_id!(#{@clocks.size},x)\nend") # clock ID = clock index + 1
        @short_names << s
      }
      @context_id = @short_names.join(',').intern
    end

    # Dimension for DBMs = number of clocks + 1 (add reference clock, ID = 0)
    def dim
      @clocks.size+1
    end

    # Set with the point zero (origin)
    def zero
      @set_class.new(SymbolicSet.new(self, Fed.zero(dim)))
    end

    # "True" contains all clock valuations.
    def true
      @set_class.new(SymbolicSet.new(self, Fed.init(dim)))
    end

    # Generate a random set.
    def random
      @set_class.new(SymbolicSet.new(self, Fed.random(dim)))
    end

    # The empty set.
    def false
      @set_class.new(SymbolicSet.new(self, Fed.new(dim)))
    end

    # Name of the context and its clocks.
    def to_s
      "{#{@clock_names.join(',')}}"
    end

    # For changing context: gives update array for clocks.
    def update(ctx)
      a = @update_cache[ctx]
      return a if a
      @update_cache[ctx] = ctx.short_names.collect { |c|
        i = @short_names.index(c)
        i ? i+1 : nil
      }
    end

    # Clock variable within a context.
    class Clock
      include Inspect
      include CheckContext
      attr_reader :context

      def initialize(context, index)
        @context = context
        @index = index
      end

      # The context has all the names.
      def to_s
        @context.clock_names[@index]
      end

      # (short) name.
      def name
        @context.short_names[@index]
      end

      # Clock ID for operations with DBMs: +1 for the reference clock (ID=0)
      def clock_id
        @index+1
      end

      # No offset value.
      def offset
        0
      end

      def plus_clock
        clock_id
      end

      def minus_clock
        nil
      end
    end

    # A set is represented internally as a Formula or SymbolicSet.
    # In practice, only child classes Set_#{name} are used. It is
    # mostly a wrapper around Formula or SymbolicSet.
    # Formula and SymbolicSet respond to owner=, context, and fed.
    class Set
      include Inspect
      include CheckContext
      attr_reader :instance
      attr_writer :instance

      def initialize(instance)
        (@instance = instance).owner = self
      end

      def fed
        @instance.fed
      end

      def context
        @instance.context
      end

      def to_s
        @instance.to_s
      end

      def hash
        fed.hash
      end

      def eql?(s)
        fed.eql?(s)
      end

      # Change context: keep/move some clocks, remove & add others.
      # Cannot change itself because we need correct .clock_name
      # methods and the correct class Set_context_name.
      def to_context(ctx)
        f = fed.copy
        f.change_clocks!(context.update(ctx)) if !context.context_id.equal?(ctx.context_id)
        ctx.set_class.new(SymbolicSet.new(ctx, f))
      end

      # Callback for the assignments a_set.clock = something.
      def assign_clock!(clock, arg)
        assign_clock_id!(clock.clock_id, arg)
      end
      def assign_clock_id!(clock_id, arg)
        check(arg)
        if arg.kind_of?(Fixnum)
          fed.update_value!(clock_id, arg)
        else
          fed.update!(clock_id, arg.clock_id, arg.offset)
        end
        self
      end

      # Deep copy of itself & possible convertion to SymbolicSet.
      # The class is not Set but Set_#{name} within a context.
      def copy
        self.class.new(@instance.copy)
      end

      def and!(ss)
        @instance.and!(ss)
        self
      end

      def or!(ss)
        @instance.or!(ss)
        self
      end

      def &(ss)
        copy.and!(ss)
      end

      def |(ss)
        copy.or!(ss)
      end

      def -(ss)
        copy.subtract!(ss)
      end

      def +(ss)
        copy.convex_add!(ss)
      end

      def satisfies?(ss)
        check(ss)
        !(self & ss).fed.empty?
      end

      def up_stop(ss)
        copy.up_stop!(ss)
      end

      def up_stop!(ss)
        fed.up_stop!(ss.collect { |x| x.clock_id })
        self
      end

      def intern!
        @instance.intern!
        self
      end

      def reduce1!
        fed.merge_reduce!
        self
      end

      def reduce2!
        fed.convex_reduce!
        self
      end

      def reduce3!
        fed.expensive_convex_reduce!
        self
      end

      def reduce4!
        fed.partition_reduce!
        self
      end

      def reduce5!
        fed.expensive_reduce!
        self
      end

      alias_method :reduce!, :reduce1!
    end

    # Representation of sets as formulas (tree).
    class Formula
      include Inspect
      include CheckContext
      attr_reader :left, :right, :op, :context
      attr_writer :owner

      def initialize(context,left,right,op)
        @context = context
        @left = left
        @right = right
        @op = op
      end

      # Recursively generate the formula string.
      def to_s
        "(#{@left}#{@op}#{@right})"
      end

      # Convert to a SymbolicSet and return the federation.
      def fed
        (@owner.instance = SymbolicSet.new(@context, Eval.new(@context.dim, self).fed)).fed
      end

      # Shallow copy -- cannot change context.
      def copy
        clone
      end

      # At most one clock.
      def one_clock(c1, c2)
        raise "Invalid formula (#{context.clock_names[c1-1]} and #{context.clock_names[c2-1]} in #{self})" if c1 && c2
        c1 ? c1 : c2
      end

      # One clock on the left (may have to move -clock from right)
      def left_clock_id
        c = one_clock(@left.plus_clock, @right.minus_clock)
        c ? c : 0
      end

      # One clock on the right (may have to move -clock from left)
      def right_clock_id
        c = one_clock(@right.plus_clock, @left.minus_clock)
        c ? c : 0
      end

      # Find +clock expressions.
      def plus_clock
        if @op == :+
          one_clock(@left.plus_clock, @right.plus_clock)
        elsif @op == :-
          one_clock(@left.plus_clock, @right.minus_clock)
        else
          raise "Invalid formula (#{self})"
        end
      end

      # Find -clock expressions.
      def minus_clock
        if @op == :+
          one_clock(@left.minus_clock, @right.minus_clock)
        elsif @op == :-
          one_clock(@left.minus_clock, @right.plus_clock)
        else
          raise "Invalid formula (#{self})"
        end
      end

      # Exactly one +clock.
      def clock_id
        p = plus_clock
        m = minus_clock
        raise "One clock expected (#{self})" if !(p && !m)
        p
      end

      # Read clock offset for operations on federations.
      def offset
        if @op == :+
          @left.offset + @right.offset
        elsif @op == :-
          @left.offset - @right.offset
        else
          raise "Integer expression expected (#{self})"
        end
      end

      # Intersection: augment formula or convert.
      def and!(arg)
        check(arg)
        if arg.instance.kind_of?(Formula)
          (@owner.instance = Formula.new(@context, self, arg.instance, :&)).owner = @owner
          @owner = nil
        elsif formula_conjunction?
          f = self # be sure there's a ref to self
          EvalConjunction.new((@owner.instance = arg.instance.copy).fed, f)
          f.owner = nil
        else
          fed.intersection!(arg.fed)
        end

        # Ignore
        def intern!
        end
      end

      # Union: augment formula or convert.
      def or!(arg)
        check(arg)
        if arg.instance.kind_of?(Formula)
          (@owner.instance = Formula.new(@context, self, arg.instance, :|)).owner = @owner
          @owner = nil
        else
          fed.union!(arg.fed)
        end
      end

      # Test if this is a pure conjunction.
      def formula_conjunction?
        return @conjunct if @conjunct != nil
        if @op == :-
          @conjunct = !@left.kind_of?(Formula) && !@right.kind_of?(Formula)
        else
          @conjunct = @op != :| && (@op != :& || (@left.formula_conjunction? && @right.formula_conjunction?))
        end
      end
    end

    # Evaluation a formula of conjunctions.
    class EvalConjunction
      attr_reader :fed

      def initialize(fed, f)
        @fed = fed
        method(f.op).call(f)
      end

      def ==(f)
        self <= f
        self >= f
      end

      def <(f)
        @fed.constrain!(f.left_clock_id, f.right_clock_id, f.right.offset-f.left.offset, true)
      end

      def <=(f)
        @fed.constrain!(f.left_clock_id, f.right_clock_id, f.right.offset-f.left.offset, false)
      end

      def >(f)
        @fed.constrain!(f.right_clock_id, f.left_clock_id, f.left.offset-f.right.offset, true)
      end

      def >=(f)
        @fed.constrain!(f.right_clock_id, f.left_clock_id, f.left.offset-f.right.offset, false)
      end

      def +(f)
        raise 'Invalid operation (+)'
      end

      def -(f)
        raise 'Subtraction not supported (-)'
      end

      def &(f)
        method(f.left.op).call(f.left)
        method(f.right.op).call(f.right)
      end

      def |(f)
        raise 'Union not supported (|)'
      end
    end

    # Evaluation of a generic formula.
    class Eval < EvalConjunction
      def initialize(dim, f)
        super(Fed.init(dim), f)
      end

      def -(f)
        fed_op(f, :subtract!)
      end

      def &(f)
        if f.formula_conjunction?
          method(f.left.op).call(f.left)
          method(f.right.op).call(f.right)
        else
          fed_op(f, :intersection!)
        end
      end

      def |(f)
        fed_op(f, :union!)
      end

      def fed_op(f, fed_method)
        fcopy = @fed.copy
        farg = method(f.right.op).call(f.right)
        @fed = fcopy
        method(f.left.op).call(f.left).method(fed_method).call(farg)
      end
    end

    # Representation of sets as federations.
    class SymbolicSet
      include Inspect
      include CheckContext
      attr_reader :context, :fed

      def initialize(context, fed)
        @context = context
        @fed = fed
      end

      def intern!
        @fed.intern!
      end

      # This is only for formulas.
      def formula_conjunction?
        false
      end

      # Deep copy: same context, copy of Fed.
      def copy
        SymbolicSet.new(@context, @fed.copy)
      end

      # For compatibility with Formula.
      def owner=(o)
        # don't need an owner, ignore.
      end

      # To string: ask Fed to do it.
      def to_s
        @fed.formula(@context.clock_names)
      end

      # Intersection.
      def and!(arg)
        check(arg)
        if arg.instance.formula_conjunction?
          EvalConjunction.new(@fed, arg.instance)
        else
          @fed.intersection!(arg.fed)
        end
      end

      # Union.
      def or!(arg)
        check(arg)
        @fed.union!(arg.fed)
      end
    end

    # Add operators to Clock and Formula

    [ :==, :<, :<=, :>, :>= ].each { |o|
      Clock.module_eval("def #{o}(x)\n check(x)\n @context.set_class.new(Formula.new(@context, self, x, :#{o}))\n end")
      Formula.module_eval("def #{o}(x)\n check(x)\n @context.set_class.new(Formula.new(@context, self, x, :#{o}))\n end")
    }

    [ :+, :- ].each { |o|
      Clock.module_eval("def #{o}(x)\n check(x)\n Formula.new(@context, self, x, :#{o})\n end")
      Formula.module_eval("def #{o}(x)\n check(x)\n Formula.new(@context, self, x, :#{o})\n end")
    }

    # Add methods to Set -- wrappers to Fed.

    # no arg -> direct return
    [ :empty?, :unbounded? ].each { |m|
      Set.module_eval("def #{m}\n fed.#{m}\n end")
    }

    # no arg -> return self
    [ :zero!, :empty!, :convex_hull!, :up!, :down!, :free_all_up!, :free_all_down!, :relax_up!, :relax_down!, :relax_all! ].each { |m|
      Set.module_eval("def #{m}\n fed.#{m}\n self\n end")
    }

    # no arg -> return copy
    [ :empty, :convex_hull, :up, :down, :free_all_up, :free_all_down, :relax_up, :relax_down, :relax_all ].each { |m|
      Set.module_eval("def #{m}\n copy.#{m}!\n end")
    }

    # arg Set -> return self
    [ :remove_included_in!, :predt!, :subtract!, :convex_add!, :union! ].each { |m|
      Set.module_eval("def #{m}(ss)\n check(ss)\n fed.#{m}(ss.fed)\n self\n end")
    }

    # arg Set -> return copy
    [ :remove_included_in, :predt ].each { |m|
      Set.module_eval("def #{m}(ss)\n copy.#{m}!(ss)\n end")
    }

    # arg Set -> direct return
    [ :relation, :<, :>, :<=, :>=, :== ].each { |m|
      Set.module_eval("def #{m}(ss)\n check(ss)\n fed.#{m}(ss.fed)\n end")
    }

    # arg Clock -> return self
    [ :free_clock!, :free_up!, :free_down!, :relax_up_clock!, :relax_down_clock! ].each { |m|
      Set.module_eval("def #{m}(k)\n check(k)\n fed.#{m}(k.clock_id)\n self\n end")
    }

    # arg Clock -> return copy
    [ :free_clock, :free_up, :free_down, :relax_up_clock, :relax_down_clock ].each { |m|
      Set.module_eval("def #{m}(k)\n copy.#{m}!(k)\n end")
    }

    # arg [ clocks ] -> return self
    [ :up_stop!, :down_stop! ].each { |m|
      Set.module_eval("def #{m}(a)\n fed.#{m}(a.collect{|c| c.clock_id})\n self\n end")
    }

    # arg [ clocks ] -> return copy
    [ :up_stop, :down_stop ].each { |m|
      Set.module_eval("def #{m}(a)\n copy.#{m}!(a)\n end")
    }

    # direct arg -> direct return
    [ :contains?, :possible_back_delay ].each { |m|
      Set.module_eval("def #{m}(x)\n fed.#{m}(x)\n end")
    }

    # direct arg -> return self
    [ :extrapolate_max_bounds!, :diagonal_extrapolate_max_bounds! ].each { |m|
      Set.module_eval("def #{m}(x)\n fed.#{m}(x)\n self\n end")
    }

    # direct arg -> return copy
    [ :extrapolate_max_bounds, :diagonal_extrapolate_max_bounds ].each { |m|
      Set.module_eval("def #{m}(x)\n copy.#{m}!(x)\n end")
    }

    # 2xdirect arg -> return self
    [ :extrapolate_lu_bounds!, :diagonal_extrapolate_lu_bounds! ].each { |m|
      Set.module_eval("def #{m}(x,y)\n fed.#{m}(x,y)\n self\n end")
    }

    # 2xdirect arg -> return copy
    [ :extrapolate_lu_bounds, :diagonal_extrapolate_lu_bounds ].each { |m|
      Set.module_eval("def #{m}(x,y)\n copy.#{m}!(x,y)\n end")
    }

  end
end
