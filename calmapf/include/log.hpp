/*
 * post processing, e.g., calculating solution quality
 */
#pragma once
#include "dist_table.hpp"
#include "instance.hpp"
#include "parser.hpp"
#include "utils.hpp"

struct Log {
    // File output handler
    std::ofstream throughput_output_handler;
    std::ofstream csv_output_handler;
    std::ofstream step_output_handler;
    std::ofstream visual_output_handler;

    Solution step_solution;
    Solution life_long_solution;
    std::vector<std::vector<uint>> bit_status_log;
    std::shared_ptr<spdlog::logger> log_console;

    Log(Parser* parser);
    ~Log();

    bool update_solution(Solution& solution, std::vector<uint> bit_status);
    bool is_feasible_solution(const Instance& ins, const int verbose = 0);
    int get_makespan();
    int get_path_cost(int i);  // single-agent path cost
    int get_sum_of_costs();
    int get_sum_of_loss();
    int get_makespan_lower_bound(const Instance& ins, DistTable& D);
    int get_sum_of_costs_lower_bound(const Instance& ins, DistTable& D);
    void print_stats(const int verbose, const Instance& ins, const double comp_time_ms);
    void make_step_log(const Instance& ins, const std::string& output_name, const double comp_time_ms, const std::string& map_name, const int seed, const bool log_short = false);
    void make_life_long_log(const Instance& ins, std::string visual_name);
    void make_throughput_log(uint index, uint start_cnt, uint make_span);
    void make_csv_log(double cache_hit_rate, uint make_span, std::vector<uint>* step_percentiles, bool failure);
};