#include <argparse/argparse.hpp>
#include <chrono>
#include <calmapf.hpp>

int main(int argc, char* argv[])
{
  auto console = spdlog::stderr_color_mt("console");
  console->set_level(spdlog::level::info);

  // arguments parser
  argparse::ArgumentParser program("lacam", "0.1.0");
  program.add_argument("-m", "--map").help("map file").required();                                                                              // map file
  program.add_argument("-ca", "--cache").help("cache type: NONE, LRU, FIFO, RANDOM").default_value(std::string("NONE"));                        // cache type                    
  program.add_argument("-ng", "--ngoals").help("number of goals").required();                                                                   // number of goals: agent first go to get goal, and then return to unloading port
  program.add_argument("-gg", "--goals_generation").help("goals generation strategy: MK, Zhang, Real").required();                                // goals generation type
  program.add_argument("-gk", "--goals-k").help("maximum k different number of goals in m segment of all goals").default_value(std::string("0"));
  program.add_argument("-gm", "--goals-m").help("maximum k different number of goals in m segment of all goals").default_value(std::string("0"));
  program.add_argument("-grf", "--goals-real-file").help("real distribution data file path").default_value(std::string("./data/order_data.csv"));
  program.add_argument("-na", "--nagents").help("number of agents").required();                                                                 // number of agents
  program.add_argument("-s", "--seed").help("seed").default_value(std::string("0"));                                                            // random seed
  program.add_argument("-v", "--verbose").help("verbose").default_value(std::string("0"));                                                      // verbose
  program.add_argument("-t", "--time_limit_sec").help("time limit sec").default_value(std::string("10"));                                       // time limit (second)
  program.add_argument("-o", "--output_step_result").help("step result output file").default_value(std::string("./result/step_result.txt"));    // output file
  program.add_argument("-c", "--output_csv_result").help("csv output file").default_value(std::string("./result/result.csv"));
  program.add_argument("-th", "--output_throughput_result").help("throughput output file").default_value(std::string("./result/throughput.csv"));
  program.add_argument("-l", "--log_short").default_value(false).implicit_value(true);
  program.add_argument("-d", "--debug").help("enable debug logging").default_value(false).implicit_value(true);                                 // debug mode

  try {
    program.parse_known_args(argc, argv);
  }
  catch (const std::runtime_error& err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    std::exit(1);
  }

  // setup instance
  const auto verbose = std::stoi(program.get<std::string>("verbose"));
  const auto time_limit_sec = std::stoi(program.get<std::string>("time_limit_sec"));
  auto deadline = Deadline(time_limit_sec * 1000);
  const auto seed = std::stoi(program.get<std::string>("seed"));
  auto MT = std::mt19937(seed);
  const auto map_name = program.get<std::string>("map");
  const auto cache = program.get<std::string>("cache");
  const auto output_step_name = program.get<std::string>("output_step_result");
  const auto output_csv_name = program.get<std::string>("output_csv_result");
  const auto output_throughput_name = program.get<std::string>("output_throughput_result");
  const auto log_short = program.get<bool>("log_short");
  const auto ngoals = std::stoi(program.get<std::string>("ngoals"));
  const auto gg = program.get<std::string>("goals_generation");
  const auto grf = program.get<std::string>("goals-real-file");
  const auto nagents = std::stoi(program.get<std::string>("nagents"));
  const auto debug = program.get<bool>("debug");
  if (debug) console->set_level(spdlog::level::debug);

  CacheType cache_type = CacheType::NONE;
  if (cache == "NONE") {
    cache_type = CacheType::NONE;
  }
  else if (cache == "LRU") {
    cache_type = CacheType::LRU;
  }
  else if (cache == "FIFO") {
    cache_type = CacheType::FIFO;
  }
  else if (cache == "RANDOM") {
    cache_type = CacheType::RANDOM;
  }
  else {
    console->error("Invalid cache type!");
    exit(1);
  }

  GoalGenerationType goal_generation_type;
  const auto goals_m = std::stoi(program.get<std::string>("goals-m"));
  const auto goals_k = std::stoi(program.get<std::string>("goals-k"));

  if (gg == "MK") {
    if (goals_m == 0 || goals_k == 0) {
      console->error("For MK generation, both --goals-m and --goals-k must have non-zero values.");
      exit(1);
    }
    goal_generation_type = GoalGenerationType::MK;
  }
  else if (gg == "Zhang") {
    goal_generation_type = GoalGenerationType::Zhang;
  }
  else if (gg == "Real") {
    goal_generation_type = GoalGenerationType::Real;
  }
  else {
    console->error("Invalid goals_generation strategy specified.");
    exit(1);
  }

  // check paras
  if (nagents > ngoals) {
    console->error("number of goals must larger or equal to number of agents");
    return 1;
  }

  // output arguments info
  console->info("Map file:         {}", map_name);
  console->info("Cache type:       {}", cache);
  console->info("Number of goals:  {}", ngoals);
  console->info("Number of agents: {}", nagents);
  console->info("Goal Generation:  {}", gg);
  if (goal_generation_type == GoalGenerationType::MK) {
    console->info("Goals m:          {}", goals_m);
    console->info("Goals k:          {}", goals_k);
  }
  console->info("Seed:             {}", seed);
  console->info("Verbose:          {}", verbose);
  console->info("Time limit (sec): {}", time_limit_sec);
  console->info("Output file:      {}", output_step_name);
  console->info("Log short:        {}", log_short);
  console->info("Debug:            {}", debug);

  // generating instance
  auto ins = Instance(map_name, &MT, console, goal_generation_type, grf, cache_type, nagents, ngoals, goals_m, goals_k);
  if (!ins.is_valid(1)) {
    console->error("instance is invalid!");
    return 1;
  }

  // initliaze log system
  Log log(console);
  // initliaze info timer
  auto timer = std::chrono::steady_clock::now();

  // prepare for throughput record
  std::ofstream throughput_file(output_throughput_name, std::ios::app);
  if (!throughput_file.is_open()) {
    std::cerr << "Failed to open file: " << output_throughput_name << std::endl;
    return 1;
  }
  throughput_file << map_name << ","
    << cache << ","
    << gg << ","
    << ngoals << ","
    << nagents << ","
    << seed << ","
    << verbose << ","
    << time_limit_sec << ","
    << goals_m << ","
    << goals_k << ",";

  // solving
  uint nagents_with_new_goals = 0;
  uint makespan = 1;
  uint cache_hit = 0;
  uint cache_access = 0;
  uint batch_idx = 0;
  uint throughput_index_cnt = 0;
  for (int i = 0; i < ngoals; i += nagents_with_new_goals) {
    batch_idx++;
    // info output
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      current_time - timer)
      .count();

    if (!debug && batch_idx % 100 == 0 && cache_access > 0 && is_cache(cache_type)) {
      double cacheRate = static_cast<double>(cache_hit) / cache_access * 100.0;
      console->info(
        "Elapsed Time: {:5}ms   |   Goals Reached: {:5}   |   Cache Hit Rate: "
        "{:2.2f}%    |   Steps Used: {:5}",
        elapsed_time, i, cacheRate, makespan);
      // Reset the timer
      timer = std::chrono::steady_clock::now();
    }
    else if (!debug && batch_idx % 100 == 0 && !is_cache(cache_type)) {
      console->info(
        "Elapsed Time: {:5}ms   |   Goals Reached: {:5}   |   Steps Used: {:5}",
        elapsed_time, i, makespan);
      // Reset the timer
      timer = std::chrono::steady_clock::now();
    }

    double throughput = double(i) / double(makespan);
    for (throughput_index_cnt; throughput_index_cnt < makespan; throughput_index_cnt += 200) {
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

    auto solution = solve(ins, verbose - 1, &deadline, &MT);
    const auto comp_time_ms = deadline.elapsed_ms();

    // failure
    if (solution.empty()) {
      std::ofstream csv_file(output_csv_name, std::ios::app);

      if (!csv_file.is_open()) {
        std::cerr << "Failed to open file: " << output_csv_name << std::endl;
        return 1;
      }

      csv_file << map_name << ","
        << cache << ","
        << gg << ","
        << ngoals << ","
        << nagents << ","
        << seed << ","
        << verbose << ","
        << time_limit_sec << ","
        << goals_m << ","
        << goals_k << ","
        << "fail to solve"
        << std::endl;

      csv_file.close();

      console->error("failed to solve");
      return 1;
    }

    // update step solution
    if (!log.update_solution(solution)) {
      console->error("Update step solution fails!");
      return 1;
    }

    // check feasibility
    if (!log.is_feasible_solution(ins, verbose)) {
      console->error("invalid solution");
      return 1;
    }

    // statistics
    makespan += (solution.size() - 1);

    // post processing
    log.print_stats(verbose, ins, comp_time_ms);
    log.make_step_log(ins, output_step_name, comp_time_ms, map_name, seed,
      log_short);

    // assign new goals
    if (is_cache(cache_type)) {
      nagents_with_new_goals = ins.update_on_reaching_goals_with_cache(solution, ngoals - i, cache_access, cache_hit);
    }
    else {
      nagents_with_new_goals = ins.update_on_reaching_goals_without_cache(solution, ngoals - i);
    }
    console->debug("Reached Goals: {}", nagents_with_new_goals);
  }
  // Get percentiles
  std::vector<uint> step_percentiles = ins.compute_percentiles();

  double total_cache_rate = is_cache(cache_type) ? static_cast<double>(cache_hit) / cache_access * 100.0 : .0;
  if (is_cache(cache_type)) {
    console->info("Total Goals Reached: {:5}   |   Total Cache Hit Rate: {:2.2f}%    |   Makespan: {:5}   |   P0 Steps: {:5}    |   P50 Steps: {:5}   |   P99 Steps: {:5}", ngoals, total_cache_rate, makespan, step_percentiles[0], step_percentiles[2], step_percentiles[6]);
  }
  else {
    console->info("Total Goals Reached: {:5}   |   Makespan: {:5}   |   P0 Steps: {:5}    |   P50 Steps: {:5}   |   P99 Steps: {:5}", ngoals, makespan, step_percentiles[0], step_percentiles[2], step_percentiles[6]);
  }
  // log.make_life_long_log(ins, seed);


  std::ofstream csv_file(output_csv_name, std::ios::app);

  if (!csv_file.is_open()) {
    std::cerr << "Failed to open file: " << output_csv_name << std::endl;
    return 1;
  }

  csv_file << map_name << ","
    << cache << ","
    << gg << ","
    << ngoals << ","
    << nagents << ","
    << seed << ","
    << verbose << ","
    << time_limit_sec << ","
    << goals_m << ","
    << goals_k << ","
    << total_cache_rate << ","
    << makespan << ","
    << step_percentiles[0] << ","
    << step_percentiles[2] << ","
    << step_percentiles[6]
    << std::endl;

  csv_file.close();

  double total_throughput = double(ngoals) / double(makespan);
  throughput_file << total_throughput << std::endl;
  throughput_file.close();

  return 0;
}
