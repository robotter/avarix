set_max_path_size(10)

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

