#include <argparse/argparse.hpp>
#include <chrono>
#include <calmapf.hpp>

int main(int argc, char* argv[])
{
  auto console = spdlog::stderr_color_mt("console");
  console->set_level(spdlog::level::info);

  Parser parser(argc, argv);

  auto deadline = Deadline(parser.time_limit_sec * 1000);

  // generating instance
  auto ins = Instance(parser.map_file, &parser.MT, console, parser.goals_gen_strategy, parser.real_dist_file_path, parser.cache_type, parser.look_ahead_num, parser.delay_deadline_limit, parser.num_agents, parser.num_goals, parser.goals_max_m, parser.goals_max_k);
  if (!ins.is_valid(1)) {
    console->error("instance is invalid!");
    return 1;
  }

  // initliaze log system
  Log log(console);
  // initliaze info timer
  auto timer = std::chrono::steady_clock::now();

  // prepare for throughput record
  std::ofstream throughput_file(parser.output_throughput_file, std::ios::app);
  if (!throughput_file.is_open()) {
    std::cerr << "Failed to open file: " << parser.output_throughput_file << std::endl;
    return 1;
  }
  throughput_file << parser.map_file << ","
    << parser.cache_type_input << ","
    << parser.goals_gen_strategy_input << ","
    << parser.num_goals << ","
    << parser.num_agents << ","
    << parser.random_seed << ","
    << parser.verbose_level << ","
    << parser.time_limit_sec << ","
    << parser.goals_max_m << ","
    << parser.goals_max_k << ",";

  // solving
  uint nagents_with_new_goals = 0;
  uint makespan = 1;
  uint cache_hit = 0;
  uint cache_access = 0;
  uint batch_idx = 0;
  uint throughput_index_cnt = 0;
  for (int i = 0; i < parser.num_goals; i += nagents_with_new_goals) {
    batch_idx++;
    // info output
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      current_time - timer)
      .count();

    if (!parser.debug_log && batch_idx % 100 == 0 && cache_access > 0 && is_cache(parser.cache_type)) {
      double cacheRate = static_cast<double>(cache_hit) / cache_access * 100.0;
      console->info(
        "Elapsed Time: {:5}ms   |   Goals Reached: {:5}   |   Cache Hit Rate: "
        "{:2.2f}%    |   Steps Used: {:5}",
        elapsed_time, i, cacheRate, makespan);
      // Reset the timer
      timer = std::chrono::steady_clock::now();
    }
    else if (!parser.debug_log && batch_idx % 100 == 0 && !is_cache(parser.cache_type)) {
      console->info(
        "Elapsed Time: {:5}ms   |   Goals Reached: {:5}   |   Steps Used: {:5}",
        elapsed_time, i, makespan);
      // Reset the timer
      timer = std::chrono::steady_clock::now();
    }

    double throughput = double(i) / double(makespan);
    for (; throughput_index_cnt < makespan; throughput_index_cnt += 200) {
      throughput_file << throughput << ",";
    }

    // ternimal log
    console->debug(
      "----------------------------------------------------------------------"
      "----------------------------------------");
    console->debug("STEP:   {}", makespan);
    console->debug("STARTS: {}", ins.starts);
    console->debug("GOALS:  {}", ins.goals);

    // reset time clock
    assert(deadline.reset());

    auto solution = solve(ins, parser.verbose_level - 1, &deadline, &parser.MT);
    const auto comp_time_ms = deadline.elapsed_ms();

    // failure
    if (solution.empty()) {
      std::ofstream csv_file(parser.output_csv_file, std::ios::app);

      if (!csv_file.is_open()) {
        std::cerr << "Failed to open file: " << parser.output_csv_file << std::endl;
        return 1;
      }

      csv_file << parser.map_file << ","
        << parser.cache_type_input << ","
        << parser.look_ahead_num << ","
        << parser.delay_deadline_limit << ","
        << parser.goals_gen_strategy_input << ","
        << parser.num_goals << ","
        << parser.num_agents << ","
        << parser.random_seed << ","
        << parser.verbose_level << ","
        << parser.time_limit_sec << ","
        << parser.goals_max_m << ","
        << parser.goals_max_k << ","
        << "fail to solve"
        << std::endl;

      csv_file.close();

      console->error("failed to solve");
      return 1;
    }

    // update step solution
    if (!log.update_solution(solution, ins.bit_status)) {
      console->error("Update step solution fails!");
      return 1;
    }

    // check feasibility
    if (!log.is_feasible_solution(ins, parser.verbose_level)) {
      console->error("invalid solution");
      return 1;
    }

    // statistics
    makespan += (solution.size() - 1);

    // post processing
    log.print_stats(parser.verbose_level, ins, comp_time_ms);
    log.make_step_log(ins, parser.output_step_file, comp_time_ms, parser.map_file, parser.random_seed, parser.short_log_format);

    // assign new goals
    if (is_cache(parser.cache_type)) {
      nagents_with_new_goals = ins.update_on_reaching_goals_with_cache(solution, parser.num_goals - i, cache_access, cache_hit);
    }
    else {
      nagents_with_new_goals = ins.update_on_reaching_goals_without_cache(solution, parser.num_goals - i);
    }
    console->debug("Reached Goals: {}", nagents_with_new_goals);
  }
  // Get percentiles
  std::vector<uint> step_percentiles = ins.compute_percentiles();

  double total_cache_rate = is_cache(parser.cache_type) ? static_cast<double>(cache_hit) / cache_access * 100.0 : .0;
  if (is_cache(parser.cache_type)) {
    console->info("Total Goals Reached: {:5}   |   Total Cache Hit Rate: {:2.2f}%    |   Makespan: {:5}   |   P0 Steps: {:5}    |   P50 Steps: {:5}   |   P99 Steps: {:5}", parser.num_goals, total_cache_rate, makespan, step_percentiles[0], step_percentiles[2], step_percentiles[6]);
  }
  else {
    console->info("Total Goals Reached: {:5}   |   Makespan: {:5}   |   P0 Steps: {:5}    |   P50 Steps: {:5}   |   P99 Steps: {:5}", parser.num_goals, makespan, step_percentiles[0], step_percentiles[2], step_percentiles[6]);
  }
  log.make_life_long_log(ins, parser.output_visual_file);


  std::ofstream csv_file(parser.output_csv_file, std::ios::app);

  if (!csv_file.is_open()) {
    std::cerr << "Failed to open file: " << parser.output_csv_file << std::endl;
    return 1;
  }

  csv_file << parser.map_file << ","
    << parser.cache_type_input << ","
    << parser.look_ahead_num << ","
    << parser.delay_deadline_limit << ","
    << parser.goals_gen_strategy_input << ","
    << parser.num_goals << ","
    << parser.num_agents << ","
    << parser.random_seed << ","
    << parser.verbose_level << ","
    << parser.time_limit_sec << ","
    << parser.goals_max_m << ","
    << parser.goals_max_k << ","
    << total_cache_rate << ","
    << makespan << ","
    << step_percentiles[0] << ","
    << step_percentiles[2] << ","
    << step_percentiles[6]
    << std::endl;

  csv_file.close();

  double total_throughput = double(parser.num_goals) / double(makespan);
  throughput_file << total_throughput << std::endl;
  throughput_file.close();

  return 0;
}
