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
    std::vector<std::vector<uint>> bit_cache_insert_lock;
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
     * @brief Check if the cargo is in cache. Used for look ahead protocol.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @return true if the cargo is in cache, or false.
    */
    bool look_ahead_cache(Vertex* cargo);

    /**
     * @brief Attempt to find a cached cargo and retrieve associated goals.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @return A pointer to the Vertex of the cargo in the cache,
     *         or to the Vertex in the warehouse if not cached.
     */
    Vertex* try_cache_cargo(Vertex* cargo);

    /**
     * @brief Find a cache block for cargo that is not cached (cache-miss) and set goals.
     *        This is triggered when a cargo cache miss occurs.
     * @param cargo A pointer to the Vertex representing the cargo.
     * @param port_list A pointer to the vector of Vertex representing the unloading ports.
     * @return A pointer to the Vertex representing the cache block, or
     *         the unloading port Vertex if a suitable block cannot be found or
     *         if the cargo is already cached (in a multi-agent context).
     */
    Vertex* try_insert_cache(Vertex* cargo, std::vector<Vertex*> port_list);

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

};