/*
 * instance definition
 */
#pragma once

#include "graph.hpp"
#include "parser.hpp"
#include "utils.hpp"

struct Instance {
  Graph graph;                    // graph
  Config starts;                  // initial configuration
  Config goals;                   // goal configuration, can be in warehouse block/cache block
  Config cargo_goals;             // cargo goal configuration
  std::vector<uint> cargo_cnts;   // each cargo cnts, help variable for cargo_steps
  std::vector<uint> cargo_steps;  // each cargo steps 

  // Status control:
  // 0 -> cache miss, going for warehouse get cargo
  // 1 -> cache hit, going for cache get cargo
  // 2 -> warehouse get cargo, find empty block, going back to insert cache
  // 3 -> warehouse get cargo, cannot find empty block, going back to unloading port
  // 4 -> cache get cargo, going back to unloading port
  std::vector<uint> bit_status;

  std::vector<int> agent_group;   // agents group
  Parser* parser;                 // paras
  std::shared_ptr<spdlog::logger> instance_console;

  // Instructor
  Instance(Parser* parser);
  // Destructor
  ~Instance() {}

  // Assign agent group
  void assign_agent_group();
  // Simple feasibility check of instance
  void _is_valid(int verbose = 0);
  // Check if reached port
  bool is_port(Vertex* port) const;

  // Check agents when reaching goals with cache
  uint update_on_reaching_goals_with_cache(
    std::vector<Config>& vertex_list,
    int remain_goals,
    uint& cache_access,
    uint& cache_hit
  );

  // Check agents when reaching goals without cache
  uint update_on_reaching_goals_without_cache(
    std::vector<Config>& vertex_list,
    int remain_goals
  );

  // Compute percentiles steps
  std::vector<uint> compute_percentiles() const;
};
