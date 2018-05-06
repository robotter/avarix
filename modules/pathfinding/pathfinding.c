#include <stdint.h>
#include <stdbool.h>
#include "pathfinding.h"
#include "pathfinding/pathfinding_graphs.inc.c"


#define PATHFINDING_COST_MAX  0xffffffff

static uint32_t vertex_cost(const pathfinding_node_t *start, const pathfinding_node_t *goal)
{
  const int16_t dx = start->x - goal->x;
  const int16_t dy = start->y - goal->y;
  uint32_t c = (int32_t)dx*dx + (int32_t)dy*dy;
  // compute square integer root
  // avr-libc does not provide lsqrt :(
  uint32_t res = 0;
  uint32_t bit = 1UL << 30;
  while(bit > c) {
    bit >>= 2;
  }
  while(bit != 0) {
    if(c >= res + bit) {
      c -= res + bit;
      res = (res >> 1) + bit;
    } else {
      res >>= 1;
    }
    bit >>= 2;
  }
  return res + PATHFINDING_NODE_COST;
}

#define FINDER_VERTEX_COST(finder, index0, index1) \
    vertex_cost(&(finder)->nodes[index0], &(finder)->nodes[index1])


/// Return true if node is blocked by obstacle
static bool obstacle_blocks_node(const pathfinding_obstacle_t *obstacle, const pathfinding_node_t *node)
{
  const int16_t dx = node->x - obstacle->x;
  const int16_t dy = node->y - obstacle->y;
  const int32_t r = obstacle->r;
  return (int32_t)dx*dx + (int32_t)dy*dy < r*r;
}

/// Return true if a node is blocked by any obstacle
static bool node_blocked(const pathfinding_t *finder, const pathfinding_node_t *a)
{
  for(uint8_t i=0; i<finder->obstacles_size; i++) {
    //ignore obstacles with null radius
    if(finder->obstacles[i].r == 0) {
      return false;
    }

    if(obstacle_blocks_node(&finder->obstacles[i], a)) {
      return true;
    }
  }
  return false;
}

#define FINDER_NODE_BLOCKED(finder, index_node) \
    node_blocked((finder), &(finder)->nodes[index_node])


/// Return true if vertex is blocked by obstacle
static bool obstacle_blocks_vertex(const pathfinding_obstacle_t *o, const pathfinding_node_t *a, const pathfinding_node_t *b)
{
  const int16_t dx_ao = o->x - a->x;
  const int16_t dy_ao = o->y - a->y;
  const int16_t dx_ab = b->x - a->x;
  const int16_t dy_ab = b->y - a->y;
  const uint32_t d2_ao = (int32_t)dx_ao * dx_ao + (int32_t)dy_ao * dy_ao;
  const uint32_t d2_ab = (int32_t)dx_ab * dx_ab + (int32_t)dy_ab * dy_ab;
  // early check: if AO > AB, no intersection
  // this is true because blocked nodes have already been filtered
  if(d2_ao > d2_ab) {
    return false;
  }
  // u is the distance along AB of the projection P, of O on AB, scaled by AB²
  // then:  AP = AB * u / AB² = u / AB
  const int32_t u = (int32_t)dx_ab * dx_ao + (int32_t)dy_ab * dy_ao;
  // P must be in AB for an intersection: 0 < u < AB²
  // Note: we already handled cases where the A or B are in the obstacle.
  if(u <= 0 || (uint32_t)u >= d2_ab) {
    return false;
  }
  // The obstacle intersects if:  OP² = AO² - u² / AB² < r²
  const uint32_t u2_ab2 = ((uint64_t)u * u) / d2_ab;
  return d2_ao - u2_ab2 < (uint32_t)o->r * o->r;
}

/// Return true if a vertex is blocked by any obstacle
static bool vertex_blocked(const pathfinding_t *finder, const pathfinding_node_t *a, const pathfinding_node_t *b)
{
  for(uint8_t i=0; i<finder->obstacles_size; i++) {
    //ignore obstacles with null radius
    if(finder->obstacles[i].r == 0) {
      return false;
    }

    if(obstacle_blocks_vertex(&finder->obstacles[i], a, b)) {
      return true;
    }
  }
  return false;
}

