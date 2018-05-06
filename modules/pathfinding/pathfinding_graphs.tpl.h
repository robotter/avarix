#ifndef PATHFINDING_GRAPHS_H__
#define PATHFINDING_GRAPHS_H__

#if DOXYGEN

/// Maximum path size (as defined in `pathfinding_config.py`)
#define PATHFINDING_MAX_PATH_SIZE  10
#define PATHFINDING_NODE_COST  0

/// List of graph nodes indexes
enum {
  GRAPH_NODE_ALPHA,
  GRAPH_NODE_BETA,
  GRAPH_NODE_GAMMA,
  GRAPH_NODES_SIZE
};

/// List of graph nodes
extern const pathfinding_node_t graph_nodes;

#else

#define PATHFINDING_MAX_PATH_SIZE  $$avarix:self.max_path_size$$
#define PATHFINDING_NODE_COST  $$avarix:self.node_cost$$

#pragma avarix_tpl self.graphs_h()

#endif

#endif
