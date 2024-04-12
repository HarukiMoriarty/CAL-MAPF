#include "../include/log.hpp"

#include "../include/dist_table.hpp"

Log::Log(Parser* parser)
{
  log_console = spdlog::stderr_color_mt("log");
  if (parser->debug_log) log_console->set_level(spdlog::level::debug);
  else log_console->set_level(spdlog::level::info);

  // Open throughput file
  throughput_output_handler.open(parser->output_throughput_file, std::ios::app);
  if (!throughput_output_handler.is_open()) {
    log_console->error("Failed to open file: {}", parser->output_throughput_file);
    exit(1);
  }
  throughput_output_handler << parser->map_file << "," << parser->cache_type_input << "," << parser->goals_gen_strategy_input << "," << parser->num_goals << "," << parser->num_agents << "," << parser->random_seed << "," << "," << parser->time_limit_sec << "," << parser->goals_max_m << "," << parser->goals_max_k << ",";

  // Open csv file
  csv_output_handler.open(parser->output_csv_file, std::ios::app);
  if (!csv_output_handler.is_open()) {
    log_console->error("Failed to open file: {}", parser->output_csv_file);
    exit(1);
  }
  csv_output_handler << parser->map_file << "," << parser->cache_type_input << "," << parser->look_ahead_num << "," << parser->delay_deadline_limit << "," << parser->goals_gen_strategy_input << "," << parser->num_goals << "," << parser->num_agents << "," << parser->random_seed << "," << "," << parser->time_limit_sec << "," << parser->goals_max_m << "," << parser->goals_max_k << ",";

  // Open step file
  step_output_handler.open(parser->output_step_file, std::ios::app);
  if (!step_output_handler.is_open()) {
    log_console->error("Failed to open file: {}", parser->output_step_file);
    exit(1);
  }

  // Open visual file
  visual_output_handler.open(parser->output_visual_file, std::ios::app);
  if (!visual_output_handler.is_open()) {
    log_console->error("Failed to open file: {}", parser->output_visual_file);
    exit(1);
  }
}

Log::~Log() {
  // Close file
  throughput_output_handler.close();
  csv_output_handler.close();
  step_output_handler.close();
  visual_output_handler.close();
}

void Log::update_solution(Solution& solution, std::vector<uint> bit_status)
{
  // Update step solution
  step_solution = solution;

  // Update life long solution
  if (life_long_solution.empty()) {
    life_long_solution = step_solution;
  }
  else {
    life_long_solution.insert(life_long_solution.end(),
      step_solution.begin() + 1, step_solution.end());
  }

  // Update bit status life long log
  if (bit_status_log.empty()) {
    for (uint i = 0; i < solution.size(); i++) {
      bit_status_log.push_back(bit_status);
    }
  }
  else {
    for (uint i = 1; i < solution.size(); i++) {
      bit_status_log.push_back(bit_status);
    }
  }

  return;
}

bool Log::is_feasible_solution(const Instance& ins)
{
  if (step_solution.empty()) return true;

  // Check start locations
  if (!is_same_config(step_solution.front(), ins.starts)) {
    log_console->error("invalid starts");
    return false;
  }

  // Check goal locations
  if (!is_reach_at_least_one(step_solution.back(), ins.goals)) {
    log_console->error("invalid goals");
    return false;
  }

  for (size_t t = 1; t < step_solution.size(); ++t) {
    for (size_t i = 0; i < ins.parser->num_agents; ++i) {
      auto v_i_from = step_solution[t - 1][i];
      auto v_i_to = step_solution[t][i];
      // Check connectivity
      if (v_i_from != v_i_to &&
        std::find(v_i_to->neighbor.begin(), v_i_to->neighbor.end(), v_i_from) == v_i_to->neighbor.end()) {
        log_console->error("invalid move");
        return false;
      }

      // Check conflicts
      for (size_t j = i + 1; j < ins.parser->num_agents; ++j) {
        auto v_j_from = step_solution[t - 1][j];
        auto v_j_to = step_solution[t][j];
        // Vertex conflicts
        if (v_j_to == v_i_to) {
          log_console->error("vertex conflict");
          return false;
        }
        // Swap conflicts
        if (v_j_to == v_i_from && v_j_from == v_i_to) {
          log_console->error("edge conflict");
          return false;
        }
      }
    }
  }

  return true;
}

