import re
import itertools
from functools import reduce
try:
  from fractions import gcd
except ImportError:
  from math import gcd

try:
  execfile
except NameError:
  def execfile(filename, locals=None, globals=None):
    with open(filename) as f:
      exec(f.read(), locals, globals)


def lcm(a, b):
  """Return the lowest common multiple"""
  return a * b // gcd(a, b)


class Task:
  def __init__(self, name, period, cost=None):
    if not re.match(r'^[a-zA-Z][a-zA-Z0-9_]*$', name):
      raise ValueError("invalid task name: %s" % name)
    if period is None:
      if cost is not None:
        raise ValueError("unexpected cost for task '%s' with no period" % name)
    else:
      if period <= 0:
        raise ValueError("invalid period for task '%s'" % name)
      if cost is None:
        raise ValueError("invalid cost for task '%s'" % name)
    self.name = name
    self.period = period
    self.cost = cost
    self.offset = None


class CodeGenerator:
  """
  Generate code from a script with task configuration

  Attribute:
    tasks -- list of tasks
    min_period -- minimum task period
    slots -- list of execution slots, each slot is a list of tasks

  """

  def __init__(self, script):
    self.min_period = None
    self.tasks = []

    # methods used in the script to define tasks
    def set_min_period(period):
      if period <= 0:
        raise ValueError("invalid min period")
      self.min_period = int(period)
    def add_task(name, period, cost=None):
      self.tasks.append(Task(name, period, cost))

    script_globals = {}
    script_locals = {
        'set_min_period': set_min_period,
        'add_task': add_task,
        }
    execfile(script, script_globals, script_locals)
    if self.min_period is None:
      raise ValueError("min period not set in config script")
    if self.tasks is None:
      raise ValueError("tasks not set in config script")

    # check tasks against min period, check names for duplicates
    names = set()
    for task in self.tasks:
      if task.name in names:
        raise ValueError("duplicate task name: %s" % task.name)
      names.add(task.name)
      if task.period is not None:
        if task.period < self.min_period:
          raise ValueError("period too low for task '%s'" % task.name)
        if task.period % self.min_period != 0:
          raise ValueError("period of task '%s' not a multiple of min period" % task.name)

    self.solve()


  def solve(self):
    """Sort tasks and distribute their execution time offset"""

    # sort tasks by total cost (freq * cost)
    # non periodic tasks are put at the end
    # None is less than every integer, so this is ok
    self.tasks.sort(key=lambda t: None if t.period is None else t.period * t.cost, reverse=True)

    slots = [ [] for i in range(self.slot_count()) ]
    for task in self.tasks:
      if task.period is None:
        break  # next tasks' period are None too
      period = task.period / self.min_period
      # get the best offsets
      best_cost = None
      best_offsets = []
      for i in range(period):
        cost = sum(t.cost for slot in slots[i::period] for t in slot)
        if best_cost is None or cost < best_cost:
          best_cost = cost
          best_offsets = [i]
        elif cost == best_cost:
          best_offsets.append(i)

      # identify groups of contiguous best offsets
      best_group = None
      for _, g in itertools.groupby(enumerate(best_offsets), lambda i,j: i-j):
        group = [ x[1] for x in g ]
        start, end = group[0], group[-1]
        if best_group is None or end - start > best_group[1] - best_group[0]:
          best_group = start, end

      # take the middle of the largest group
      offset = (best_group[0] + best_group[1]) // 2
      task.offset = offset * self.min_period

      # add the task to the slots
      for slot in slots[offset::period]:
        slot.append(task)


  def min_period_us(self):
    return self.min_period

  def slot_count(self):
    # lowest common multiple of all periods
    periods = [ int(t.period//self.min_period) for t in self.tasks if t.period is not None ]
    if not len(periods):
      return 0
    return reduce(lcm, periods)

  def periodic_tasks_end(self):
    for i, task in enumerate(self.tasks):
      if task.period is None:
        return i
    return len(self.tasks)


  def task_name_enum(self):
    ret = ''
    for task in self.tasks:
      ret += "  IDLE_TASK_%s,\n" % task.name
    return ret

  def idle_always_tasks_size(self):
    return len(self.tasks) - self.periodic_tasks_end()

  def idle_periodic_tasks(self):
    ret = ''
    for task in self.tasks:
      if task.period is not None:
        ret += "  { NULL, %u, %u },\n" % (task.period, task.offset)
    return ret


if __name__ == 'avarix_templatizer':
  import sys
  template_locals = {'self': CodeGenerator(sys.argv[1])}


