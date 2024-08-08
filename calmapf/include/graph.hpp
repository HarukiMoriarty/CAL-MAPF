/*
 * graph definition
 */
#pragma once
#include "utils.hpp"
#include "parser.hpp"
#include "cache.hpp"
#include <map>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/discrete_distribution.hpp>

struct Graph {
  Vertices V;                                 // without nullptr
  Vertices U;                                 // with nullptr, i.e., |U| = width * height
  Vertices unloading_ports;                   // unloading port
  Cache* cache;                               // cache
  std::vector<Vertices> cargo_vertices;
  std::vector<Goals> goals_queue;             // goals queue: length [ngoals], maximum [k] different goals in any [m] length sublist 
  std::vector<std::deque<int>> goals_delay;   // goals delay: prevent cargos is delayed by look ahead 

  int width;                                  // grid width
  int height;                                 // grid height
  int group;                                  // group number
  GraphType type;                             // graph type

  Parser* parser;
  std::shared_ptr<spdlog::logger> graph_console;

  // Instructor with cache
  Graph(Parser* parser);
  // Destructor
  ~Graph();

  GraphType get_graph_type(std::string type);
  int size() const;                       // the number of vertices, |V|
  Vertex* random_target_vertex(int group);
  void _fill_goals_list(int group);
  Vertex* get_next_goal(int group, int look_ahead = 1);
};

bool is_same_config(const Config& C1, const Config& C2);          // Check equivalence of two configurations
bool is_reach_at_least_one(const Config& C1, const Config& C2);   // Check if the solution reached at least one goal

// hash function of configuration
// c.f.
// https://stackoverflow.com/questions/10405030/c-unordered-map-fail-when-used-with-a-vector-as-key
struct ConfigHasher {
  uint operator()(const Config& C) const;
};
