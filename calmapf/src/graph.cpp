#include "../include/graph.hpp"


/* Help function */
double calculate_sum(int start, int end) {
  double sum = 0.0;
  for (int i = start; i <= end; ++i) {
    sum += 1.0 / (i + 1);
  }
  return sum;
}

std::vector<double> calculate_probabilities(int n) {
  std::vector<double> probabilities(n);

  // The first 10% of items
  double sum_first_10 = calculate_sum(0, int(n * 0.1) - 1);
  for (int i = 0; i < int(n * 0.1); ++i) {
    probabilities[i] = 0.7 / sum_first_10 * (1.0 / (i + 1));
  }

  // For item 10, it has the same probability as item 9
  probabilities[int(n * 0.1)] = probabilities[int(n * 0.1) - 1];

  // The next 20% of items
  double sum_next_20 = calculate_sum(int(n * 0.1), int(n * 0.3) - 1);
  for (int i = int(n * 0.1) + 1; i < int(n * 0.3); ++i) {
    probabilities[i] = 0.2 / sum_next_20 * (1.0 / (i + 1));
  }

  // For item 30, it has the same probability as item 29
  probabilities[int(n * 0.3)] = probabilities[int(n * 0.3) - 1];

  // The last 70% of items
  double sum_last_70 = calculate_sum(int(n * 0.3), n - 1);
  for (int i = int(n * 0.3) + 1; i < n; ++i) {
    probabilities[i] = 0.1 / sum_last_70 * (1.0 / (i + 1));
  }

  return probabilities;
}

// This is a general function that loads teh data file and compute the frequency of each product.
// The frequencies will be used to generate a sequence of products.
// The probability vector: prob_v[product_id] = frequency (0 if product_id never appears in the data).
std::vector<float> compute_frequency_from_file(std::string file_path) {
  int total = 0;
  int n_line = 0;
  int largest_idx = 0;
  std::string line;
  std::map<int, float> product_frequency;
  std::vector<float> prob_v;

  std::ifstream file(file_path);
  if (!file.is_open()) {
    std::cerr << "Failed to open data file." << std::endl;
    return prob_v;
  }

  while (getline(file, line)) {
    if (n_line == 0) {
      n_line++;
      continue;
    }
    std::stringstream ss(line);
    std::string field;
    getline(ss, field, ',');    // get the first element in each row (the product id).
    int product_id = stoi(field);
    product_frequency[product_id]++;
    if (product_id > largest_idx) {
      largest_idx = product_id;
    }
    total++;
    n_line++;
  }

  for (auto it = product_frequency.begin(); it != product_frequency.end(); it++) {
    it->second = it->second / total;
  }

  for (int i = 0;i <= largest_idx;i++) {
    prob_v.push_back(product_frequency[i]);
  }

  return prob_v;
}
/* Help function end*/


Graph::~Graph()
{
  for (auto& v : V)
    if (v != nullptr) delete v;
  V.clear();
  delete cache;
}

// regular function to load graph
static const std::regex r_type = std::regex(R"(type\s(\w+))");
static const std::regex r_group = std::regex(R"(group\s(\d+))");
static const std::regex r_height = std::regex(R"(height\s(\d+))");
static const std::regex r_width = std::regex(R"(width\s(\d+))");
static const std::regex r_map = std::regex(R"(map)");

GraphType Graph::get_graph_type(std::string type) {
  if (type == "single_port") {
    return GraphType::SINGLE_PORT;
  }
  else if (type == "multi_port") {
    return GraphType::MULTI_PORT;
  }
  else {
    graph_console->error("Invalid graph type!");
    exit(1);
  }
}

