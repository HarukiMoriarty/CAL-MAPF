#include <calmapf.hpp>

#include "gtest/gtest.h"

TEST(Instance, state_machine_test)
{
  Parser cache_state_machine_test_parser = Parser("./assets/test/test_instance.map", CacheType::LRU, 1);
  Instance instance(&cache_state_machine_test_parser);

  /* Graph
    TTTTTTTTT
    T.......T
    T.......T
    T..C.HH.T
    TU.C.HH.T
    T..C.HH.T
    T.......T
    T.......T
    TTTTTTTTT
  */

  // Initilization check
  ASSERT_EQ(1, instance.bit_status[0]);
  uint cache_access, cache_hit = 0;
  std::vector<Config> vertex_list;
  Config step;
  Vertex* goal = instance.graph.cargo_vertices[0][0];;

  // Generate goal queue
  instance.goals[0] = goal;
  instance.cargo_goals[0] = goal;
  instance.graph.goals_queue[0].clear();
  instance.graph.goals_queue[0].push_back(instance.graph.cargo_vertices[0][1]);
  instance.graph.goals_queue[0].push_back(instance.graph.cargo_vertices[0][1]);
  instance.graph.goals_delay[0].clear();
  instance.graph.goals_delay[0].push_back(0);
  instance.graph.goals_delay[0].push_back(0);
  instance.instance_console->info("Front goal {}", *(instance.graph.goals_queue[0].front()));


  // Status 1 -> Status 4
  step.push_back(goal);
  vertex_list.push_back(step);

  ASSERT_EQ(0, instance.update_on_reaching_goals_with_cache(vertex_list, 100, cache_access, cache_hit));
  ASSERT_EQ(4, instance.bit_status[0]);

  goal = instance.goals[0];

  // Status 4 -> Status 6
  step.clear();
  vertex_list.clear();
  step.push_back(goal);
  vertex_list.push_back(step);

  ASSERT_EQ(0, instance.update_on_reaching_goals_with_cache(vertex_list, 100, cache_access, cache_hit));
  ASSERT_EQ(6, instance.bit_status[0]);

  goal = instance.goals[0];

  // Status 6 -> Status 0
  step.clear();
  vertex_list.clear();
  step.push_back(goal);
  vertex_list.push_back(step);

  ASSERT_EQ(1, instance.update_on_reaching_goals_with_cache(vertex_list, 100, cache_access, cache_hit));
  ASSERT_EQ(0, instance.bit_status[0]);

  goal = instance.goals[0];

  // Status 0 -> Status 3
  step.clear();
  vertex_list.clear();
  step.push_back(goal);
  vertex_list.push_back(step);

  ASSERT_EQ(0, instance.update_on_reaching_goals_with_cache(vertex_list, 100, cache_access, cache_hit));
  ASSERT_EQ(3, instance.bit_status[0]);

  goal = instance.goals[0];

  // Status 3 -> Status 1
  step.clear();
  vertex_list.clear();
  step.push_back(goal);
  vertex_list.push_back(step);

  ASSERT_EQ(0, instance.update_on_reaching_goals_with_cache(vertex_list, 100, cache_access, cache_hit));
  ASSERT_EQ(1, instance.bit_status[0]);

  goal = instance.goals[0];

  // Status 1 -> Status 4
  step.clear();
  vertex_list.clear();
  step.push_back(goal);
  vertex_list.push_back(step);

  ASSERT_EQ(0, instance.update_on_reaching_goals_with_cache(vertex_list, 100, cache_access, cache_hit));
  ASSERT_EQ(4, instance.bit_status[0]);

  goal = instance.goals[0];

  // Status 4 -> Status 6
  step.clear();
  vertex_list.clear();
  step.push_back(goal);
  vertex_list.push_back(step);

  ASSERT_EQ(0, instance.update_on_reaching_goals_with_cache(vertex_list, 100, cache_access, cache_hit));
  ASSERT_EQ(6, instance.bit_status[0]);

  goal = instance.goals[0];

  // Status 6 -> Status 2
  step.clear();
  vertex_list.clear();
  step.push_back(goal);
  vertex_list.push_back(step);

  ASSERT_EQ(1, instance.update_on_reaching_goals_with_cache(vertex_list, 100, cache_access, cache_hit));
  ASSERT_EQ(2, instance.bit_status[0]);

  goal = instance.goals[0];

  // Status 2 -> Status 6
  step.clear();
  vertex_list.clear();
  step.push_back(goal);
  vertex_list.push_back(step);

  ASSERT_EQ(0, instance.update_on_reaching_goals_with_cache(vertex_list, 100, cache_access, cache_hit));
  ASSERT_EQ(6, instance.bit_status[0]);
}
