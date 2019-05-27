/** @defgroup pathfinding Pathfinding
 * @brief Pathfinding
 *
 * Find a path in a graph of nodes, using A* algorithm.
 * Nodes and vertices can be blocked by obstacles, defined at runtime.
 *
 * Graphs must be defined in pathfinding_config.py.
 * For instance:
 * \code{.py}
 * set_max_path_size(10)
 *
 * add_graph('graph', {
 *   # name: (x, y)
 *   'alpha': (0, 100),
 *   'beta': (50, 50),
 *   'gamma': (100, 75),
 * }, [
 *   # (node1, node2)
 *   ('alpha', 'beta'),
 *   ('beta', 'gamma'),
 * ])
 * \endcode
 */
//@{
/**
 * @file
 * @brief Pathfinding module definitions
 */
#ifndef PATHFINDING_H__
#define PATHFINDING_H__

#include <avarix.h>
#include <stdint.h>
#include <stdbool.h>

/// Graph node
typedef struct {
  int16_t x, y;
  uint8_t neighbors_size;  ///< Number of neighbors
  uint8_t const *neighbors;  ///< Neighbor node indexes
} pathfinding_node_t;

/// Graph obstacle (circular)
typedef struct {
  int16_t x, y;
  int16_t r;
} pathfinding_obstacle_t;

#include "pathfinding/pathfinding_graphs.h"

/// Path finder
typedef struct {
  /// Graph nodes
  const pathfinding_node_t *nodes;
  /// Nodes array size
  uint8_t nodes_size;
  /// Obstacles, to compute unreachable nodes
  pathfinding_obstacle_t *obstacles;
  /// Obstacles array size
  uint8_t obstacles_size;
  /// Result path, include the starting node
  uint8_t path[PATHFINDING_MAX_PATH_SIZE];
  /// Result path size, 0 if no path found
  uint8_t path_size;
} pathfinding_t;


/// Find a path
void pathfinding_search(pathfinding_t *finder, uint8_t start, uint8_t goal);

/// Find nearest node to given coordinates
uint8_t pathfinding_nearest_node(const pathfinding_t *finder, int16_t x, int16_t y);

/// Set nodes of a given graph to a pathfinding_t object
#define pathfinding_set_nodes(finder, name)  do { \
  (finder)->nodes = name ## _nodes; \
  (finder)->nodes_size = ARRAY_LEN(name ## _nodes); \
} while(0)

#endif
//@}
