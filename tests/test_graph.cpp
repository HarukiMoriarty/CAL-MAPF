#include <lacam.hpp>
#include "gtest/gtest.h"

TEST(Graph, single_port_load_graph)
{
  const std::string filename = "../assets/test/test-8-8-single_port.map";
  auto test = spdlog::stderr_color_mt("test_single_port");
  auto MT = std::mt19937(0);
  test->set_level(spdlog::level::debug);

  auto G = Graph(filename, test, 10, 5, 10, CacheType::LRU, &MT);

  // Test graph paras
  ASSERT_EQ(G.size(), 36);
  ASSERT_EQ(G.U.size(), 64);
  ASSERT_EQ(G.width, 8);
  ASSERT_EQ(G.height, 8);
  ASSERT_EQ(G.group, 1);
  ASSERT_EQ(G.unloading_ports.size(), 1);
  ASSERT_EQ(G.cargo_vertices.size(), 1);
  ASSERT_EQ(G.cargo_vertices[0].size(), 3);
  ASSERT_EQ(G.cache->node_id.size(), 1);
  ASSERT_EQ(G.cache->node_id[0].size(), 3);
  ASSERT_EQ(G.goals_list.size(), 1);
  ASSERT_EQ(G.goals_list[0].size(), 10);

  // Test normal block
  ASSERT_EQ(G.V[0]->neighbor.size(), 2);
  ASSERT_EQ(G.V[0]->neighbor[0]->id, 1);
  ASSERT_EQ(G.V[0]->neighbor[1]->id, 6);

  // Test unloading port block
  ASSERT_EQ(G.unloading_ports[0]->neighbor.size(), 3);
  ASSERT_EQ(G.unloading_ports[0]->neighbor[0]->id, 13);
  ASSERT_EQ(G.unloading_ports[0]->neighbor[1]->id, 18);
  ASSERT_EQ(G.unloading_ports[0]->neighbor[2]->id, 6);

  // Test cargo block
  ASSERT_EQ(G.cargo_vertices[0][0]->neighbor.size(), 2);
  ASSERT_EQ(G.cargo_vertices[0][0]->neighbor[0]->id, 11);
  ASSERT_EQ(G.cargo_vertices[0][0]->neighbor[1]->id, 4);

  // Test cache block
  ASSERT_EQ(G.cache->node_id[0][0]->neighbor.size(), 2);
  ASSERT_EQ(G.cache->node_id[0][0]->neighbor[0]->id, 8);
  ASSERT_EQ(G.cache->node_id[0][0]->neighbor[1]->id, 3);
}

TEST(Graph, multi_port_load_graph)
{
  const std::string filename = "../assets/test/test-16-16-multi_port.map";
  auto test = spdlog::stderr_color_mt("test_multi_port");
  auto MT = std::mt19937(0);
  test->set_level(spdlog::level::debug);

  auto G = Graph(filename, test, 10, 5, 10, CacheType::LRU, &MT);

  // Test graph paras
  ASSERT_EQ(G.size(), 196);
  ASSERT_EQ(G.U.size(), 256);
  ASSERT_EQ(G.width, 16);
  ASSERT_EQ(G.height, 16);
  ASSERT_EQ(G.group, 2);
  ASSERT_EQ(G.unloading_ports.size(), 2);
  ASSERT_EQ(G.cargo_vertices.size(), 2);
  ASSERT_EQ(G.cargo_vertices[0].size(), 6);
  ASSERT_EQ(G.cargo_vertices[1].size(), 4);
  ASSERT_EQ(G.cache->node_id.size(), 2);
  ASSERT_EQ(G.cache->node_id[0].size(), 6);
  ASSERT_EQ(G.cache->node_id[1].size(), 4);
  ASSERT_EQ(G.goals_list.size(), 2);
  ASSERT_EQ(G.goals_list[0].size(), 10);
  ASSERT_EQ(G.goals_list[1].size(), 10);

  // Test normal block
  ASSERT_EQ(G.V[0]->neighbor.size(), 2);
  ASSERT_EQ(G.V[0]->neighbor[0]->id, 1);
  ASSERT_EQ(G.V[0]->neighbor[1]->id, 14);

  // Test unloading port block
  ASSERT_EQ(G.unloading_ports[0]->neighbor.size(), 3);
  ASSERT_EQ(G.unloading_ports[0]->neighbor[0]->id, 29);
  ASSERT_EQ(G.unloading_ports[0]->neighbor[1]->id, 42);
  ASSERT_EQ(G.unloading_ports[0]->neighbor[2]->id, 14);

  // Test cargo block
  ASSERT_EQ(G.cargo_vertices[0][0]->neighbor.size(), 2);
  ASSERT_EQ(G.cargo_vertices[0][0]->neighbor[0]->id, 21);
  ASSERT_EQ(G.cargo_vertices[0][0]->neighbor[1]->id, 8);

  // Test cache block
  ASSERT_EQ(G.cache->node_id[0][0]->neighbor.size(), 2);
  ASSERT_EQ(G.cache->node_id[0][0]->neighbor[0]->id, 16);
  ASSERT_EQ(G.cache->node_id[0][0]->neighbor[1]->id, 3);
}
