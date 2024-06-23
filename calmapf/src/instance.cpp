#include "../include/instance.hpp"

Instance::Instance(Parser* _parser) : graph(Graph(_parser)), parser(_parser)
{
  instance_console = spdlog::stderr_color_mt("instance");
  if (parser->debug_log) instance_console->set_level(spdlog::level::debug);
  else instance_console->set_level(spdlog::level::info);

  const auto K = graph.size();
  assign_agent_group();

  // set agents random start potition
  auto s_indexes = std::vector<int>(K);
  std::iota(s_indexes.begin(), s_indexes.end(), 0);
  std::shuffle(s_indexes.begin(), s_indexes.end(), parser->MT);
  int i = 0;
  while (true) {
    if (i >= K) return;
    starts.push_back(graph.V[s_indexes[i]]);
    if (starts.size() == parser->num_agents) break;
    ++i;
  }

  // set goals
  int j = 0;
  while (true) {
    if (j >= K) return;
    Vertex* goal = graph.get_next_goal(agent_group[j]);
    goals.push_back(goal);
    cargo_goals.push_back(goal);
    garbages.push_back(goal);
    bit_status.push_back(1);      // At the begining, the cache is empty, all agents should at status 1
    cargo_cnts.push_back(0);
    if (goals.size() == parser->num_agents) break;
    ++j;
  }

  // check instance
  _is_valid();
}

void Instance::_is_valid()
{
  if (parser->num_agents != starts.size() || parser->num_agents != goals.size()) {
    instance_console->error("invalid N, check instance nagents {} starts {} goals {}", parser->num_agents, starts.size(), goals.size());
    exit(1);
  }
}

