// Cache implementation
// Author: Zhenghong Yu

#include "../include/cache.hpp"

Cache::Cache(
    std::shared_ptr<spdlog::logger> _logger,
    CacheType _cache_type,
    std::mt19937* _randomSeed) :
    logger(std::move(_logger)),
    cache_type(_cache_type),
    randomSeed(_randomSeed) {};

Cache::~Cache() {};

bool Cache::_update_cache_evited_policy_statistics(const uint group, const uint index, const bool fifo_option) {
    switch (cache_type) {
    case CacheType::LRU:
        LRU_cnt[group] += 1;
        LRU[group][index] = LRU_cnt[group];
        break;
    case CacheType::FIFO:
        if (fifo_option) {
            FIFO_cnt[group] += 1;
            FIFO[group][index] = FIFO_cnt[group];
        }
        break;
    case CacheType::RANDOM:
        break;
    default:
        logger->error("Unreachable cache state!");
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
    switch (cache_type) {
    case CacheType::LRU:
        for (uint i = 0; i < LRU.size(); i++) {
            // If it's not locked and (it's the first element or the smallest so far)
            if (bit_cache_insert_lock[group][i] == 0 && bit_cache_get_lock[group][i] == 0 && (min_value == -1 || LRU[group][i] < min_value)) {
                min_value = LRU[group][i];
                min_index = i;
            }
        }
        return min_index;
    case CacheType::FIFO:
        for (uint i = 0; i < FIFO.size(); i++) {
            // If it's not blocked and (it's the first element or the smallest so far)
            if (bit_cache_insert_lock[group][i] == 0 && bit_cache_get_lock[group][i] == 0 && (min_value == -1 || FIFO[group][i] < min_value)) {
                min_value = FIFO[group][i];
                min_index = i;
            }
        }
        return min_index;
    case CacheType::RANDOM:
        for (uint i = 0; i < node_id.size(); i++) {
            // If it's not blocked
            if (bit_cache_insert_lock[group][i] == 0 && bit_cache_get_lock[group][i] == 0) {
                candidate.push_back(i);
            }
        }

        if (candidate.empty()) {
            return index;
        }

        index = get_random_int(randomSeed, 0, candidate.size() - 1);
        return candidate[index];
    default:
        logger->error("Unreachable cache state!");
        exit(1);
    }
}

std::vector<uint> Cache::_get_cache_block_in_cache_position(Vertex* block) {
    std::vector<uint> position;
    for (uint i = 0; i < node_id.size(); i++) {
        for (uint j = 0; j < node_id[i].size(); j++) {
            if (node_id[i][j] == block) {
                position.push_back(i);
                position.push_back(j);
                break;
            }
        }
    }
    // Cache goals must in cache
    assert(position.size() == 2);
    return position;
}

std::vector<int> Cache::_get_cargo_in_cache_position(Vertex* cargo) {
    std::vector<int> position;
    position.push_back(-1);
    position.push_back(-1);
    for (uint i = 0; i < node_cargo.size(); i++) {
        for (uint j = 0; j < node_cargo[i].size(); j++) {
            if (node_cargo[i][j] == cargo) {
                position[0] = i;
                position[1] = j;
                break;
            }
        }
    }
    return position;
}

bool Cache::_is_cargo_in_coming_cache(Vertex* cargo) {
    for (uint i = 0; i < node_coming_cargo.size(); i++) {
        for (uint j = 0; j < node_coming_cargo[i].size(); j++) {
            if (node_coming_cargo[i][j] == cargo) {
                return true;
            }
        }
    }
    return false;
}

Vertex* Cache::try_cache_cargo(Vertex* cargo) {
    std::vector<int> cache_position = _get_cargo_in_cache_position(cargo);
    int cache_group = cache_position[0];
    int cache_index = cache_position[1];

    // If we can find cargo cached and is not reserved to be replaced , we go to cache and get it
    if (cache_group != -1 && cache_index != -1 && bit_cache_insert_lock[cache_group][cache_index] == 0) {
        logger->debug("Cache hit! Agent will go {} to get cargo {}", *node_id[cache_group][cache_index], *cargo);
        // For here, we allow multiple agents lock on cache get position
        // It is impossible that a coming agent move cargo to this 
        // position while the cargo has already here
        bit_cache_get_lock[cache_group][cache_index] += 1;
        // We also update cache evicted policy statistics
        _update_cache_evited_policy_statistics(cache_group, cache_index, false);

        return node_id[cache_group][cache_index];
    }

    // If we cannot find cargo cached, we directly go to warehouse
    logger->debug("Cache miss! Agent will directly to get cargo {}", *cargo);
    return cargo;
}

Vertex* Cache::try_insert_cache(uint group, Vertex* cargo, std::vector<Vertex*> port_list) {
    // Calculate closet port
    Vertex* unloading_port = cargo->find_closest_port(port_list);

    // First, if cargo has already cached or is coming on the way, we directly go
    // to unloading port, for simplify, we just check cache group here
    if (_get_cargo_in_cache_position(cargo)[0] != -1 || _is_cargo_in_coming_cache(cargo)) return unloading_port;

    // Second try to find a empty position to insert cargo
    // TODO: optimization, can set a flag to skip this
    for (uint i = 0; i < is_empty[group].size(); i++) {
        if (is_empty[group][i]) {
            logger->debug("Find an empty cache block with index {} {}", i, *node_id[group][i]);
            // We lock this position and update LRU info
            bit_cache_insert_lock[group][i] += 1;
            // Update coming cargo info
            node_coming_cargo[group][i] = cargo;
            // Update cache evited policy statistics
            _update_cache_evited_policy_statistics(group, i, true);
            // Set the position to be used
            is_empty[group][i] = true;
            return node_id[group][i];
        }
    }

    // Third, try to find a LRU position that is not locked
    int index = _get_cache_evited_policy_index(group);

    // If we can find one, return the posititon
    if (index != -1) {
        // We lock this position and update LRU info
        bit_cache_insert_lock[group][index] += 1;
        // Updating coming cargo info
        node_coming_cargo[group][index] = cargo;
        _update_cache_evited_policy_statistics(group, index, true);
        return node_id[group][index];
    }

    // Else we can not insert into cache
    else return unloading_port;
}

bool Cache::update_cargo_into_cache(Vertex* cargo, Vertex* cache_node) {
    std::vector<int> cargo_position = _get_cargo_in_cache_position(cargo);
    int cargo_index = cargo_position[0];
    int cargo_group = cargo_position[1];

    std::vector<uint> cache_position = _get_cache_block_in_cache_position(cache_node);
    uint cache_group = cache_position[0];
    uint cache_index = cache_position[1];

    // We should only update it while it is not in cache
    assert(cargo_index == -1);
    assert(cargo_group == -1);
    assert(_is_cargo_in_coming_cache(cargo));

    // Update cache
    logger->debug("Update cargo {} to cache block {}", *cargo, *cache_node);
    node_cargo[cache_group][cache_index] = cargo;
    bit_cache_insert_lock[cache_group][cache_index] -= 1;
    return true;
}

bool Cache::update_cargo_from_cache(Vertex* cargo, Vertex* cache_node) {
    std::vector<int> cargo_position = _get_cargo_in_cache_position(cargo);
    int cargo_index = cargo_position[0];
    int cargo_group = cargo_position[1];

    std::vector<uint> cache_position = _get_cache_block_in_cache_position(cache_node);
    uint cache_group = cache_position[0];
    uint cache_index = cache_position[1];

    // We must make sure the cargo is still in the cache
    assert(cargo_index != -1);
    assert(cargo_group != -1);

    // Simply release lock
    logger->debug("Agents gets {} from cache {}", *cargo, *cache_node);
    bit_cache_get_lock[cache_group][cache_index] -= 1;
    return true;
}

