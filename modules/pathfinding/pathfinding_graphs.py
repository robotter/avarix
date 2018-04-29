#!/usr/bin/env python3

template_graph_h = """
enum {
%(c_nodes_enum)s
  %(c_VAR)s_NODES_SIZE
};

extern const pathfinding_node_t %(c_var)s_nodes[%(nodes_count)d];

"""

template_graph_c = """

%(c_neighbors)s

const pathfinding_node_t %(c_var)s_nodes[%(nodes_count)d] = {
%(c_nodes)s
};

"""


class Node:
  def __init__(self, name, index, x, y, neighbors):
    self.name = name
    self.index = index
    self.x = x
    self.y = y
    self.neighbors = neighbors

class Graph:
  def __init__(self, name, nodes, vertices):
    self.name = name

    # normalize vertices
    vertices_pairs = []
    for value in vertices:
      if isinstance(value, str):
        value = value.split()
      if len(value) < 2:
        raise ValueError("single node vertex: %r" % value)
      vertices_pairs += [(value[i], value[i+1]) for i in range(len(value)-1)]

    neighbors = {name: set() for name in nodes}  # {name: {name, ...}}
    for a, b in vertices_pairs:
      if a == b:
        raise ValueError("vertex with same nodes at both side: %r" % a)
      if a not in nodes:
        raise ValueError("unknown node used in vertices: %r" % a)
      if b not in nodes:
        raise ValueError("unknown node used in vertices: %r" % b)
      neighbors[a].add(b)
      neighbors[b].add(a)

    self.nodes = []
    for i, (name, (x, y)) in enumerate(sorted(nodes.items())):
      #if name not in neighbors:
      #  raise ValueError("node %r has no neighbors" % name)
      self.nodes.append(Node(name, i, x, y, neighbors[name]))

    # resolve node names to Node instances
    nodes_by_name = {node.name: node for node in self.nodes}
    for node in self.nodes:
      node.neighbors = [nodes_by_name[name] for name in sorted(node.neighbors)]
    self.vertices = [(nodes_by_name[n1], nodes_by_name[n2]) for n1, n2 in vertices_pairs]

  def c_node_enum(self, node):
      return "%s_NODE_%s" % (self.name.upper(), node.name.upper())

  def gen_h(self):
    c_var = self.name
    c_VAR = c_var.upper()
    nodes_count = len(self.nodes)
    c_nodes_enum = '\n'.join("  %s," % self.c_node_enum(node) for node in self.nodes)
    return template_graph_h % locals()

  def gen_c(self):
    c_var = self.name
    c_VAR = c_var.upper()
    nodes_count = len(self.nodes)

    c_neighbors = '\n'.join("static const uint8_t %s_neighbors_%s[] = {%s};" % (
      c_var, node.name, ', '.join(self.c_node_enum(n) for n in node.neighbors)
    ) for node in self.nodes)

    c_nodes = '\n'.join(" /* %2d */ [%s] = {%d, %d, %d, %s_neighbors_%s}," % (
      node.index, self.c_node_enum(node), node.x, node.y, len(node.neighbors), c_var, node.name
    ) for node in self.nodes)

    return template_graph_c % locals()


class CodeGenerator:
  """
  Generate code from a script with graphs

  Attribute:
    max_path_size -- maximum result path size
    graphs -- list of graphs

  """

  def __init__(self, script):
    self.max_path_size = None
    self.graphs = []

    def set_max_path_size(n):
      if n <= 2:
        raise ValueError("invalid max path size")
      self.max_path_size = int(n)

    def add_graph(name, nodes, vertices):
      self.graphs.append(Graph(name, nodes, vertices))

    script_globals = {}
    script_locals = {
      'set_max_path_size': set_max_path_size,
      'add_graph': add_graph,
    }

    with open(script) as f:
      exec(f.read(), script_globals, script_locals)
    if self.max_path_size is None:
      raise ValueError("max path size must be set in config script")
    if not self.graphs:
      raise ValueError("no graph configured in config script")

    # check for duplicate graph names
    names = set()
    for graph in self.graphs:
      if graph.name in names:
        raise ValueError("duplicate graph name: %s" % graph.name)
      names.add(graph.name)

  def graphs_h(self):
    return '\n'.join(g.gen_h() for g in self.graphs)

  def graphs_c(self):
    return '\n'.join(g.gen_c() for g in self.graphs)


if __name__ == 'avarix_templatizer':
  import sys
  template_locals = {'self': CodeGenerator(sys.argv[1])}