uint Instance::update_on_reaching_goals_with_cache(
  std::vector<Config>& vertex_list,
  int remain_goals,
  uint& cache_access,
  uint& cache_hit)
{
  instance_console->debug("Old Goals: {}", goals);
  instance_console->debug("Remain goals:  {}", remain_goals);
  int step = vertex_list.size() - 1;
  instance_console->debug("Step length:   {}", step);
  instance_console->debug("Solution ends: {}", vertex_list[step]);
  instance_console->debug("Status before: {}", bit_status);
  int reached_count = 0;

  // TODO: assign goals to closed free agents

  // Update steps
  for (size_t i = 0; i < vertex_list[step].size(); i++) {
    cargo_cnts[i] += step;
  }

  // First, we check vertex which status will released lock
  for (size_t j = 0; j < vertex_list[step].size(); ++j) {
    if (vertex_list[step][j] == goals[j]) {
      // Status 0 finished. ==> Status 3
      if (bit_status[j] == 0) {
        instance_console->debug("Agent {} status 0 -> status 3, reached cargo {} at cahe block {}, cleared", j, *garbages[j], *goals[j]);
        bit_status[j] = 3;
        assert(graph.cache->clear_cargo_from_cache(garbages[j], goals[j]));
        goals[j] = garbages[j];
      }
      // Status 2 finished. ==> Status 6
      // Agent has moved to cache cargo target.
      // Update cache lock info, directly move back to unloading port.
      else if (bit_status[j] == 2) {
        instance_console->debug(
          "Agent {} status 2 -> status 6, reach cached cargo {} at cache "
          "block {}, return to unloading port",
          j, *cargo_goals[j], *goals[j]);
        bit_status[j] = 6;
        assert(graph.cache->update_cargo_from_cache(cargo_goals[j], goals[j]));
        // Update goals
        goals[j] = graph.unloading_ports[cargo_goals[j]->group];
      }
      // Status 4 finished. ==> Status 6
      // Agent has bring uncached cargo back to cache.
      // Update cache, move to unloading port.
      else if (bit_status[j] == 4) {
        instance_console->debug(
          "Agent {} status 4 -> status 6, bring cargo {} to cache block "
          "{}, then return to unloading port",
          j, *cargo_goals[j], *goals[j]);
        bit_status[j] = 6;
        assert(graph.cache->update_cargo_into_cache(cargo_goals[j], goals[j]));
        // Update goals
        goals[j] = graph.unloading_ports[cargo_goals[j]->group];
      }
    }
  }

  // Second, we check vertex which status will require (or not require) lock
  for (size_t j = 0; j < vertex_list[step].size(); ++j) {
    if (bit_status[j] == 1) {
      // Status 1 finished.
      if (vertex_list[step][j] == goals[j]) {
        // Agent has moved to warehouse cargo target
        CacheAccessResult result = graph.cache->try_insert_cache(cargo_goals[j], graph.unloading_ports[agent_group[j]]);
        // Cache is full, directly get back to unloading port.
        // ==> Status 5
        if (!result.result) {
          instance_console->debug(
            "Agent {} status 1 -> status 5, reach warehouse cargo {}, cache "
            "is full, go back to unloading port",
            j, *cargo_goals[j]);
          bit_status[j] = 5;
        }
        // Find empty cache block, go and insert cargo into cache.
        // ==> Status 4
        else {
          instance_console->debug(
            "Agent {} status 1 -> status 4, reach warehouse cargo {}, find "
            "cache block to insert, go to cache block {}",
            j, *cargo_goals[j], *result.goal);
          bit_status[j] = 4;
        }
        // Update goals
        goals[j] = result.goal;
      }
      // Status 1 yet not finished
      else {
        // Check if the cargo has been cached during the period
        CacheAccessResult result = graph.cache->try_cache_cargo(cargo_goals[j]);
        if (result.result) {
          // We find cached cargo, go to cache
          // ==> Status 2
          instance_console->debug(
            "Agent {} cache hit while moving to ware house to get cargo. Go to cache {}, status 1 -> status 2",
            j, *cargo_goals[j], *result.goal);
          cache_access++;
          cache_hit++;
          bit_status[j] = 1;
          // Update goals and steps
          goals[j] = result.goal;
        }
        // Otherwise, we do nothing for cache miss situation
      }
    }
    else if (bit_status[j] == 3) {
      // Status 3 finished.
      // Agent has moved trash back to warehouse, going to fetch cargo
      if (vertex_list[step][j] == goals[j]) {
        instance_console->debug("Agent {} status 3 -> status 1, brought trash {} back to warehouse, go to fetch cargo {}", j, *goals[j], *cargo_goals[j]);
        bit_status[j] = 1;
        goals[j] = cargo_goals[j];
      }
    }
    else if (bit_status[j] == 5) {
      // Status 5 finished.
      // Agent has back to unloading port, assigned with new cargo target
      if (vertex_list[step][j] == goals[j]) {
        if (remain_goals > 0) {
          // Update statistics.
          // Otherwise we still let agent go to fetch new cargo, but we do
          // not update the statistics.
          remain_goals--;
          reached_count++;
          // Record finished cargo steps
          cargo_steps.push_back(cargo_cnts[j]);
          cargo_cnts[j] = 0;
        }

        instance_console->debug("Agent {} has bring cargo {} to unloading port", j, *cargo_goals[j]);

        // Generate new cargo goal
        Vertex* cargo = graph.get_next_goal(agent_group[j], parser->look_ahead_num);
        cargo_goals[j] = cargo;
        CacheAccessResult result = graph.cache->try_cache_cargo(cargo);

        // Cache hit, go to cache to get cached cargo
        // ==> Status 2
        if (result.result) {
          instance_console->debug(
            "Agent {} assigned with new cargo {}, cache hit. Go to cache {}, "
            "status 5 -> status 2",
            j, *cargo_goals[j], *result.goal);
          cache_access++;
          cache_hit++;
          bit_status[j] = 1;
          goals[j] = result.goal;
        }
        // Cache miss, go to warehouse to get cargo
        else {
          CacheAccessResult trash_result = graph.cache->try_cache_garbage_collection(cargo);
          if (trash_result.result) {
            // Need to do trash collection ==> Status 0
            instance_console->debug(
              "Agent {} assigned with new cargo {}, cache miss. Need to do trash collection. Go to "
              "clear cache {}, status 5 -> status 0",
              j, *cargo_goals[j], *trash_result.goal);
            garbages[j] = trash_result.garbage;
            cache_access++;
            bit_status[j] = 0;
          }
          else {
            // Directly go to warehouse ==> Status 1
            instance_console->debug(
              "Agent {} assigned with new cargo {}, cache miss. Go to "
              "warehouse, status 5 -> status 1",
              j, *cargo_goals[j]);
            cache_access++;
            bit_status[j] = 1;
          }
          goals[j] = trash_result.goal;
        }
      }
      // Agent has yet not back to unloading port, we check if there is an empty
      // cache block to insert
      else {
        if (parser->optimization) {
          CacheAccessResult result = graph.cache->try_insert_cache(cargo_goals[j], graph.unloading_ports[agent_group[j]]);
          // Check if the cache is available during the period
          if (result.result) {
            instance_console->debug(
              "Agent {} status 3 -> status 4, find cache block to insert during the moving, go to cache block {}",
              j, *cargo_goals[j], *result.goal);
            bit_status[j] = 4;
            goals[j] = result.goal;
          }
        }
      }
    }
    else if (bit_status[j] == 6) {
      // Status 6 finished.
      // We only check status 6 if it is finished
      if (vertex_list[step][j] == goals[j]) {
        if (remain_goals > 0) {
          // Update statistics.
          // Otherwise we still let agent go to fetch new cargo, but we do
          // not update the statistics.
          remain_goals--;
          reached_count++;
          // Record finished cargo steps
          cargo_steps.push_back(cargo_cnts[j]);
          cargo_cnts[j] = 0;
        }

        instance_console->debug("Agent {} has bring cargo {} to unloading port", j, *cargo_goals[j]);

        // Generate new cargo goal
        Vertex* cargo = graph.get_next_goal(agent_group[j], parser->look_ahead_num);
        cargo_goals[j] = cargo;
        CacheAccessResult result = graph.cache->try_cache_cargo(cargo);

        // Cache hit, go to cache to get cached cargo
        // ==> Status 2
        if (result.result) {
          instance_console->debug(
            "Agent {} assigned with new cargo {}, cache hit. Go to cache {}, "
            "status 6 -> status 2",
            j, *cargo_goals[j], *result.goal);
          cache_access++;
          cache_hit++;
          bit_status[j] = 2;
          goals[j] = result.goal;
        }
        // Cache miss, go to warehouse to get cargo
        else {
          CacheAccessResult trash_result = graph.cache->try_cache_garbage_collection(cargo);
          if (trash_result.result) {
            // Need to do trash collection ==> Status 0
            instance_console->debug(
              "Agent {} assigned with new cargo {}, cache miss. Need to do trash collection. Go to "
              "clear cache {}, status 6 -> status 0",
              j, *cargo_goals[j], *trash_result.goal);
            garbages[j] = trash_result.garbage;
            cache_access++;
            bit_status[j] = 0;
          }
          else {
            // Directly go to warehouse ==> Status 1
            instance_console->debug(
              "Agent {} assigned with new cargo {}, cache miss. Go to "
              "warehouse, status 6 -> status 1",
              j, *cargo_goals[j]);
            cache_access++;
            bit_status[j] = 1;
          }
          goals[j] = trash_result.goal;
        }
      }
    }
  }

  starts = vertex_list[step];
  instance_console->debug("Ends: {}", vertex_list[step]);
  instance_console->debug("New Goals: {}", goals);
  instance_console->debug("Status after: {}", bit_status);
  return reached_count;
}

