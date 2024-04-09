#include <calmapf.hpp>
#include "gtest/gtest.h"

TEST(Cache, cache_LRU_single_port_test)
{
    Parser cache_LRU_single_port_test_parser = Parser("", CacheType::LRU);
    Cache cache(&cache_LRU_single_port_test_parser);

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
    Vertex* cache_1 = new Vertex(30, 16, 9, 0);
    Vertex* cache_2 = new Vertex(39, 23, 9, 0);
    Vertex* cache_3 = new Vertex(48, 30, 9, 0);

    Vertex* cargo_1 = new Vertex(32, 18, 9, 0);
    Vertex* cargo_2 = new Vertex(33, 19, 9, 0);
    Vertex* cargo_3 = new Vertex(41, 25, 9, 0);
    Vertex* cargo_4 = new Vertex(42, 26, 9, 0);
    Vertex* cargo_5 = new Vertex(50, 32, 9, 0);
    // Vertex* cargo_6 = new Vertex(51, 33, 9);

    Config port_list;
    Vertex* unloading_port = new Vertex(37, 21, 9, 0);
    port_list.push_back(unloading_port);

    Vertices tmp_cache_node_cargo;
    tmp_cache_node_cargo.push_back(cargo_1);
    tmp_cache_node_cargo.push_back(cargo_2);
    tmp_cache_node_cargo.push_back(cargo_3);
    cache.node_cargo.push_back(tmp_cache_node_cargo);

    Vertices tmp_cache_node_coming_cargo;
    tmp_cache_node_coming_cargo.push_back(cargo_4);
    tmp_cache_node_coming_cargo.push_back(cargo_2);
    tmp_cache_node_coming_cargo.push_back(cargo_3);
    cache.node_coming_cargo.push_back(tmp_cache_node_coming_cargo);

    Vertices tmp_cache_node_id;
    tmp_cache_node_id.push_back(cache_1);
    tmp_cache_node_id.push_back(cache_2);
    tmp_cache_node_id.push_back(cache_3);
    cache.node_id.push_back(tmp_cache_node_id);

    std::vector<bool> tmp_cache_is_empty;
    tmp_cache_is_empty.push_back(false);
    tmp_cache_is_empty.push_back(false);
    tmp_cache_is_empty.push_back(false);
    cache.is_empty.push_back(tmp_cache_is_empty);

    std::vector<int> tmp_cache_lru;
    tmp_cache_lru.push_back(3);
    tmp_cache_lru.push_back(2);
    tmp_cache_lru.push_back(1);
    cache.LRU.push_back(tmp_cache_lru);

    cache.LRU_cnt.push_back(3);

    std::vector<uint> tmp_cache_bit_cache_get_lock;
    tmp_cache_bit_cache_get_lock.push_back(0);
    tmp_cache_bit_cache_get_lock.push_back(0);
    tmp_cache_bit_cache_get_lock.push_back(0);
    cache.bit_cache_get_lock.push_back(tmp_cache_bit_cache_get_lock);

    std::vector<uint> tmp_cache_bit_cache_insert_lock;
    tmp_cache_bit_cache_insert_lock.push_back(0);
    tmp_cache_bit_cache_insert_lock.push_back(0);
    tmp_cache_bit_cache_insert_lock.push_back(0);
    cache.bit_cache_insert_lock.push_back(tmp_cache_bit_cache_insert_lock);

    // Test `_get_cache_block_in_cache_index(Vertex* block)`
    ASSERT_EQ(0, cache._get_cache_block_in_cache_position(cache_1));

    // Test `_get_cargo_in_cache_index(Vertex* cargo)`
    ASSERT_EQ(0, cache._get_cargo_in_cache_position(cargo_1));
    ASSERT_EQ(-1, cache._get_cargo_in_cache_position(cargo_5));

    // Test `_is_cargo_in_coming_cache(Vertex* cargo)`
    ASSERT_EQ(true, cache._is_cargo_in_coming_cache(cargo_4));
    ASSERT_EQ(false, cache._is_cargo_in_coming_cache(cargo_5));

    // Test `try_cache_cargo(Vertex* cargo)`
    // We will get lock block cache_1 with cargo_1
    // LRU_cnt: (4, 2, 1)
    ASSERT_EQ(cache_1, cache.try_cache_cargo(cargo_1));
    ASSERT_EQ(cargo_5, cache.try_cache_cargo(cargo_5));
    ASSERT_EQ(4, cache.LRU[0][0]);

    // Test `try_insert_cache(Vertex* cargo)`
    // We will insert lock block cache_3 with cargo_5
    ASSERT_EQ(unloading_port, cache.try_insert_cache(cargo_1, port_list));
    ASSERT_EQ(unloading_port, cache.try_insert_cache(cargo_4, port_list));
    ASSERT_EQ(2, cache._get_cache_evited_policy_index(0));
    ASSERT_EQ(cache_3, cache.try_insert_cache(cargo_5, port_list));
    ASSERT_EQ(cargo_5, cache.node_coming_cargo[0][2]);

    // Test `update_cargo_into_cache`
    ASSERT_EQ(true, cache.update_cargo_into_cache(cargo_5, cache_3));
    ASSERT_EQ(0, cache.bit_cache_insert_lock[0][2]);

    // Test `update_cargo_from_cache`
    ASSERT_EQ(true, cache.update_cargo_from_cache(cargo_1, cache_1));
    ASSERT_EQ(0, cache.bit_cache_get_lock[0][0]);
}

