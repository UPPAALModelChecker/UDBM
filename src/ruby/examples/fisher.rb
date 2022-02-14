# Fisher's protocol.
#
# This is a "hard" encoding of the well-known
# Fisher's mutual exclusion protocol. There is
# no optimization and active clock reduction is
# not encoded either. This example is meant to
# be a simple illustration of how to encode a
# model in Ruby and have it model-checked with
# the help of the DBM library.
# The UPPAAL model can be found in the demo
# directory distributed with UPPAAL.
#
# Author: Alexandre David <adavid@cs.aau.dk>.

PROMPT1 = false
PROMPT2 = false
ALGORITHM = 1
REDUCE = false

$explored = 0

require 'udbm-sys'
require 'udbm-gtk' if PROMPT1 || PROMPT2
include UDBM

class State                     # The representation of states.
  @@Sys = nil                   # One static system for all states.
  @@Max = nil                   # One array of maximal constants...
  attr_reader :loc, :set
  attr_writer :loc, :set

  def State.set_sys(sys,max)
    @@Sys = sys
    @@Max = max
  end

  # Discrete part of the state = @loc.
  # It's an array with locations & variables.
  def initialize(clocks, *loc)
    @set = clocks.zero          # Initial state starts at zero.
    @loc = *loc << 0            # Add variables at the end (id).
  end

  # Optional extra information to know if this state is waiting.
  def waiting!
    @waiting = true
    self
  end
  def waiting?
    @waiting
  end
  def visit!
    @waiting = false
    self
  end

  # Read variable id.
  def id
    @loc[@@Sys.size]
  end

  # Set variable id.
  def id=(x)
    @loc[@@Sys.size] = x
  end

  # Self deep copy.
  def copy
    s = clone
    s.set = @set.copy
    s.loc = @loc.clone
    s
  end

  # Reset clock x = value.
  def reset(x, value)
    @set.assign_clock!(x, value)
  end

  # Constrain this state with a formula.
  def constrain(formula)
    @set.and!(formula) if formula
    !@set.empty?
  end

  # Delay this state + apply invariant + extrapolate.
  def delay!
    @set.up!
    # For each process (i)
    0.upto(@@Sys.size-1) { |i|
      # apply invariant of the active location of that process.
      return nil if !constrain(@@Sys[i].inv[@loc[i]])
    }
    @set.extrapolate_max_bounds!(@@Max)
    self
  end

  # Iterator over successor states.
  def succ_each
    if !@set.empty?
      # For each process (i)
      0.upto(@@Sys.size-1) { |i|
        # For each outgoing edge of the active location of that process,
        @@Sys[i].edges[@loc[i]].each { |e|
          s = copy
          # Try to fire the transition & delay the successor.
          if (s.loc[i] = e.call(s)) && s.delay!
            yield(s)
          end
        }
      }
    end
  end

  # Detect violation of mutual exclusion.
  def error?
    $explored += 1
    count = 0
    0.upto(@@Sys.size-1) { |i| count += 1 if loc[i] == 3 }
    if count > 1
      puts "Error found:\n" + state.inspect
      exit
    end
  end

  # Filter which states we want to show.
  def show?
    count = 0
    0.upto(@@Sys.size-1) { |i| count += 1 if loc[i] == 2 }
    count == @@Sys.size-1
  end

  # For printing.
  @@Names = [ "idle", "req", "wait", "CS" ]
  def to_s
    s = "<"
    0.upto(@@Sys.size-1) { |i| s += @@Names[loc[i]] + "," }
    s += " id=#{id},\n " + set.to_s + ">"
  end
end

# Timing constant for Fisher
K = 1

class Fisher
  @@Locs = [ :idle, :req, :wait, :CS ]
  @@Idle = 0
  @@Req = 1
  @@Wait = 2
  @@CS = 3

  attr_reader :inv, :edges

  def initialize(pid, x)
    # Invariants.
    @inv = [
      nil,    # Idle
      x <= K, # Req
      nil,    # Wait
      nil ]   # CS

    # Automaton.
    @edges = [
      [ proc { |s| # Idle -> Req
          if s.id == 0
            s.reset(x, 0)
            @@Req
          end } ],

      [ proc { |s| # Req -> Wait
          s.reset(x, 0)
          s.id = pid
          @@Wait } ],

      [ proc { |s| # Wait -> Req
          if s.id == 0
            s.reset(x, 0)
            @@Req
          end },
        proc { |s| # Wait -> CS
          if s.id == pid && s.constrain(x > K)
            @@CS
          end } ],

      [ proc { |s| # CS -> Idle
          s.reset(x, 0)
          s.id = 0
          @@Idle } ] ]
  end
end

# Count the zones in a hash table of states (stats).
def count_zones(table)
  count = 0
  table.each { |key,value|
    count += value.set.fed.size
  }
  puts "#{count} zones in the hash table."
end

# Creation of the system.
clocks = Context.create('F',:x1,:x2,:x3,:x4)
State.set_sys([ Fisher.new(1, clocks.x1),
                Fisher.new(2, clocks.x2),
                Fisher.new(3, clocks.x3),
                Fisher.new(4, clocks.x4) ],
              [ 0, K, K, K, K ])

# Show states (interactive).
def show(str, state, label)
  print str
  puts state
  state.set.show(label)
  STDIN.getc
end

if ALGORITHM == 0

  waiting = [ State.new(clocks, 0,0,0,0).delay! ]
  passed = Hash.new

  while !waiting.empty?
    state = waiting.shift                      # FIFO queue.
    show("",state,"state") if PROMPT1
    lookup = passed[state.loc]                 # Lookup on the discrete part.
    if !lookup || !(state.set <= lookup.set)   # If state is not included in passed.
      state.error?                             # Explore this state.
      state.succ_each { |successor|            # For each successor.
        show("=> ",successor,"succ") if PROMPT1
        waiting << successor                   # Put it in the waiting queue.
      }
      # Add state to the passed list.
      if !lookup
        passed[state.loc] = state
        show("",state,"new") if PROMPT2 && state.show?
      else
        passed[state.loc].set |= state.set if !REDUCE
        passed[state.loc].set.or!(state.set).reduce! if REDUCE
        show("",passed[state.loc],"new") if PROMPT2 && passed[state.loc].show?
      end
    end
  end
  puts "#{passed.size} states in the passed list."
  count_zones(passed)

elsif ALGORITHM == 1

  init = State.new(clocks, 0,0,0,0,0).delay!
  init.error?

  pwlist = Hash.new
  pwlist[init.loc] = init
  queue = [ init.waiting! ]

  while state = queue.shift                    # Read state and test if != nil.
    show("",state,"state") if PROMPT1
    state.visit!.succ_each { |successor|       # For each successor.
      show("=> ",successor,"succ") if PROMPT1
      lookup = pwlist[successor.loc]           # Look-up in the hash table.
      if !lookup                               # New entry.
        successor.error?                       # Explore
        pwlist[successor.loc] = successor
        show("",successor,"new") if PROMPT2 && successor.show?
        queue << successor.waiting!
      elsif !(successor.set <= lookup.set)     # Inclusion check.
        successor.error?                       # Explore.
        lookup.set |= successor.set if !REDUCE
        lookup.set.or!(successor.set).reduce1! if REDUCE
        show("",lookup,"new") if PROMPT2 && lookup.show?
        queue << lookup.waiting! if !lookup.waiting?
      end
    }
  end
  puts "#{pwlist.size} states in the PW list."
  count_zones(pwlist)

end # ALGORITHM

puts "#{$explored} states were explored."
puts "Safe!"