uint Instance::update_on_reaching_goals_without_cache(
  std::vector<Config>& vertex_list,
  int remain_goals)
{
  instance_console->debug("Remain goals:  {}", remain_goals);
  int step = vertex_list.size() - 1;
  instance_console->debug("Step length:   {}", step);
  instance_console->debug("Solution ends: {}", vertex_list[step]);
  int reached_count = 0;

  for (size_t j = 0; j < vertex_list[step].size(); ++j) {
    // Update steps
    cargo_cnts[j] += step;
    if (vertex_list[step][j] == goals[j]) {
      if (is_port(goals[j])) {
        if (remain_goals > 0) {
          // Update statistics.
          // Otherwise we still let agent go to fetch new cargo, but we do
          // not update the statistics.
          remain_goals--;
          reached_count++;
          // Recorded finished cargo steps
          cargo_steps.push_back(cargo_cnts[j]);
          cargo_cnts[j] = 0;
        }
        Vertex* cargo = graph.get_next_goal(agent_group[j]);
        goals[j] = cargo;
        cargo_goals[j] = cargo;
      }
      else {
        goals[j] = graph.unloading_ports[cargo_goals[j]->group];
      }
    }
  }

  starts = vertex_list[step];
  instance_console->debug("Ends: {}", starts);
  return reached_count;
}

std::vector<uint> Instance::compute_percentiles() const {
  instance_console->debug("cargo step size: {}", cargo_steps.size());
  std::vector<uint> sorted_steps(cargo_steps);

  // Calculate indices for the percentiles
  auto idx = [&](double p) -> size_t {
    return static_cast<size_t>(std::floor((p * sorted_steps.size()) / 100.0));
    };

  std::vector<uint> percentiles;
  std::vector<double> required_percentiles = { 0, 25, 50, 75, 90, 95, 99, 100 };
  for (double p : required_percentiles) {
    size_t i = idx(p);
    // Partially sort to find the ith smallest element (percentile)
    std::nth_element(sorted_steps.begin(), sorted_steps.begin() + i, sorted_steps.end());
    percentiles.push_back(sorted_steps[i]);
  }

  return percentiles;
}

bool Instance::is_port(Vertex* vertex) const {
  if (std::find(graph.unloading_ports.begin(), graph.unloading_ports.end(), vertex) != graph.unloading_ports.end()) {
    return true;
  }
  else return false;
}

void Instance::assign_agent_group() {
  // Ensure nagents is a multiple of ngroups
  assert(parser->num_agents % graph.group == 0);
  int per_group_agent = parser->num_agents / graph.group;
  agent_group.resize(parser->num_agents);

  for (uint i = 0; i < parser->num_agents; ++i) {
    // Assign each agent to a group
    agent_group[i] = i / per_group_agent;
  }
}