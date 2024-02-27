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
    logger->error("Invalid graph type!");
    exit(1);
  }
}

Graph::Graph(
  const std::string& filename,
  std::shared_ptr<spdlog::logger> _logger,
  GoalGenerationType goal_generation_type,
  uint goals_m,
  uint goals_k,
  uint ngoals,
  CacheType cache_type,
  std::mt19937* _randomSeed) :
  V(Vertices()),
  width(0),
  height(0),
  randomSeed(_randomSeed),
  logger(_logger)
{
  std::ifstream file(filename);
  if (!file) {
    logger->error("file {} is not found.", filename);
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
  goals_list.resize(group);
  goals_cnt.resize(group, 0);

  if (is_cache(cache_type)) {
    // Generate cache
    cache = new Cache(_logger, cache_type, randomSeed);

    // Tmp variables
    int y = 0;
    Vertices tmp_cache_node;
    Vertices tmp_cargo_vertices;

    // Read map
    while (getline(file, line)) {
      logger->debug("{}", line);
      if (line.empty()) {
        // Stop reading if we reached specified group
        if (group_cnt >= group) break;

        // Update cache variables
        cache->node_cargo.push_back(tmp_cache_node);
        cache->node_id.push_back(tmp_cache_node);
        cache->node_coming_cargo.push_back(tmp_cache_node);
        cache->bit_cache_get_lock.emplace_back(tmp_cache_node.size(), 0);
        cache->bit_cache_insert_lock.emplace_back(tmp_cache_node.size(), 0);
        cache->is_empty.emplace_back(tmp_cache_node.size(), false);
        switch (cache_type) {
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
          logger->error("Unreachable cache type!");
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
    // logger->info("Cache blocks:     {}", cache->node_id);
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
          if (u != nullptr) v->neighbor.push_back(u);
        }
        // right
        if (x < width - 1) {
          auto u = U[width * y + (x + 1)];
          if (u != nullptr) v->neighbor.push_back(u);
        }
        // up
        if (y < height - 1) {
          auto u = U[width * (y + 1) + x];
          if (u != nullptr) v->neighbor.push_back(u);
        }
        // down
        if (y > 0) {
          auto u = U[width * (y - 1) + x];
          if (u != nullptr) v->neighbor.push_back(u);
        }
      }
    }
  }

  logger->info("Unloading ports:  {}", unloading_ports);
  logger->info("Generating goals...");

  for (int i = 0; i < group; i++) {
    fill_goals_list(goal_generation_type, i, goals_m, goals_k, ngoals);
  }
}

int Graph::size() const { return V.size(); }

Vertex* Graph::random_target_vertex(int group) {
  // Assert not empty
  assert(!cargo_vertices[group].empty());

  int index = get_random_int(randomSeed, 0, cargo_vertices[group].size() - 1);
  auto it = cargo_vertices[group].begin();
  std::advance(it, index);
  return *it;
}

void Graph::fill_goals_list(GoalGenerationType goal_generation_type, int group, uint goals_m, uint goals_k, uint ngoals) {
  if (goal_generation_type == GoalGenerationType::MK) {
    std::deque<Vertex*> sliding_window;
    std::unordered_map<Vertex*, int> goal_count;
    std::unordered_set<Vertex*> diff_goals;
    uint goals_generated = 0;

    while (goals_list[group].size() < ngoals) {
      if (goals_generated % 1000 == 0) {
        logger->info("Generated {:5}/{:5} goals.", goals_generated, ngoals);
      }

      Vertex* selected_goal = random_target_vertex(group);
      if (!selected_goal) {
        // Stop if no more goals can be selected
        break;
      }

      if (sliding_window.size() == goals_m) {
        Vertex* removed_goal = sliding_window.front();
        sliding_window.pop_front();
        goal_count[removed_goal]--;
        if (goal_count[removed_goal] == 0) {
          goal_count.erase(removed_goal);
          diff_goals.erase(removed_goal);
        }
      }

      if (diff_goals.size() == goals_k) {
        int index = get_random_int(randomSeed, 0, goals_k - 1);
        auto it = diff_goals.begin();
        std::advance(it, index);
        selected_goal = *it;
      }

      // Update status
      sliding_window.push_back(selected_goal);
      goal_count[selected_goal]++;
      diff_goals.insert(selected_goal);
      goals_list[group].push_back(selected_goal);
      goals_generated++;
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
  else if (goal_generation_type == GoalGenerationType::Zhang) {
    std::vector<double> item_prob = calculate_probabilities(cargo_vertices[group].size());
    logger->debug("length {}", item_prob.size());
    // boost::random::discrete_distribution<> dist(item_prob);
    for (uint i = 0; i < ngoals; i++) {
      goals_list[group].push_back(cargo_vertices[group][0]);
    }
  }

  logger->debug("Group {} goals {}", group, goals_list[group]);
}

Vertex* Graph::get_next_goal(int group) {
  assert(!goals_list[group].empty());
  // We should let each reaching unploading-port agent to be disappeared when the total
  // number of goals has been assigned but yet finished. For now, we let it 
  // out to have new random target. Let us know if this will cause any problem
  if (goals_cnt[group] >= goals_list[group].size()) {
    return random_target_vertex(group);
  }
  auto goal = goals_list[group][goals_cnt[group]];
  goals_cnt[group]++;
  return goal;
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