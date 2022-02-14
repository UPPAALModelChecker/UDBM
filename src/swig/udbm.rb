# Load module UDBM + setup namespace
#
require 'UDBM'
include UDBM

# Add sugar to Constraint.
#
class Constraint
  include Comparable
  def strict?
    isStrict
  end
  def strict=(s)
    setStrict(s)
  end
  def bound=(b)
    setBound(b)
  end
  def bound
    getBound
  end
  def <=>(c)
    cmp(c)
  end
end

# Allow: matrix <=0 <=0 <=3 <=0
#        -> DBMMatrix
def matrix
  DBMMatrix.new
end

# Allow: DBM { matrix <=0 <=0 <inf <=0 }
#        DBM(2) { matrix <=0 <=0 <inf <=0 }
#        DBM(2) {}
#        DBM(2)
#        -> DBM
def DBM(dim = 0)
  if block_given?
    a = yield
    if a
      d = DBM.new(a)
      raise "DimensionMismatch" if dim > 0 && dim != d.dim
      return d
    end
  end
  raise "InvalidDimension" if dim <= 0
  DBM.new(dim)
end

# Allow: DBMVector(0,1,3,..)
#
def DBMVector(*a)
  a.DBMVector
end

# Allow: DBMPoint(0,1.2,2.1,...)
#
def DBMPoint(*a)
  a.DBMPoint
end

# Allow: Fed(2) { (matrix ...) | (matrix ...) }
#        Fed(2) { matrix ... }
#        Fed(2) {[ matrix ..., matrix ... ]}
#        Fed(2) {}
#        -> Fed
# The version with { (..) | (..) } is more efficient
def Fed(dim)
  f = Fed.new(dim)
  if block_given?
    a = yield
    if a
      if a.kind_of?(Array)
        a.each { |b| f.add(b) }
      else
        f.add(a)
      end
    end
  end
  f
end

# Let Zero=Constraint(0)
# Allow: [ Zero, Zero, Inf, Zero ].DBM
#        -> DBM
#        [ 0, 1, 3].DBMVector
#        -> DBMVector
class Array
  def DBM
    a = DBMMatrix.new
    each { |c|
      if c.strict
        a < c.bound
      else
        a <= c.bound
      end
    }
    DBM.new a
  end
  def DBMVector
    raise "InvalidSize" if size < 1
    raise "IllegalFirstValue" if self[0] != 0
    v=DBMVector.new
    (1..(size-1)).each { |i| v << self[i] }
    v
  end
  def DBMPoint
    raise "InvalidSize" if size < 1
    raise "IllegalFirstValue" if self[0] != 0
    v=DBMPoint.new
    (1..(size-1)).each { |i| v << self[i] }
    v
  end
end

# Allow to access different array-like types
# like arrays.
#
class FedArray
  def [](i)
    get(i)
  end
end
class DBMVector
  def [](i)
    get(i)
  end
end
class DBMPoint
  def [](i)
    get(i)
  end
end

# Allow: (matrix <=0 <=0 <=3 <=0).DBM
#        -> DBM
class DBMMatrix
  def DBM
    DBM.new self
  end
end

