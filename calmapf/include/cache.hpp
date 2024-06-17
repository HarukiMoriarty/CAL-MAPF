// Cache definition
// Author: Zhenghong Yu

#pragma once

#include "utils.hpp"
#include "parser.hpp"
#include <cassert>

struct Cache {
    std::vector<Vertices> node_cargo;
    std::vector<Vertices> node_id;
    std::vector<Vertices> node_coming_cargo;
    std::vector<std::vector<uint>> node_cargo_num;
    std::vector<std::vector<uint>> bit_cache_get_lock;
    std::vector<std::vector<uint>> bit_cache_insert_or_clear_lock;
    std::vector<std::vector<bool>> is_empty;

    // LRU paras
    std::vector<std::vector<int>> LRU;
    std::vector<uint> LRU_cnt;

    // FIFO paras
    std::vector<std::vector<int>> FIFO;
    std::vector<uint> FIFO_cnt;

    // Random paras (no paras)

    // Parser
    Parser* parser;

    // Logger
    std::shared_ptr<spdlog::logger> cache_console;

    Cache(Parser* _parser);
    ~Cache();

    /**
     * @brief Update evicted policy statistics
     * @param group cache block group number
     * @param index An index used for lru/fifo policy
     * @param fifo_option An option to control fifo policy
     * @return true if successful, false otherwise
    */
    bool _update_cache_evited_policy_statistics(const uint group, const uint index, const bool fifo_option);

    /**
     * @brief Get index of evicted cache block
     * @param group cache block group number
     * @return true if successful, false otherwise
    */
    int _get_cache_evited_policy_index(const uint group);

    /**
     * @brief Get the index of a specified cache block.
     * @param block A pointer to the Vertex representing the block.
     * @return index of the cache block.
     */
    int _get_cache_block_in_cache_position(Vertex* block);

    /**
     * @brief Check if a specific cargo is cached.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @return index of the cache block, -1 if not find.
     */
    int _get_cargo_in_cache_position(Vertex* cargo);

    /**
     * @brief Check if a specific cargo is coming to cache.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @return true if is coming to cacehe, or false.
    */
    bool _is_cargo_in_coming_cache(Vertex* cargo);

    /**
     * @brief Check if cache need a garbage collection.
     * @param group cache block group number
     * @return true if actually needs, or false.
    */
    bool _is_garbage_collection(int group);

    /**
     * @brief Check if the cargo is in cache. Used for look ahead protocol.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @return true if the cargo is in cache, or false.
    */
    bool look_ahead_cache(Vertex* cargo);

    /**
     * @brief Attempt to find a cached cargo and retrieve associated goals.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @return A CacheAccessResult, true if we find cached cargo, false otherwise.
     */
    CacheAccessResult try_cache_cargo(Vertex* cargo);

    /**
     * @brief Find an empty cache block for cargo that is not cached (cache-miss) and insert.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @param unloading_port A pointer to the unloading port.
     * @return A CacheAccessResult, true if we find one, false otherwise.
    */
    CacheAccessResult try_insert_cache(Vertex* cargo, Vertex* unloading_port);

    /**
     * @brief Attempt to find a garbage to free one cache block.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @return A CacheAccessResult, true if need to do garbage collection
     *         and actually find one to collect, false otherwise.
    */
    CacheAccessResult try_cache_garbage_collection(Vertex* cargo);

    /**
     * @brief Insert cargo into cache. This occurs when an agent brings a
     *        cache-miss cargo back to the cache.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @param cache_node A pointer to the Vertex representing the cache goal.
     * @return true if successful, false otherwise.
     */
    bool update_cargo_into_cache(Vertex* cargo, Vertex* cache_node);

    /**
     * @brief Release lock when retrieving a cached cargo.
     *        This occurs when an agent reaches the cached cargo.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @param cache_node A pointer to the Vertex representing the cache goal.
     * @return true if successful, false otherwise.
     */
    bool update_cargo_from_cache(Vertex* cargo, Vertex* cache_node);

    /**
     * @brief Release lock when clear a cached cargo.
     *        This occurs when an agent reaches the cached garbage cargo.
     * @param cargo A pointer to the Vertex representing the garbage cargo.
     * @param cache_node A pointer to the vertex representing the cache goal.
     * @return true if succecssful, false otherwise.
    */
    bool clear_cargo_from_cache(Vertex* cargo, Vertex* cache_node);
};