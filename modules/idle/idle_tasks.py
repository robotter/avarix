import re


class Task:
  def __init__(self, name, freq, cost):
    if not re.match(r'^[a-zA-Z][a-zA-Z0-9_]*$', name):
      raise ValueError("invalid task name: %s" % name)
    if freq <= 0 or freq > 255:
      raise ValueError("invalid frequency for task '%s'" % name)
    self.name = name
    self.freq = freq
    self.cost = cost
    self.offset = None


class CodeGenerator:
  """
  Generate code from a script with task configuration

  Attribute:
    tasks -- list of tasks
    max_freq -- maximum task frequency
    slots -- list of execution slots, each slot is a list of tasks

  """

  def __init__(self, script):
    self.max_freq = None
    self.tasks = []

    # methods used in the script to define tasks
    def set_max_frequency(freq):
      if freq <= 0:
        raise ValueError("invalid max frequency")
      if freq > 255:
        raise ValueError("max frequency is too high")
      self.max_freq = freq
    def add_task(name, freq, cost):
      self.tasks.append(Task(name, freq, cost))

    script_globals = {}
    script_locals = {
        'set_max_frequency': set_max_frequency,
        'add_task': add_task,
        }
    execfile(script, script_globals, script_locals)
    if self.max_freq is None:
      raise ValueError("max frequency not set in config script")
    if self.tasks is None:
      raise ValueError("tasks not set in config script")

    # check tasks against max freq, check names for duplicates
    names = set()
    for task in self.tasks:
      if task.name in names:
        raise ValueError("duplicate task name: %s" % task.name)
      names.add(task.name)
      if task.freq > self.max_freq:
        raise ValueError("frequency too high for task '%s'" % task.name)
      if self.max_freq % task.freq != 0:
        raise ValueError("frequency of task '%s' not a divider of max frequency" % task.name)

    self.solve()


  def solve(self):
    """Distribute tasks to slots"""

    slots = [ [] for i in range(self.max_freq) ]
    # sort tasks by total cost (freq * cost)
    for task in sorted(self.tasks, key=lambda t: t.freq * t.cost, reverse=True):
      period = self.max_freq / task.freq
      # get the better slot
      _, task.offset = min( (sum(t.cost for slot in slots[i::period] for t in slot), i) for i in range(period) )
      # add the task to the slots
      for slot in slots[task.offset::period]:
        slot.append(task)


  def task_name_enum(self):
    ret = ''
    for task in self.tasks:
      ret += "  IDLE_TASK_%s,\n" % task.name
    return ret

  def idle_tasks(self):
    ret = ''
    for task in self.tasks:
      ret += "  { NULL, %u, %u },\n" % (self.max_freq / task.freq, task.offset)
    return ret


if __name__ == 'avarix_templatizer':
  import sys
  template_locals = {'self': CodeGenerator(sys.argv[1])}


