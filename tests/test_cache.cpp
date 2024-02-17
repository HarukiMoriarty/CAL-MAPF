#include <lacam.hpp>
#include "gtest/gtest.h"

TEST(Cache, cache_LRU_test)
{
    auto test = spdlog::stderr_color_mt("test_LRU_cache");
    test->set_level(spdlog::level::debug);

    Cache cache(test, CacheType::LRU);

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


    // Set paras
    Vertex* cache_1 = new Vertex(30, 16, 9);
    Vertex* cache_2 = new Vertex(39, 23, 9);
    Vertex* cache_3 = new Vertex(48, 30, 9);

    Vertex* cargo_1 = new Vertex(32, 18, 9);
    Vertex* cargo_2 = new Vertex(33, 19, 9);
    Vertex* cargo_3 = new Vertex(41, 25, 9);
    Vertex* cargo_4 = new Vertex(42, 26, 9);
    Vertex* cargo_5 = new Vertex(50, 32, 9);
    // Vertex* cargo_6 = new Vertex(51, 33, 9);

    Config port_list;
    Vertex* unloading_port = new Vertex(37, 21, 9);
    port_list.push_back(unloading_port);

    cache.node_cargo.push_back(cargo_1);
    cache.node_cargo.push_back(cargo_2);
    cache.node_cargo.push_back(cargo_3);

    cache.node_coming_cargo.push_back(cargo_4);
    cache.node_coming_cargo.push_back(cache_2);
    cache.node_coming_cargo.push_back(cache_3);

    cache.node_id.push_back(cache_1);
    cache.node_id.push_back(cache_2);
    cache.node_id.push_back(cache_3);

    cache.LRU.push_back(3);
    cache.LRU.push_back(2);
    cache.LRU.push_back(1);

    cache.LRU_cnt = 3;

    cache.bit_cache_get_lock.push_back(0);
    cache.bit_cache_get_lock.push_back(0);
    cache.bit_cache_get_lock.push_back(0);

    cache.bit_cache_insert_lock.push_back(0);
    cache.bit_cache_insert_lock.push_back(0);
    cache.bit_cache_insert_lock.push_back(0);

    // Test `_get_cache_block_in_cache_index(Vertex* block)`
    ASSERT_EQ(0, cache._get_cache_block_in_cache_index(cache_1));

    // Test `_get_cargo_in_cache_index(Vertex* cargo)`
    ASSERT_EQ(0, cache._get_cargo_in_cache_index(cargo_1));
    ASSERT_EQ(-1, cache._get_cargo_in_cache_index(cargo_5));

    // Test `_is_cargo_in_coming_cache(Vertex* cargo)`
    ASSERT_EQ(true, cache._is_cargo_in_coming_cache(cargo_4));
    ASSERT_EQ(false, cache._is_cargo_in_coming_cache(cargo_5));

    // Test `try_cache_cargo(Vertex* cargo)`
    // We will get lock block cache_1 with cargo_1
    // LRU_cnt: (4, 2, 1)
    ASSERT_EQ(cache_1, cache.try_cache_cargo(cargo_1));
    ASSERT_EQ(cargo_5, cache.try_cache_cargo(cargo_5));
    ASSERT_EQ(4, cache.LRU[0]);

    // Test `try_insert_cache(Vertex* cargo)`
    // We will insert lock block cache_3 with cargo_5
    ASSERT_EQ(unloading_port, cache.try_insert_cache(cargo_1, port_list));
    ASSERT_EQ(unloading_port, cache.try_insert_cache(cargo_4, port_list));
    ASSERT_EQ(2, cache._get_cache_evited_policy_index());
    ASSERT_EQ(cache_3, cache.try_insert_cache(cargo_5, port_list));
    ASSERT_EQ(cargo_5, cache.node_coming_cargo[2]);

    // Test `update_cargo_into_cache`
    ASSERT_EQ(true, cache.update_cargo_into_cache(cargo_5, cache_3));
    ASSERT_EQ(0, cache.bit_cache_insert_lock[2]);

    // Test `update_cargo_from_cache`
    ASSERT_EQ(true, cache.update_cargo_from_cache(cargo_1, cache_1));
    ASSERT_EQ(0, cache.bit_cache_get_lock[0]);
}