Graph::Graph(Parser* _parser) : parser(_parser)
{
  // Set up logger
  if (auto existing_console = spdlog::get("graph"); existing_console != nullptr) graph_console = existing_console;
  else graph_console = spdlog::stderr_color_mt("graph");
  if (parser->debug_log) graph_console->set_level(spdlog::level::debug);
  else graph_console->set_level(spdlog::level::info);

  std::ifstream file(parser->map_file);
  if (!file) {
    graph_console->error("file {} is not found.", parser->map_file);
    return;
  }

  std::string line;
  std::smatch results;

  // read fundamental graph parameters
  while (getline(file, line)) {
    // for CRLF coding
    if (*(line.end() - 1) == 0x0d) line.pop_back();

    if (std::regex_match(line, results, r_type)) {
      type = get_graph_type(results[1].str());
    }

    if (std::regex_match(line, results, r_group)) {
      group = std::stoi(results[1].str());
    }

    if (std::regex_match(line, results, r_height)) {
      height = std::stoi(results[1].str());
    }
    if (std::regex_match(line, results, r_width)) {
      width = std::stoi(results[1].str());
    }
    if (std::regex_match(line, results, r_map)) break;
  }

  U = Vertices(width * height, nullptr);
  int group_cnt = 0;
  goals_queue.resize(group);
  goals_delay.resize(group);

  if (is_cache(parser->cache_type)) {
    // Generate cache
    cache = new Cache(parser);

    // Tmp variables
    int y = 0;
    Vertices tmp_cache_node;
    Vertices tmp_cargo_vertices;

    // Read map
    while (getline(file, line)) {
      graph_console->debug("{}", line);
      if (line.empty()) {
        // Stop reading if we reached specified group
        if (group_cnt >= group) break;

        // Update cache variables
        cache->node_cargo.push_back(tmp_cache_node);
        cache->node_id.push_back(tmp_cache_node);
        cache->node_coming_cargo.push_back(tmp_cache_node);
        cache->node_cargo_num.emplace_back(tmp_cache_node.size(), 0);
        cache->bit_cache_get_lock.emplace_back(tmp_cache_node.size(), 0);
        cache->bit_cache_insert_or_clear_lock.emplace_back(tmp_cache_node.size(), 0);
        cache->is_empty.emplace_back(tmp_cache_node.size(), true);
        switch (parser->cache_type) {
        case CacheType::LRU:
          cache->LRU.emplace_back(tmp_cache_node.size(), 0);
          cache->LRU_cnt.resize(tmp_cache_node.size(), 0);
          break;
        case CacheType::FIFO:
          cache->FIFO.emplace_back(tmp_cache_node.size(), 0);
          cache->FIFO_cnt.resize(tmp_cache_node.size(), 0);
          break;
        case CacheType::RANDOM:
          break;
        default:
          graph_console->error("Unreachable cache type!");
          exit(1);
        }

        // Update cargo vertices
        cargo_vertices.push_back(tmp_cargo_vertices);

        // Clear tmp variables
        tmp_cache_node.clear();
        tmp_cargo_vertices.clear();

        // Update group index
        group_cnt++;
        continue;
      }

      // for CRLF coding
      if (*(line.end() - 1) == 0x0d) line.pop_back();

      for (int x = 0; x < width; ++x) {
        char s = line[x];

        // Record walls
        if (s == 'T' or s == '@') continue;

        // Generate vertices
        auto index = width * y + x;
        auto v = new Vertex(V.size(), index, width, group_cnt);

        // Record unloading ports
        if (s == 'U') {
          unloading_ports.push_back(v);
        }

        // Record cache blocks
        else if (s == 'C') {
          v->cargo = true;
          // insert into tmp variable
          tmp_cache_node.push_back(v);
        }

        // record cargo blocks
        else if (s == 'H') {
          v->cargo = true;
          tmp_cargo_vertices.push_back(v);
        }

        // Record in whole map
        V.push_back(v);
        U[index] = v;
      }
      ++y;
    }
    file.close();

    // create edges
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        auto v = U[width * y + x];
        if (v == nullptr) continue;
        // left
        if (x > 0) {
          auto u = U[width * y + (x - 1)];
          if (v->cargo) {
            if (u != nullptr && !u->cargo) v->neighbor.push_back(u);
          }
          else {
            if (u != nullptr) v->neighbor.push_back(u);
          }
        }
        // right
        if (x < width - 1) {
          auto u = U[width * y + (x + 1)];
          if (v->cargo) {
            if (u != nullptr && !u->cargo) v->neighbor.push_back(u);
          }
          else {
            if (u != nullptr) v->neighbor.push_back(u);
          }
        }
        // up
        if (y < height - 1) {
          auto u = U[width * (y + 1) + x];
          if (v->cargo) {
            if (u != nullptr && !u->cargo) v->neighbor.push_back(u);
          }
          else {
            if (u != nullptr) v->neighbor.push_back(u);
          }
        }
        // down
        if (y > 0) {
          auto u = U[width * (y - 1) + x];
          if (v->cargo) {
            if (u != nullptr && !u->cargo) v->neighbor.push_back(u);
          }
          else {
            if (u != nullptr) v->neighbor.push_back(u);
          }
        }
      }
    }

    for (uint i = 0; i < cache->node_id.size(); i++) {
      graph_console->info("Cache blocks:     {}", cache->node_id[i]);
    }
  }
  else {
    // Tmp variables
    int y = 0;
    Vertices tmp_cargo_vertices;

    while (getline(file, line)) {
      if (line.empty()) {
        // Stop reading if we reached specified group
        if (group_cnt >= group) break;

        // Update cargo variables
        cargo_vertices.push_back(tmp_cargo_vertices);

        // Clear tmp variables
        tmp_cargo_vertices.clear();

        // Update group index
        group_cnt++;
        continue;
      }
      // for CRLF coding
      if (*(line.end() - 1) == 0x0d) line.pop_back();

      for (int x = 0; x < width; ++x) {
        char s = line[x];

        // Record walls
        if (s == 'T' or s == '@') continue;

        // Generate vertices
        auto index = width * y + x;
        auto v = new Vertex(V.size(), index, width, group_cnt);

        // Record unloading ports
        if (s == 'U') {
          unloading_ports.push_back(v);
        }

        // Record cargo blocks
        else if (s == 'H') {
          v->cargo = true;
          tmp_cargo_vertices.push_back(v);
        }

        // Record in whole map
        V.push_back(v);
        U[index] = v;
      }
      ++y;
    }
    file.close();

    // create edges
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        auto v = U[width * y + x];
        if (v == nullptr) continue;
        // left
        if (x > 0) {
          auto u = U[width * y + (x - 1)];
          if (v->cargo) {
            if (u != nullptr && !u->cargo) v->neighbor.push_back(u);
          }
          else {
            if (u != nullptr) v->neighbor.push_back(u);
          }
        }
        // right
        if (x < width - 1) {
          auto u = U[width * y + (x + 1)];
          if (v->cargo) {
            if (u != nullptr && !u->cargo) v->neighbor.push_back(u);
          }
          else {
            if (u != nullptr) v->neighbor.push_back(u);
          }
        }
        // up
        if (y < height - 1) {
          auto u = U[width * (y + 1) + x];
          if (v->cargo) {
            if (u != nullptr && !u->cargo) v->neighbor.push_back(u);
          }
          else {
            if (u != nullptr) v->neighbor.push_back(u);
          }
        }
        // down
        if (y > 0) {
          auto u = U[width * (y - 1) + x];
          if (v->cargo) {
            if (u != nullptr && !u->cargo) v->neighbor.push_back(u);
          }
          else {
            if (u != nullptr) v->neighbor.push_back(u);
          }
        }
      }
    }
  }

  graph_console->info("Unloading ports:  {}", unloading_ports);
  graph_console->info("Generating goals...");

  for (int i = 0; i < group; i++) {
    _fill_goals_list(i);
  }
}