int Log::get_makespan()
{
  if (step_solution.empty()) return 0;
  return step_solution.size() - 1;
}

int Log::get_path_cost(int i)
{
  const auto makespan = step_solution.size();
  const auto g = step_solution.back()[i];
  auto c = makespan;
  while (c > 0 && step_solution[c - 1][i] == g) --c;
  return c;
}

int Log::get_sum_of_costs()
{
  if (step_solution.empty()) return 0;
  int c = 0;
  const auto N = step_solution.front().size();
  for (size_t i = 0; i < N; ++i) c += get_path_cost(i);
  return c;
}

int Log::get_sum_of_loss()
{
  if (step_solution.empty()) return 0;
  int c = 0;
  const auto N = step_solution.front().size();
  const auto T = step_solution.size();
  for (size_t i = 0; i < N; ++i) {
    auto g = step_solution.back()[i];
    for (size_t t = 1; t < T; ++t) {
      if (step_solution[t - 1][i] != g || step_solution[t][i] != g) ++c;
    }
  }
  return c;
}

int Log::get_makespan_lower_bound(const Instance& ins, DistTable& dist_table)
{
  int c = 0;
  for (size_t i = 0; i < ins.parser->num_agents; ++i) {
    c = std::max(c, dist_table.get(i, ins.starts[i]));
  }
  return c;
}

int Log::get_sum_of_costs_lower_bound(const Instance& ins,
  DistTable& dist_table)
{
  int c = 0;
  for (size_t i = 0; i < ins.parser->num_agents; ++i) {
    c += dist_table.get(i, ins.starts[i]);
  }
  return c;
}

void Log::print_stats(const Instance& ins, const double comp_time_ms)
{
  auto ceil = [](float x) { return std::ceil(x * 100) / 100; };

  auto dist_table = DistTable(ins);
  const auto makespan = get_makespan();
  const auto makespan_lb = get_makespan_lower_bound(ins, dist_table);
  const auto sum_of_costs = get_sum_of_costs();
  const auto sum_of_costs_lb = get_sum_of_costs_lower_bound(ins, dist_table);
  const auto sum_of_loss = get_sum_of_loss();

  log_console->debug(
    "solved: {} ms\tmakespan: {} (lb={}, ub={})\tsum_of_costs: {} (lb={}, "
    "ub={})\tsum_of_loss: {} (lb={}, ub={})",
    comp_time_ms, makespan, makespan_lb,
    ceil(static_cast<float>(makespan) / makespan_lb), sum_of_costs,
    sum_of_costs_lb, ceil(static_cast<float>(sum_of_costs) / sum_of_costs_lb),
    sum_of_loss, sum_of_costs_lb,
    ceil(static_cast<float>(sum_of_loss) / sum_of_costs_lb));
}

// for log of map_name
static const std::regex r_map_name = std::regex(R"(.+/(.+))");

