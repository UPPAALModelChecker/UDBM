
# Leader election protocol by Leslie Lamport described in
# the paper "Real Time is Really Simple".
#
# The following is Ruby implementation of the protocol,
# which solves the following problem:
# There is a number of interconnected network nodes. Each node
# has an address. The goal of the protocol is to identify the
# node with the lowest address in the network and elect that
# node to be the leader. In order for the protocol to be correct,
# all connected nodes should have elected the same leader.
#
# The implementation is based on classes representing nodes,
# messsages and the system as a whole. Uppaal DBM library is used
# for modeling time.
#
# Author: Jiri Simsa, simsa@mail.muni.cz

require 'udbm-sys'
#require 'udbm-gtk'
include UDBM

DEBUG = false # Print output?
N = 2 # Number of processes - 1
I_TMO = 10 # Initial timeout period
TMO_D = 5 # Timeout delay
MSG_D = 3 # Message delay
MSG = 6 # Maximum number of node's pending messages
MAX_MSG = MSG * (N+1) # Maximum number of pending messages
MAX_DST = 1 # Maximum distance between nodes (topology related)

CH_SEND = (0..MAX_MSG-1).to_a # Message channels
CH_DELIVER = (MAX_MSG..MAX_MSG+N+1).to_a # Delivery channels


$ldr = 0                               # Correct leader value
$neighbors = [[0,1,1],[1,0,1],[1,1,0]] # Network topology


class Fixnum
  def call(s)
    self
  end
end

class State
  @@Sys = nil
  @@Max = nil
  attr_reader :loc, :set, :used, :ms, :up, :to_deliver, :messages, :inv, :ldrs
  attr_writer :loc, :set, :used, :ms, :up, :to_deliver, :messages, :inv, :ldrs

  def State.set_sys(sys,max)
    @@Sys = sys
    @@Max = max
  end

  def initialize(clocks, *loc)
    @set = clocks.zero
    @loc = *loc
    @used = Array.new(MAX_MSG, false) # Flags for using channels
    @ms = [0,0,0] # Shared message for value passing
    @up = 0 # Shared upstream id for value passing
    @to_deliver = Array.new(MAX_MSG)
    @to_deliver = @to_deliver.collect { |i| Array.new(N+1, false) }
    @messages = Array.new(MAX_MSG)
    @messages = @messages.collect { |i| Array.new(N+1, [0,0,0]) }
    @inv = Array.new(N+1)
    @ldrs = (0..N).to_a.collect { |i| [] << i << 0 }
  end
  
  # Self deep copy.
  def copy
    s = clone
    s.set = @set.copy
    s.loc = @loc.clone
    s.used = @used.clone
    s.ms = @ms.clone
    s.to_deliver = @to_deliver.clone
    0. upto(MAX_MSG-1) { |i| s.to_deliver[i] = @to_deliver[i].clone }
    s.messages = @messages.clone
    0. upto(MAX_MSG-1) { |i| s.messages[i] = @messages[i].clone }
    s.inv = @inv.clone
    s.ldrs = @ldrs.clone
    s
  end

  # Reset x = somthing.
  def reset(x, val)
    @set.assign_clock_id!(x.clock_id, val)
  end

  # Used in guards.
  def constrain(formula)
    @set.and!(formula) if formula
    !@set.empty?
  end

  def is_committed?
    (0..N).each { |i|
      if [1,2,3].include?(@loc[i])
        return true
      end
    }
    false
  end

  def delay!
    @set.up! if !is_committed?
    (0..N).each { |i|
      if @inv[i]
        @@Sys[i].inv[0] = @inv[i]
      end
    }
    (0..@@Sys.size-1).each { |i|      
      return nil if !constrain(@@Sys[i].inv[@loc[i]])
    }
    @set.extrapolate_max_bounds!(@@Max).reduce!
    self
  end

  def succ_each
    if !@set.empty?
      # Commited transitions
      committed = false
      (0..N).each { |i|
        if [1,2,3].include?(@loc[i])
          committed = true

          @@Sys[i].edges[@loc[i]].each { |e|
            if e[0]
              (N+1..N+MAX_MSG).each { |j|
                @@Sys[j].edges[@loc[j]].each { |f|
                  s = copy
                  if ((channel_id(f[0],s) == channel_id(e[0],s)) &&
                        (s.loc[i] = e[1].call(s)) &&
                        (s.loc[j] = f[1].call(s)) && 
                        s.delay!) 
                    puts " -> Succesor state " + s.loc.join(",") if DEBUG
                    yield(s)
                  end
                }
              }
            else
              s = copy
              if (s.loc[i] = e[1].call(s)) && s.delay!
                puts " -> Succesor state " + s.loc.join(",") if DEBUG 
                yield(s)
              end                          
            end
          }
        end
      }
      if (!committed)
        # Other transitions
        (0..N).each { |i|

          @@Sys[i].edges[@loc[i]].each { |e|
            if e[0]
              (N+1..N+MAX_MSG).each { |j|
                @@Sys[j].edges[@loc[j]].each { |f|
                  s = copy
                  if ((channel_id(f[0],s) == channel_id(e[0],s)) &&
                        (s.loc[i] = e[1].call(s)) &&
                        (s.loc[j] = f[1].call(s)) && 
                        s.delay!) 
                    puts " -> Succesor state " + s.loc.join(",") if DEBUG
                    yield(s)
                  end
                }
              }
            else
              s = copy
              if (s.loc[i] = e[1].call(s)) && s.delay!
                puts " -> Succesor state " + s.loc.join(",") if DEBUG 
                yield(s)
              end                          
            end
          }
        }
      end
    end
  end
  
  def channel_id(c,s)
    c.call(s)
  end