int Graph::size() const { return V.size(); }

Vertex* Graph::random_target_vertex(int group) {
  // Assert not empty
  assert(!cargo_vertices[group].empty());

  int index = get_random_int(&parser->MT, 0, cargo_vertices[group].size() - 1);
  auto it = cargo_vertices[group].begin();
  std::advance(it, index);
  return *it;
}

void Graph::_fill_goals_list(int group_index) {
  for (uint i = 0; i < parser->strategy_num_goals.size(); i++) {
    if (i == 0 && parser->strategy_num_goals[i] != 0) {
      std::deque<Vertex*> sliding_window;
      std::unordered_map<Vertex*, int> goal_count;
      std::unordered_set<Vertex*> diff_goals;

      while (goals_queue[group_index].size() < (uint(parser->strategy_num_goals[0] / group) + 1)) {
        Vertex* selected_goal = random_target_vertex(group_index);

        if (sliding_window.size() == parser->goals_max_m) {
          Vertex* removed_goal = sliding_window.front();
          sliding_window.pop_front();
          goal_count[removed_goal]--;
          if (goal_count[removed_goal] == 0) {
            goal_count.erase(removed_goal);
            diff_goals.erase(removed_goal);
          }
        }

        if (diff_goals.size() == parser->goals_max_k) {
          int index = get_random_int(&parser->MT, 0, parser->goals_max_k - 1);
          auto it = diff_goals.begin();
          std::advance(it, index);
          selected_goal = *it;
        }

        // Update status
        sliding_window.push_back(selected_goal);
        goal_count[selected_goal]++;
        diff_goals.insert(selected_goal);
        goals_queue[group_index].push_back(selected_goal);
        goals_delay[group_index].push_back(0);
      }
    }
    // The generation method that is based on "Zhang, Y., 2016. Correlated storage assignment strategy to reduce travel distance in order picking. IFAC-PapersOnLine, 49(2), pp.30-35."
    // A summary of the method.
    // 10% of items have a sum of 70% probability to be selected (A-class). 20% of items with 20% probability (B-class), 70% of items with 10% (C-class)
    // Assumptions:
    // 1. The probability for each item decreases as the item index increases.
    // 2. The last item in A-class has the same probability as the first item in B-class.
    // 3. The last item in B-class has the same probability as the first item in C-class.
    // 4. The sum of the probabilities for all A-class items,B-class items, and C-class items are 70%, 20%, and 10%, correspondingly.
    else if (i == 1 && parser->strategy_num_goals[i] != 0) {
      std::vector<double> item_prob = calculate_probabilities(cargo_vertices[group_index].size());
      boost::random::discrete_distribution<> dist(item_prob);
      for (uint i = 0; i < (parser->strategy_num_goals[1] / uint(group) + 1); i++) {
        goals_queue[group_index].push_back(cargo_vertices[group_index][dist(parser->MT)]);
        goals_delay[group_index].push_back(0);
      }
    }
    else if (i == 2 && parser->strategy_num_goals[i] != 0) {
      std::vector<float> prob_v = compute_frequency_from_file(parser->real_dist_file_path);
      prob_v.resize(cargo_vertices[group_index].size());
      boost::random::discrete_distribution<> dist(prob_v);
      for (uint i = 0; i < (parser->strategy_num_goals[2] / uint(group) + 1); i++) {
        goals_queue[group_index].push_back(cargo_vertices[group_index][dist(parser->MT)]);
        goals_delay[group_index].push_back(0);
      }
    }
  }

  graph_console->info("Group {} goals {}", group, goals_queue[group_index].size());
}