TEST(Cache, cache_FIFO_test)
{
    auto test = spdlog::stderr_color_mt("test_FIFO_cache");
    test->set_level(spdlog::level::debug);

    Cache cache(test, CacheType::FIFO);

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


    // Set paras
    Vertex* cache_1 = new Vertex(30, 16, 9);
    Vertex* cache_2 = new Vertex(39, 23, 9);
    Vertex* cache_3 = new Vertex(48, 30, 9);

    Vertex* cargo_1 = new Vertex(32, 18, 9);
    Vertex* cargo_2 = new Vertex(33, 19, 9);
    Vertex* cargo_3 = new Vertex(41, 25, 9);
    Vertex* cargo_4 = new Vertex(42, 26, 9);
    Vertex* cargo_5 = new Vertex(50, 32, 9);
    // Vertex* cargo_6 = new Vertex(51, 33, 9);

    Config port_list;
    Vertex* unloading_port = new Vertex(37, 21, 9);
    port_list.push_back(unloading_port);


    cache.node_cargo.push_back(cargo_1);
    cache.node_cargo.push_back(cargo_2);
    cache.node_cargo.push_back(cargo_3);

    cache.node_coming_cargo.push_back(cargo_4);
    cache.node_coming_cargo.push_back(cache_2);
    cache.node_coming_cargo.push_back(cache_3);

    cache.node_id.push_back(cache_1);
    cache.node_id.push_back(cache_2);
    cache.node_id.push_back(cache_3);

    cache.FIFO.push_back(3);
    cache.FIFO.push_back(2);
    cache.FIFO.push_back(1);

    cache.FIFO_cnt = 3;

    cache.bit_cache_get_lock.push_back(0);
    cache.bit_cache_get_lock.push_back(0);
    cache.bit_cache_get_lock.push_back(0);

    cache.bit_cache_insert_lock.push_back(0);
    cache.bit_cache_insert_lock.push_back(0);
    cache.bit_cache_insert_lock.push_back(0);

    // Test `_get_cache_block_in_cache_index(Vertex* block)`
    ASSERT_EQ(0, cache._get_cache_block_in_cache_index(cache_1));

    // Test `_get_cargo_in_cache_index(Vertex* cargo)`
    ASSERT_EQ(0, cache._get_cargo_in_cache_index(cargo_1));
    ASSERT_EQ(-1, cache._get_cargo_in_cache_index(cargo_5));

    // Test `_is_cargo_in_coming_cache(Vertex* cargo)`
    ASSERT_EQ(true, cache._is_cargo_in_coming_cache(cargo_4));
    ASSERT_EQ(false, cache._is_cargo_in_coming_cache(cargo_5));

    // Test `try_cache_cargo(Vertex* cargo)`
    // We will get lock block cache_1 with cargo_1
    // FIFO_cnt: (3, 2, 1)
    ASSERT_EQ(cache_1, cache.try_cache_cargo(cargo_1));
    ASSERT_EQ(cargo_5, cache.try_cache_cargo(cargo_5));
    ASSERT_EQ(3, cache.FIFO[0]);

    // Test `try_insert_cache(Vertex* cargo)`
    // We will insert lock block cache_3 with cargo_5
    ASSERT_EQ(unloading_port, cache.try_insert_cache(cargo_1, port_list));
    ASSERT_EQ(unloading_port, cache.try_insert_cache(cargo_4, port_list));
    ASSERT_EQ(2, cache._get_cache_evited_policy_index());
    ASSERT_EQ(cache_3, cache.try_insert_cache(cargo_5, port_list));
    ASSERT_EQ(cargo_5, cache.node_coming_cargo[2]);

    // Test `update_cargo_into_cache`
    ASSERT_EQ(true, cache.update_cargo_into_cache(cargo_5, cache_3));
    ASSERT_EQ(0, cache.bit_cache_insert_lock[2]);

    // Test `update_cargo_from_cache`
    ASSERT_EQ(true, cache.update_cargo_from_cache(cargo_1, cache_1));
    ASSERT_EQ(0, cache.bit_cache_get_lock[0]);
}