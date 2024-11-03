// Cache implementation
// Author: Zhenghong Yu

#include "../include/cache.hpp"

Cache::Cache(Parser* _parser) : parser(_parser) {
    // Set up logger
    if (auto existing_console = spdlog::get("cache"); existing_console != nullptr) cache_console = existing_console;
    else cache_console = spdlog::stderr_color_mt("cache");
    if (parser->debug_log) cache_console->set_level(spdlog::level::debug);
    else cache_console->set_level(spdlog::level::info);
};

Cache::~Cache() {};

bool Cache::_update_cache_evited_policy_statistics(const uint group, const uint index, const bool fifo_option) {
    switch (parser->cache_type) {
    case CacheType::LRU:
        LRU_cnt[group] = LRU_cnt[group] + 1;
        LRU[group][index] = LRU_cnt[group];
        break;
    case CacheType::FIFO:
        if (fifo_option) {
            FIFO_cnt[group] = FIFO_cnt[group] + 1;
            FIFO[group][index] = FIFO_cnt[group];
        }
        break;
    case CacheType::SIEVE:
        if (fifo_option) {
            SIEVE[group].enqueue(index);
        }
        else {
            SIEVE_ref_bit[group][index] = true;
        }
    case CacheType::RANDOM:
        break;
    default:
        cache_console->error("Unreachable cache state!");
        exit(1);
    }

    return true;
}

int Cache::_get_cache_evited_policy_index(const uint group) {
    // LRU, FIFO paras
    int min_value = -1;
    int min_index = -1;

    // RANDOM paras
    int index = -1;
    std::vector<uint> candidate;

    switch (parser->cache_type) {
    case CacheType::LRU:
        for (uint i = 0; i < LRU[group].size(); i++) {
            // If it's not locked and (it's the first element or the smallest so far)
            if (bit_cache_insert_or_clear_lock[group][i] == 0 && bit_cache_get_lock[group][i] == 0 && (min_value == -1 || LRU[group][i] < min_value)) {
                min_value = LRU[group][i];
                min_index = i;
            }
        }
        return min_index;
    case CacheType::FIFO:
        for (uint i = 0; i < FIFO[group].size(); i++) {
            // If it's not blocked and (it's the first element or the smallest so far)
            if (bit_cache_insert_or_clear_lock[group][i] == 0 && bit_cache_get_lock[group][i] == 0 && (min_value == -1 || FIFO[group][i] < min_value)) {
                min_value = FIFO[group][i];
                min_index = i;
            }
        }
        return min_index;
    case CacheType::SIEVE: {
        uint max_find_times = 2 * SIEVE[group].size();
        while (max_find_times > 0) {
            uint SIEVE_index = SIEVE[group].at(SIEVE_hand[group]);
            if (bit_cache_insert_or_clear_lock[group][SIEVE_index] == 0 && bit_cache_get_lock[group][SIEVE_index] == 0 && !SIEVE_ref_bit[group][SIEVE_index]) {
                SIEVE[group].remove(SIEVE_hand[group]);
                SIEVE_hand[group] = (SIEVE_hand[group]) % SIEVE[group].size();
                return SIEVE_index;
            }
            else {
                SIEVE_ref_bit[group][SIEVE_index] = false;
                SIEVE_hand[group] = (SIEVE_hand[group]) % SIEVE[group].size();

            }
            max_find_times--;
        }
        return -1;
    }
    case CacheType::RANDOM:
        for (uint i = 0; i < node_id[group].size(); i++) {
            // If it's not blocked
            if (bit_cache_insert_or_clear_lock[group][i] == 0 && bit_cache_get_lock[group][i] == 0) {
                candidate.push_back(i);
            }
        }

        if (candidate.empty()) {
            return index;
        }

        index = get_random_int(&parser->MT, 0, candidate.size() - 1);
        return candidate[index];
    default:
        cache_console->error("Unreachable cache state!");
        exit(1);
    }
}

int Cache::_get_cache_block_in_cache_position(Vertex* block) {
    int index = -1;
    for (uint i = 0; i < node_id[block->group].size(); i++) {
        if (node_id[block->group][i] == block) {
            index = i;
            break;
        }
    }
    // Cache goals must in cache
    assert(index != -1);
    return index;
}

int Cache::_get_cargo_in_cache_position(Vertex* cargo) {
    int index = -2;
    for (uint i = 0; i < node_cargo[cargo->group].size(); i++) {
        if (node_cargo[cargo->group][i] == cargo) {
            if (node_cargo_num[cargo->group][i] > 0) {
                index = i;
                break;
            }
            else {
                index = -1;
                break;
            }
        }
    }
    return index;
}

bool Cache::_is_cargo_in_coming_cache(Vertex* cargo) {
    for (uint i = 0; i < node_coming_cargo[cargo->group].size(); i++) {
        if (node_coming_cargo[cargo->group][i] == cargo) {
            return true;
        }
    }
    return false;
}

bool Cache::_is_garbage_collection(int group) {
    for (uint i = 0; i < is_empty[group].size(); i++) {
        if (is_empty[group][i]) {
            cache_console->debug("No need garbage collection");
            return false;
        }
    }
    cache_console->debug("Need garbage collection");
    return true;
}