Vertex* Graph::get_next_goal(int group, int look_ahead) {
  assert(goals_queue[group].size() == goals_delay[group].size());
  // Check if the specific group's queue is empty
  if (goals_queue[group].empty()) {
    return random_target_vertex(group);
  }

  std::vector<Vertex*> temp_goals;
  std::vector<int> temp_goals_delay;
  int size = std::min(look_ahead, static_cast<int>(goals_queue[group].size()));

  int cache_hit_index = 0;
  for (int i = 0; i < size; ++i) {
    Vertex* current_goal = goals_queue[group].front();
    int current_goal_delay = goals_delay[group].front();
    goals_queue[group].pop_front();
    goals_delay[group].pop_front();
    temp_goals.push_back(current_goal);
    temp_goals_delay.push_back(current_goal_delay);

    if (cache != nullptr && (cache->look_ahead_cache(current_goal) || current_goal_delay >= parser->delay_deadline_limit)) {
      cache_hit_index = i;
      break;
    }
  }

  Vertex* selected_goal = temp_goals[cache_hit_index];
  for (int i = temp_goals.size() - 1; i >= 0; i--) {
    if (i != cache_hit_index) {
      goals_queue[group].push_front(temp_goals[i]);
      goals_delay[group].push_front(++temp_goals_delay[i]);
    }
  }
  return selected_goal;
}

bool is_same_config(const Config& C1, const Config& C2)
{
  const auto N = C1.size();
  for (size_t i = 0; i < N; ++i) {
    if (C1[i]->id != C2[i]->id) return false;
  }
  return true;
}

bool is_reach_at_least_one(const Config& C1, const Config& C2)
{
  const auto N = C1.size();
  for (size_t i = 0; i < N; i++) {
    if (C1[i]->id == C2[i]->id) return true;
  }
  return false;
}

uint ConfigHasher::operator()(const Config& C) const
{
  uint hash = C.size();
  for (auto& v : C) {
    hash ^= v->id + 0x9e3779b9 + (hash << 6) + (hash >> 2);
  }
  return hash;
}