end

class Message
  @@Locs = [ :init, :send ]
  @@Init = 0
  @@Send = 1

  attr_reader :inv, :edges, :pid

  def initialize(pid, x)
    @pid = pid
    @inv = [
      nil, # Init
      x <= MSG_D ] # Send
    @edges = [
      [ [ CH_SEND[pid], 
          proc { |s| # Init -> Send
            prepare(s)
            s.reset(x, 0)
            @@Send } ]
      ],
      (0..N).to_a.collect { |i|
        [ CH_DELIVER[i] ] <<
          proc { |s| # Send -> Send
          if (s.to_deliver[pid][i] && (count(s) > 1))
            deliver_to(s, i, false)
            @@Send
          end }
      } + 
        (0..N).to_a.collect { |i|
        [  CH_DELIVER[i] ] <<
          proc { |s| # Send -> Init
          if (s.to_deliver[pid][i] && (count(s) == 1))
            deliver_to(s, i, true)
            @@Init 
          end }
      }
    ]
  end
  
  def prepare(s) 
    s.messages[pid] = s.ms
    s.used[pid] = true
    0.upto(N) { |i|
      s.to_deliver[pid][i] = ((i != s.up) && ($neighbors[i][s.messages[pid][0]] == 1))
    }
  end
  
  def deliver_to(s,i,clear)
    s.ms = s.messages[pid]
    s.to_deliver[pid][i] = false
    if (clear) 
      s.used[pid] = false
      s.messages[pid] = [0,0,0]
    end
  end

  def count(s)
    cnt = 0
    0.upto(N) { |i|
      if s.to_deliver[pid][i] 
        cnt = cnt + 1
      end
    }
    return cnt
  end
end