TEST(Cache, cache_FIFO_single_port_test)
{
    Parser cache_FIFO_single_port_test = Parser("", CacheType::FIFO);
    Cache cache(&cache_FIFO_single_port_test);

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
    Vertex* cache_1 = new Vertex(30, 16, 9, 0);
    Vertex* cache_2 = new Vertex(39, 23, 9, 0);
    Vertex* cache_3 = new Vertex(48, 30, 9, 0);

    Vertex* cargo_1 = new Vertex(32, 18, 9, 0);
    Vertex* cargo_2 = new Vertex(33, 19, 9, 0);
    Vertex* cargo_3 = new Vertex(41, 25, 9, 0);
    Vertex* cargo_4 = new Vertex(42, 26, 9, 0);
    Vertex* cargo_5 = new Vertex(50, 32, 9, 0);
    // Vertex* cargo_6 = new Vertex(51, 33, 9);

    Config port_list;
    Vertex* unloading_port = new Vertex(37, 21, 9, 0);
    port_list.push_back(unloading_port);

    Vertices tmp_cache_node_cargo;
    tmp_cache_node_cargo.push_back(cargo_1);
    tmp_cache_node_cargo.push_back(cargo_2);
    tmp_cache_node_cargo.push_back(cargo_3);
    cache.node_cargo.push_back(tmp_cache_node_cargo);

    Vertices tmp_cache_node_coming_cargo;
    tmp_cache_node_coming_cargo.push_back(cargo_4);
    tmp_cache_node_coming_cargo.push_back(cargo_2);
    tmp_cache_node_coming_cargo.push_back(cargo_3);
    cache.node_coming_cargo.push_back(tmp_cache_node_coming_cargo);

    Vertices tmp_cache_node_id;
    tmp_cache_node_id.push_back(cache_1);
    tmp_cache_node_id.push_back(cache_2);
    tmp_cache_node_id.push_back(cache_3);
    cache.node_id.push_back(tmp_cache_node_id);

    std::vector<bool> tmp_cache_is_empty;
    tmp_cache_is_empty.push_back(false);
    tmp_cache_is_empty.push_back(false);
    tmp_cache_is_empty.push_back(false);
    cache.is_empty.push_back(tmp_cache_is_empty);

    std::vector<int> tmp_cache_fifo;
    tmp_cache_fifo.push_back(3);
    tmp_cache_fifo.push_back(2);
    tmp_cache_fifo.push_back(1);
    cache.FIFO.push_back(tmp_cache_fifo);

    cache.FIFO_cnt.push_back(3);

    std::vector<uint> tmp_cache_bit_cache_get_lock;
    tmp_cache_bit_cache_get_lock.push_back(0);
    tmp_cache_bit_cache_get_lock.push_back(0);
    tmp_cache_bit_cache_get_lock.push_back(0);
    cache.bit_cache_get_lock.push_back(tmp_cache_bit_cache_get_lock);

    std::vector<uint> tmp_cache_bit_cache_insert_lock;
    tmp_cache_bit_cache_insert_lock.push_back(0);
    tmp_cache_bit_cache_insert_lock.push_back(0);
    tmp_cache_bit_cache_insert_lock.push_back(0);
    cache.bit_cache_insert_lock.push_back(tmp_cache_bit_cache_insert_lock);

    // Test `_get_cache_block_in_cache_index(Vertex* block)`
    ASSERT_EQ(0, cache._get_cache_block_in_cache_position(cache_1));

    // Test `_get_cargo_in_cache_index(Vertex* cargo)`
    ASSERT_EQ(0, cache._get_cargo_in_cache_position(cargo_1));
    ASSERT_EQ(-1, cache._get_cargo_in_cache_position(cargo_5));

    // Test `_is_cargo_in_coming_cache(Vertex* cargo)`
    ASSERT_EQ(true, cache._is_cargo_in_coming_cache(cargo_4));
    ASSERT_EQ(false, cache._is_cargo_in_coming_cache(cargo_5));

    // Test `try_cache_cargo(Vertex* cargo)`
    // We will get lock block cache_1 with cargo_1
    // FIFO_cnt: (3, 2, 1)
    ASSERT_EQ(cache_1, cache.try_cache_cargo(cargo_1));
    ASSERT_EQ(cargo_5, cache.try_cache_cargo(cargo_5));
    ASSERT_EQ(3, cache.FIFO[0][0]);

    // Test `try_insert_cache(Vertex* cargo)`
    // We will insert lock block cache_3 with cargo_5
    ASSERT_EQ(unloading_port, cache.try_insert_cache(cargo_1, port_list));
    ASSERT_EQ(unloading_port, cache.try_insert_cache(cargo_4, port_list));
    ASSERT_EQ(2, cache._get_cache_evited_policy_index(0));
    ASSERT_EQ(cache_3, cache.try_insert_cache(cargo_5, port_list));
    ASSERT_EQ(cargo_5, cache.node_coming_cargo[0][2]);

    // Test `update_cargo_into_cache`
    ASSERT_EQ(true, cache.update_cargo_into_cache(cargo_5, cache_3));
    ASSERT_EQ(0, cache.bit_cache_insert_lock[0][2]);

    // Test `update_cargo_from_cache`
    ASSERT_EQ(true, cache.update_cargo_from_cache(cargo_1, cache_1));
    ASSERT_EQ(0, cache.bit_cache_get_lock[0][0]);
}