bool Cache::look_ahead_cache(Vertex* cargo) {
    int cache_index = _get_cargo_in_cache_position(cargo);
    if (cache_index >= 0 && bit_cache_insert_or_clear_lock[cargo->group][cache_index] == 0) return true;
    return false;
}

CacheAccessResult Cache::try_cache_cargo(Vertex* cargo) {
    int group = cargo->group;
    int cache_index = _get_cargo_in_cache_position(cargo);

    // If we can find cargo cached, is not reserved to be replaced and is not reserved to be cleared, we go to cache and get it
    if (cache_index >= 0 && bit_cache_insert_or_clear_lock[group][cache_index] == 0) {
        cache_console->debug("Cache hit! Agent will go {} to get cargo {}", *node_id[group][cache_index], *cargo);
        // For here, we allow multiple agents lock on cache get position
        // It is impossible that a coming agent move cargo to this 
        // position while the cargo has already here
        bit_cache_get_lock[group][cache_index] += 1;
        // We also update cache evicted policy statistics
        _update_cache_evited_policy_statistics(group, cache_index, false);
        // Update cargo number
        node_cargo_num[group][cache_index] -= 1;

        return CacheAccessResult(true, node_id[group][cache_index]);
    }

    // If we cannot find cargo cached, we directly go to warehouse
    // cache_console->debug("Cache miss! Agent will directly to get cargo {}", *cargo);
    return CacheAccessResult(false, cargo);
}

CacheAccessResult Cache::try_insert_cache(Vertex* cargo, Vertex* unloading_port) {
    int group = cargo->group;

    // First, if cargo has already cached or is coming on the way, we directly go
    // to unloading port, for simplify, we just check cache group here
    if (_get_cargo_in_cache_position(cargo) != -2 || _is_cargo_in_coming_cache(cargo)) return CacheAccessResult(false, unloading_port);

    // Second try to find a empty position to insert cargo
    for (uint i = 0; i < is_empty[group].size(); i++) {
        if (is_empty[group][i]) {
            cache_console->debug("Find an empty cache block with index {} {} to insert", i, *node_id[group][i]);
            // We lock this position and update LRU info
            bit_cache_insert_or_clear_lock[group][i] += 1;
            // Update coming cargo info
            node_coming_cargo[group][i] = cargo;
            // Update cache evited policy statistics
            _update_cache_evited_policy_statistics(group, i, true);
            // Set the position to be used
            is_empty[group][i] = false;
            return CacheAccessResult(true, node_id[group][i]);
        }
    }

    // There is no empty block, we can not insert into cache
    return CacheAccessResult(false, unloading_port);
}

CacheAccessResult Cache::try_cache_garbage_collection(Vertex* cargo) {
    int group = cargo->group;
    if (_is_garbage_collection(group)) {
        // Try to find a LRU position that is not locked
        int index = _get_cache_evited_policy_index(group);

        // If we can find one, return the position
        if (index != -1) {
            // We lock this position
            bit_cache_insert_or_clear_lock[group][index] += 1;
            return CacheAccessResult(true, node_id[group][index], node_cargo[group][index]);
        }

        return CacheAccessResult(false, cargo);
    }
    else {
        return CacheAccessResult(false, cargo);
    }
}

bool Cache::update_cargo_into_cache(Vertex* cargo, Vertex* cache_node) {
    int cargo_index = _get_cargo_in_cache_position(cargo);
    int cache_index = _get_cache_block_in_cache_position(cache_node);

    // We should only update it while it is not in cache
    assert(cargo_index == -2);
    assert(_is_cargo_in_coming_cache(cargo));

    // Update cache
    cache_console->debug("Update cargo {} to cache block {}", *cargo, *cache_node);
    node_cargo[cache_node->group][cache_index] = cargo;
    bit_cache_insert_or_clear_lock[cache_node->group][cache_index] -= 1;
    node_cargo_num[cache_node->group][cache_index] = parser->agent_capacity - 1;
    // Set it as not empty
    is_empty[cache_node->group][cache_index] = false;
    return true;
}

bool Cache::update_cargo_from_cache(Vertex* cargo, Vertex* cache_node) {
    int cargo_index = _get_cargo_in_cache_position(cargo);
    int cache_index = _get_cache_block_in_cache_position(cache_node);

    // We must make sure the cargo is still in the cache
    assert(cargo_index != -2);

    // Simply release lock
    cache_console->debug("Agents gets {} from cache {}", *cargo, *cache_node);
    bit_cache_get_lock[cache_node->group][cache_index] -= 1;

    // If the cache block has no more cargoes and is not locked, set it as empty
    if (bit_cache_get_lock[cache_node->group][cache_index] == 0 && node_cargo_num[cache_node->group][cache_index] == 0)
    {
        is_empty[cache_node->group][cache_index] = true;
    }

    return true;
}

bool Cache::clear_cargo_from_cache(Vertex* cargo, Vertex* cache_node) {
    int cargo_index = _get_cargo_in_cache_position(cargo);
    int cache_index = _get_cache_block_in_cache_position(cache_node);

    // We must make sure the cargo is still in the cache
    assert(cargo_index != -2);

    // Simply release lock and set cache block as empty
    cache_console->debug("Agents clear {} from cache {}", *cargo, *cache_node);
    bit_cache_insert_or_clear_lock[cache_node->group][cache_index] -= 1;
    is_empty[cache_node->group][cache_index] = true;

    return true;
}