void Log::make_step_log(const Instance& ins, const std::string& output_name,
  const double comp_time_ms, const std::string& map_name,
  const int seed, const bool log_short)
{
  // map name
  std::smatch results;
  const auto map_recorded_name =
    (std::regex_match(map_name, results, r_map_name)) ? results[1].str()
    : map_name;

  // for instance-specific values
  auto dist_table = DistTable(ins);

  // log for visualizer
  auto get_x = [&](int k) { return k % ins.graph.width; };
  auto get_y = [&](int k) { return k / ins.graph.width; };

  step_output_handler << "agents=" << ins.parser->num_agents << std::endl
    << "map_file=" << map_recorded_name << std::endl
    << "solver=planner" << std::endl
    << "solved=" << !step_solution.empty() << std::endl
    << "soc=" << get_sum_of_costs() << std::endl
    << "soc_lb=" << get_sum_of_costs_lower_bound(ins, dist_table) << std::endl
    << "makespan=" << get_makespan() << std::endl
    << "makespan_lb=" << get_makespan_lower_bound(ins, dist_table) << std::endl
    << "sum_of_loss=" << get_sum_of_loss() << std::endl
    << "sum_of_loss_lb=" << get_sum_of_costs_lower_bound(ins, dist_table) << std::endl
    << "comp_time=" << comp_time_ms << std::endl
    << "seed=" << seed << std::endl;

  if (log_short) return;
  step_output_handler << "starts=";
  for (size_t i = 0; i < ins.parser->num_agents; ++i) {
    auto k = ins.starts[i]->index;
    step_output_handler << "(" << get_x(k) << "," << get_y(k) << "),";
  }
  step_output_handler << std::endl << "goals=";
  for (size_t i = 0; i < ins.parser->num_agents; ++i) {
    auto k = ins.goals[i]->index;
    step_output_handler << "(" << get_x(k) << "," << get_y(k) << "),";
  }
  step_output_handler << std::endl << "solution=" << std::endl;

  std::vector<std::vector<int> > new_sol(ins.parser->num_agents, std::vector<int>(step_solution.size(), 0));
  for (size_t t = 0; t < step_solution.size(); ++t) {
    step_output_handler << t << ":";
    auto C = step_solution[t];
    int idx = 0;
    for (auto v : C) {
      step_output_handler << "(" << get_x(v->index) << "," << get_y(v->index) << "),";
      new_sol[idx][t] = v->index;
      idx++;
    }
    step_output_handler << std::endl;
  }
}

void Log::make_life_long_log(const Instance& ins, std::string visual_name)
{
  log_console->info("life long solution size: {}, bit status size: {}", life_long_solution.size(), bit_status_log.size());

  auto dist_table = DistTable(ins);
  auto get_x = [&](int k) { return k % ins.graph.width; };
  auto get_y = [&](int k) { return k / ins.graph.width; };
  std::vector<std::vector<int> > new_sol(ins.parser->num_agents, std::vector<int>(life_long_solution.size(), 0));

  for (size_t t = 0; t < life_long_solution.size(); ++t) {
    auto C = life_long_solution[t];
    int idx = 0;
    for (auto v : C) {
      new_sol[idx][t] = v->index;
      idx++;
    }
  }

  visual_output_handler << "width: " << ins.graph.width << std::endl
    << "height: " << ins.graph.height << std::endl
    << "schedule: " << std::endl;

  for (size_t a = 0; a < new_sol.size(); ++a) {
    visual_output_handler << "  agent" << a << ":" << std::endl;
    for (size_t t = 0; t < new_sol[a].size(); ++t) {
      visual_output_handler << "    - x: " << get_y(new_sol[a][t]) << std::endl
        << "      y: " << get_x(new_sol[a][t]) << std::endl
        << "      t: " << t << std::endl
        << "      s: " << bit_status_log[t][a] << std::endl;
    }
  }
}

void Log::make_throughput_log(uint index, uint* start_cnt, uint make_span)
{
  double throughput = double(index) / double(make_span);
  for (; *start_cnt < make_span; *start_cnt += 200) {
    throughput_output_handler << throughput << ",";
  }
}

void Log::make_csv_log(double cache_hit_rate, uint make_span, std::vector<uint>* step_percentiles, bool failure)
{
  if (!failure) {
    csv_output_handler << cache_hit_rate << "," << make_span << "," << (*step_percentiles)[0] << "," << (*step_percentiles)[2] << "," << (*step_percentiles)[6] << std::endl;
  }
  else {
    csv_output_handler << "fail to solve" << std::endl;
  }
}