#define FINDER_VERTEX_BLOCKED(finder, index_a, index_b) \
    vertex_blocked((finder), &(finder)->nodes[index_a], &(finder)->nodes[index_b])


void pathfinding_search(pathfinding_t *finder, uint8_t start, uint8_t goal)
{
  struct {
    enum { PENDING, OPEN, CLOSED } state;
    uint8_t previous;  // most efficicient previous node (0xff for start)
    uint32_t partial_cost;  // cost from start (squared distance)
    uint32_t total_cost;  // estimated total cost (squared distance)
  } astar_nodes[finder->nodes_size];

  // initialize astar_nodes
  // consider blocked nodes as already visited
  for(uint8_t i=0; i<finder->nodes_size; i++) {
    astar_nodes[i].state = PENDING;
    astar_nodes[i].partial_cost = PATHFINDING_COST_MAX;
    astar_nodes[i].total_cost = PATHFINDING_COST_MAX;
  }

  // start node must not be blocked
  if(FINDER_NODE_BLOCKED(finder, start)) {
    goto fail;
  }

  // initialize start node
  astar_nodes[start].state = OPEN;
  astar_nodes[start].previous = 0xff;
  astar_nodes[start].partial_cost = 0;
  astar_nodes[start].total_cost = FINDER_VERTEX_COST(finder, start, goal);

  for(;;) {
    // find node with lowest estimated cost
    uint8_t current = 0xff;
    uint32_t min_cost = PATHFINDING_COST_MAX;
    for(uint8_t i=0; i<finder->nodes_size; i++) {
      if(astar_nodes[i].state == OPEN) {
        if(astar_nodes[i].total_cost < min_cost) {
          min_cost = astar_nodes[i].total_cost;
          current = i;
        }
      }
    }

    if(current == 0xff) {
      goto fail;
    } else if(current == goal) {
      // re build path: get size, then fill values
      uint8_t path_size = 1;  // +1 for starting node
      for(uint8_t i=current; i!=start; i=astar_nodes[i].previous) {
        path_size++;
      }
      finder->path_size = path_size;
      for(uint8_t i=current; i!=start; i=astar_nodes[i].previous) {
        finder->path[--path_size] = i;
      }
      finder->path[0] = start;
      return;
    }

    astar_nodes[current].state = CLOSED;
    for(uint8_t i=0; i<finder->nodes[current].neighbors_size; i++) {
      uint8_t neighbor = finder->nodes[current].neighbors[i];
      uint8_t state = astar_nodes[neighbor].state;
      if(state == PENDING) {
        if(FINDER_NODE_BLOCKED(finder, neighbor)) {
          astar_nodes[neighbor].state = CLOSED;
          continue;
        }
      } else if(state == CLOSED) {
        continue;
      }
      if(FINDER_VERTEX_BLOCKED(finder, current, neighbor)) {
        continue;
      }

      astar_nodes[neighbor].state = OPEN;  // may be a no-op
      uint32_t cost = astar_nodes[current].partial_cost + FINDER_VERTEX_COST(finder, current, neighbor);
      if(cost < astar_nodes[neighbor].partial_cost) {
        // better origin path
        astar_nodes[neighbor].previous = current;
        astar_nodes[neighbor].partial_cost = cost;
        astar_nodes[neighbor].total_cost = cost + FINDER_VERTEX_COST(finder, neighbor, goal);
      }
    }

    astar_nodes[current].state = CLOSED;
  }

fail:
  finder->path_size = 0;
  return;
}

uint8_t pathfinding_nearest_node(const pathfinding_t *finder, int16_t x, int16_t y)
{
  uint32_t dmin = 0xffff;
  uint8_t nearest = 0xff;
  for(uint8_t i=0; i<finder->nodes_size; i++) {
    const int16_t sdx = finder->nodes[i].x - x;
    const int16_t sdy = finder->nodes[i].y - y;
    const uint32_t dx = sdx > 0 ? sdx : -sdx;
    const uint32_t dy = sdy > 0 ? sdy : -sdy;
    uint32_t d = dx*dx + dy*dy;
    if(d < dmin) {
      dmin = d;
      nearest = i;
    }
  }
  return nearest;
}

