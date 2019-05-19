set_max_path_size(10)

# the nodes can add a constant value to all computed distance costs
# by default, the node cost is set to 0
#set_node_cost(20)

add_graph('graph', {
  # nodes: {name: (x, y)}
  'alpha': (0, 100),
  'beta': (50, 50),
  'gamma': (100, 75),
}, [
  # vertices: [(node1, node2)]
  ('alpha', 'beta'),
  ('beta', 'gamma'),
])