# Common sugar for DBM and Fed
#
module CommonDBMSugar
  def empty?
    isEmpty
  end
  def empty!
    setEmpty
    self
  end
  def zero!
    setZero
    self
  end
  def init!
    setInit
    self
  end
  def intern!
    intern
    self
  end
  def convex_union(a)
    self + a
  end
  def convex_union!(a)
    convexHull(a)
  end
  def tighten_clock(k,v)
    c=copy
    c.constrainClock(k,v) # -> bool
    c
  end
  def tighten_clock!(k,v)
    constrainClock(k,v)
  end
  def tighten(*a)
    c=copy
    c.constrain(*a) # -> bool
    c
  end
  def tighten!(*a)
    constrain(*a)
  end
  def intersects?(a)
    intersects(a)
  end
  def intersection(a)
    self & a
  end
  def intersection!(a)
    intersectionWith(a)
  end
  def up
    copy.applyUp
  end
  def up!
    applyUp
  end
  def down
    copy.applyDown
  end
  def down!
    applyDown
  end
  def free_clock(k)
    copy.applyFreeClock(k)
  end
  def free_clock!(k)
    applyFreeClock(k)
  end
  def free_up(k)
    copy.applyFreeUp(k)
  end
  def free_up!(k)
    applyFreeUp(k)
  end
  def free_down(k)
    copy.applyFreeDown(k)
  end
  def free_down!(k)
    applyFreeDown(k)
  end
  def free_all_up
    copy.applyFreeAllUp
  end
  def free_all_up!
    applyFreeAllUp
  end
  def free_all_down
    copy.applyFreeAllDown
  end
  def free_all_down!
    applyFreeAllDown
  end
  def set_value(k,v)
    copy.updateValue(k,v)
  end
  def set_value!(k,v)
    updateValue(k,v)
  end
  def set_clock(x,y)
    copy.updateClock(x,y)
  end
  def set_clock!(x,y)
    updateClock(x,y)
  end
  def inc_clock(x,inc)
    copy.updateIncrement(x,inc)
  end
  def inc_clock!(x,inc)
    updateIncrement(x,inc)
  end
  def update(x,y,v)
    copy.updateGeneral(x,y,v)
  end
  def update!(x,y,v)
    updateGeneral(x,y,v)
  end
  def satisfies?(a)
    satisfies(a)
  end
  def unbounded?
    isUnbounded
  end
  def relax_up(*a)
    copy.relaxUp(*a)
  end
  def relax_up!(*a)
    relaxUp(*a)
  end
  def relax_down(*a)
    copy.relaxDown(*a)
  end
  def relax_down!(*a)
    relaxDown(*a)
  end
  def relax_all
    copy.relaxAll
  end
  def relax_all!
    relaxAll
  end
  def sub(a)
    self - a
  end
  def subtraction_empty?(a)
    isSubtractionEmpty(a)
  end
  def contains?(a)
    contains(a)
  end
  def extrapolate_max_bounds(a)
    copy.extrapolateMaxBounds(a)
  end
  def extrapolate_max_bounds!(a)
    extrapolateMaxBounds(a)
  end
  def diagonal_extrapolate_max_bounds(a)
    copy.diagonalExtrapolateMaxBounds(a)
  end
  def diagonal_extrapolate_max_bounds!(a)
    diagonalExtrapolateMaxBounds(a)
  end
  def extrapolate_LU_bounds(l,u)
    copy.extrapolateLUBounds(l,u)
  end
  def extrapolate_LU_bounds!(l,u)
    extrapolateLUBounds(l,u)
  end
  def diagonal_extrapolate_LU_bounds(l,u)
    copy.diagonalExtrapolateLUBounds(l,u)
  end
  def diagonal_extrapolate_LU_bounds!(l,u)
    diagonalExtrapolateLUBounds(l,u)
  end
  def get_point
    getPoint
  end
  # Inclusion in the sense of set inclusion
  def set_relation(a)
    exactRelation(a)
  end
  # Inclusions in the sense of DBM inclusion
  def dbm_relation(a)
    relation(a)
  end
  def included_in?(a)
    isIncludedIn(a)
  end
  def strictly_included_in?(a)
    isStrictlyIncludedIn(a)
  end
end

# Add sugar to DBM
#
class DBM
  include CommonDBMSugar
  def [](i,j)
    read(i,j)
  end
  def Fed
    Fed.new(self)
  end
end

# Add sugar to Fed
#
class Fed
  include CommonDBMSugar
  def convex_hull
    copy.convexHull
  end
  def convex_hull!
    convexHull
  end
  def union(a)
    self | a
  end
  def union!(a)
    unionWith(a)
  end
  def append(a)
    copy.add(a)
  end
  def append!(a)
    add(a)
  end
  def sub!(a)
    subtract(a)
  end
  def reduce
    copy.mergeReduce
  end
  def reduce!
    mergeReduce
  end
  def reduce_more
    copy.partitionReduce
  end
  def reduce_more!
    partitionReduce
  end
  def predt(bad)
    copy.applyPredt(bad)
  end
  def predt!(bad)
    applyPredt(bad)
  end
  def has?(a)
    has(a)
  end
  def remove_included_in(a)
    copy.removeIncludedIn(a)
  end
  def remove_included_in!(a)
    removeIncludedIn(a)
  end
end

# Add sugar to DBMVector
class DBMVector
  def to_a
    (0..(size-1)).collect { |i| get(i) }
  end
end