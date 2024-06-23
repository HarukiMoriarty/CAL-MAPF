// Parser definition
// Zhenghong Yu
#pragma once

#include <argparse/argparse.hpp>
#include "utils.hpp"

struct Parser {
    // Map settings
    std::string map_file;

    // Cache settings
    std::string cache_type_input;
    CacheType cache_type;

    int look_ahead_num;
    int delay_deadline_limit;

    // Goal settings
    uint num_goals;

    std::string goals_gen_strategy_input;
    GoalGenerationType goals_gen_strategy;

    uint goals_max_k;
    uint goals_max_m;
    std::string real_dist_file_path;

    // Agent settings
    uint num_agents;
    uint agent_capacity;

    // Instance settings
    int random_seed;
    std::mt19937 MT;

    int time_limit_sec;

    // Output settings
    std::string output_step_file;
    std::string output_csv_file;
    std::string output_throughput_file;
    std::string output_visual_file;

    // Log settings
    bool short_log_format;
    bool debug_log;

    // Logger
    std::shared_ptr<spdlog::logger> parser_console;

    // Optimize
    bool optimization;

    Parser(int argc, char* argv[]);
    void _post_parse();
    void _check();
    void _print();

    // Unit test only
    Parser(std::string _map_file, CacheType _cache_type, uint _num_agents = 4);
};