class Node
  @@Locs = [ :init, :upd, :elect, :error ]
  @@Init = 0
  @@Recieve = 1
  @@Update = 2
  @@Elect = 3
  @@Error = 4

  attr_reader :inv, :edges, :pid, :timer

  def initialize(pid, x, gt)
    @pid = pid
    @timer = I_TMO
    @inv = [
      x <= (@timer + TMO_D), # Init
      nil, # Recieve 
      nil, # Update
      nil, # Elect
      nil ] # Error
    @edges = [
      [ [ nil , 
          proc { |s| # Init -> Elect
            if s.constrain(x > @timer)
              elect(s)
              @@Elect
            end } ],
        [ nil,
          proc { |s| # Init -> Error
            if s.constrain(gt > (I_TMO + TMO_D + $neighbors[pid][$ldr]*MSG_D)) and 
                (s.ldrs[pid][0] != $ldr)
              @@Error
            end
          } ] ,
        [ CH_DELIVER[pid],
          proc { |s| # Init -> Recieve
            @@Recieve } ] ],
      [ [ nil ,
          proc { |s| # Recieve -> Init
            if !better(s)
              @@Init
            end } ],
        [ nil ,
          proc { |s| # Recieve -> Update
            if better(s)
              update(x,s)
              @@Update
            end } ] ],
      [ [ proc { |s| CH_SEND[getSlot(s)] } , 
          proc { |s| # Update -> Init
            sendMsg(s, s.ms[0])
            s.reset(x, 0)
            @@Init } ] ],
      [ [ proc { |s| CH_SEND[getSlot(s)] } ,
          proc { |s| # Update -> Init
            sendMsg(s, pid)
            s.reset(x, 0)
            @@Init } ] ],
      [ ]
    ]
  end
  
  def better(s)
    (s.ms[1] < s.ldrs[@pid][0]) || 
      ((s.ms[1] == s.ldrs[@pid][0]) && (s.ms[2] + 1 <= s.ldrs[@pid][1]))
  end

  def update(x,s)
    d = s.ms[2] + 1
    @timer =  I_TMO + TMO_D + d*MSG_D
    s.inv[@pid] = x <= (@timer + TMO_D)
    s.ldrs[@pid] = [s.ms[1], d]    
  end

  def elect(s)
    s.ldrs[@pid] = [@pid, 0]
  end

  def sendMsg(s, up)
    s.up = up
    s.ms = [@pid, s.ldrs[@pid][0], s.ldrs[@pid][1]]
  end

  def getSlot(s)
    (@pid*MSG).upto((@pid+1)*MSG-1) { |i|
      if !s.used[i]
        return i
      end
    }
  end
end

clocks = Context.create('F',:gt,
                        :n0,:n1,:n2,
                        :s0,:s1,:s2,:s3,:s4,:s5,:s6,:s7,:s8,:s9,:s10,:s11,:s12,:s13,:s14,:s15,:s16,:s17#,:s18,:s19,:s20,:s21,:s22,:s23
                        )

State.set_sys([ Node.new(0, clocks.n0, clocks.gt),
                Node.new(1, clocks.n1, clocks.gt),
                Node.new(2, clocks.n2, clocks.gt),
                Message.new(0, clocks.s0),
                Message.new(1, clocks.s1),
                Message.new(2, clocks.s2),
                Message.new(3, clocks.s3),
                Message.new(4, clocks.s4),
                Message.new(5, clocks.s5),
                Message.new(6, clocks.s6),
                Message.new(7, clocks.s7),
                Message.new(8, clocks.s8),
                Message.new(9, clocks.s9),
                Message.new(10, clocks.s10),
                Message.new(11, clocks.s11),
                Message.new(12, clocks.s12),
                Message.new(13, clocks.s13),
                Message.new(14, clocks.s14),
                Message.new(15, clocks.s15),
                Message.new(16, clocks.s16),
                Message.new(17, clocks.s17),
                # Message.new(18, clocks.s18),
                # Message.new(19, clocks.s19),
                # Message.new(20, clocks.s20),
                # Message.new(21, clocks.s21),
                # Message.new(22, clocks.s22),
                # Message.new(23, clocks.s23)
              ],
              [ 0, I_TMO+TMO_D+MAX_DST*MSG_D, 
                I_TMO+2*TMO_D+N*MSG_D, I_TMO+2*TMO_D+N*MSG_D, I_TMO+2*TMO_D+N*MSG_D, 
                MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, 
                MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D #, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D, MSG_D 
              ])

def error?(state)
  count = 0
  (0..2).each { |i| count += 1 if state.loc[i] == 4 }
  if count > 0
    puts "Error found:\n" + state.inspect
    exit
  end
end

init = State.new(clocks, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0).delay!
error?(init)

passed = Hash.new
waiting = [ init ]
i = 0

while state = waiting.pop # shift -> BFS vs. pop -> DFS
  puts "Visiting state " + state.loc.join(",") if DEBUG
  state.succ_each { |s|
    error?(s)
    p = passed[s.loc]
    if p
      if p.set <= s.set
        p.set.empty!
        passed[s.loc] = s.copy
        waiting << s
      else
        s.set.remove_included_in!(p.set)
        if !s.set.empty?
          p.set.or!(s.set)
          waiting << s
        else
          i = i+1
        end
      end
    else
      passed[s.loc] = s.copy
      waiting << s
    end
  }
  state.set.show('current') if (DEBUG)
  STDIN.getc if (DEBUG)
end

puts "#{passed.size} states in the passed list."
puts "#{i} hash table hit(s)."
puts "Safe!"
