// Parser implementation
// Zhenghong Yu

#include "../include/parser.hpp"

Parser::Parser(int argc, char* argv[]) {
    // Set up logger
    if (auto existing_console = spdlog::get("parser"); existing_console != nullptr) parser_console = existing_console;
    else parser_console = spdlog::stderr_color_mt("parser");
    parser_console->set_level(spdlog::level::info);

    // arguments definition
    argparse::ArgumentParser program("CAL-MAPF", "0.1.0");
    program.add_argument("-mf", "--map-file").help("Path to the map file.").required();
    program.add_argument("-ct", "--cache-type").help("Type of cache to use: NONE, LRU, FIFO, RANDOM. Defaults to NONE.").default_value(std::string("NONE"));
    program.add_argument("-lan", "--look-ahead-num").help("Number for look-ahead logic. Defaults to 1.").default_value(std::string("1"));
    program.add_argument("-ddl", "--delay-deadline-limit").help("Delay deadline limit for task assignment. Defaults to 1.").default_value(std::string("1"));
    program.add_argument("-ng", "--num-goals").help("Number of goals to achieve.").required();
    program.add_argument("-ggs", "--goals-gen-strategy").help("Strategy for goals generation: MK, Zhang, Real.").required();
    program.add_argument("-gmk", "--goals-max-k").help("Maximum 'k' different goals in 'm' segments of all goals. Defaults to 0.").default_value(std::string("0"));
    program.add_argument("-gmm", "--goals-max-m").help("Maximum 'k' different goals in 'm' segments of all goals. Defaults to 100.").default_value(std::string("100"));
    program.add_argument("-rdfp", "--real-dist-file-path").help("Path to the real distribution data file. Defaults to './data/order_data.csv'.").default_value(std::string("./data/order_data.csv"));
    program.add_argument("-na", "--num-agents").help("Number of agents to use.").required();
    program.add_argument("-ac", "--agent-capacity").help("Capacity of agents.").default_value(std::string("100"));
    program.add_argument("-rs", "--random-seed").help("Seed for random number generation. Defaults to 0.").default_value(std::string("0"));
    program.add_argument("-tls", "--time-limit-sec").help("Time limit in seconds. Defaults to 10.").default_value(std::string("10"));
    program.add_argument("-osrf", "--output-step-file").help("Path to the step result output file. Defaults to './result/step_result.txt'.").default_value(std::string("./result/step_result.txt"));
    program.add_argument("-ocf", "--output-csv-file").help("Path to the throughput output file. Defaults to './result/throughput.csv'.").default_value(std::string("./result/result.csv"));
    program.add_argument("-otf", "--output-throughput-file").help("Path to the throughput output file. Defaults to './result/throughput.csv'.").default_value(std::string("./result/throughput.csv"));
    program.add_argument("-vof", "--visual-output-file").help("Path to the visual output file. Defaults to './result/vis.yaml'.").default_value(std::string("./result/vis.yaml"));
    program.add_argument("-slf", "--short-log-format").help("Enable short log format. Implicitly true when set.").default_value(false).implicit_value(true);
    program.add_argument("-dl", "--debug-log").help("Enable debug logging. Implicitly true when set.").default_value(false).implicit_value(true);

    try {
        program.parse_known_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        parser_console->error("{}", err.what());
        std::exit(1);
    }

    map_file = program.get<std::string>("map-file");

    cache_type_input = program.get<std::string>("cache-type");
    look_ahead_num = std::stoi(program.get<std::string>("look-ahead-num"));
    delay_deadline_limit = std::stoi(program.get<std::string>("delay-deadline-limit"));

    num_goals = std::stoi(program.get<std::string>("num-goals"));
    goals_gen_strategy_input = program.get<std::string>("goals-gen-strategy");
    goals_max_k = std::stoi(program.get<std::string>("goals-max-k"));
    goals_max_m = std::stoi(program.get<std::string>("goals-max-m"));
    real_dist_file_path = program.get<std::string>("real-dist-file-path");

    num_agents = std::stoi(program.get<std::string>("num-agents"));
    agent_capacity = std::stoi(program.get<std::string>("agent-capacity"));

    random_seed = std::stoi(program.get<std::string>("random-seed"));
    time_limit_sec = std::stoi(program.get<std::string>("time-limit-sec"));

    output_step_file = program.get<std::string>("output-step-file");
    output_csv_file = program.get<std::string>("output-csv-file");
    output_throughput_file = program.get<std::string>("output-throughput-file");
    output_visual_file = program.get<std::string>("visual-output-file");

    short_log_format = program.get<bool>("short-log-format");
    debug_log = program.get<bool>("debug-log");

    // Post parse
    _post_parse();

    // Check
    _check();

    // Print
    _print();
}

void Parser::_post_parse() {
    // Set log level
    if (debug_log) parser_console->set_level(spdlog::level::debug);

    // Set cache type
    cache_type = CacheType::NONE;
    if (cache_type_input == "NONE") {
        cache_type = CacheType::NONE;
    }
    else if (cache_type_input == "LRU") {
        cache_type = CacheType::LRU;
    }
    else if (cache_type_input == "FIFO") {
        cache_type = CacheType::FIFO;
    }
    else if (cache_type_input == "RANDOM") {
        cache_type = CacheType::RANDOM;
    }
    else {
        parser_console->error("Invalid cache type!");
        exit(1);
    }

    // Set goal generation strategy
    if (goals_gen_strategy_input == "MK") {
        if (goals_max_k == 0 || goals_max_m == 0) {
            parser_console->error("For MK generation, both --goals-m and --goals-k must have non-zero values.");
            exit(1);
        }
        goals_gen_strategy = GoalGenerationType::MK;
    }
    else if (goals_gen_strategy_input == "Zhang") {
        goals_gen_strategy = GoalGenerationType::Zhang;
    }
    else if (goals_gen_strategy_input == "Real") {
        goals_gen_strategy = GoalGenerationType::Real;
    }
    else {
        parser_console->error("Invalid goals_generation strategy specified.");
        exit(1);
    }

    // Set MT
    MT = std::mt19937(random_seed);
}

void Parser::_check() {
    // Check paras
    if (num_agents > num_goals) {
        parser_console->error("number of goals must larger or equal to number of agents");
        exit(1);
    }
    if (look_ahead_num < 1) {
        parser_console->error("look ahead should be greater than 1");
        exit(1);
    }
}

void Parser::_print() {
    parser_console->info("Map file:         {}", map_file);
    parser_console->info("Cache type:       {}", cache_type_input);
    parser_console->info("Look ahead:       {}", look_ahead_num);
    parser_console->info("Number of goals:  {}", num_goals);
    parser_console->info("Number of agents: {}", num_agents);
    parser_console->info("Goal Generation:  {}", goals_gen_strategy_input);
    if (goals_gen_strategy == GoalGenerationType::MK) {
        parser_console->info("Goals m:          {}", goals_max_m);
        parser_console->info("Goals k:          {}", goals_max_k);
    }
    else if (goals_gen_strategy == GoalGenerationType::Real) {
        parser_console->info("Real disy file:   {}", real_dist_file_path);
    }
    parser_console->info("Seed:             {}", random_seed);
    parser_console->info("Time limit (sec): {}", time_limit_sec);
    parser_console->info("Step file:        {}", output_step_file);
    parser_console->info("CSV file:         {}", output_csv_file);
    parser_console->info("Throughput file:  {}", output_throughput_file);
    parser_console->info("Visual file:      {}", output_visual_file);
    parser_console->info("Log short:        {}", short_log_format);
    parser_console->info("Debug:            {}", debug_log);
}

// Unit test only
Parser::Parser(
    std::string _map_file,
    CacheType _cache_type,
    uint _num_agents) :
    map_file(_map_file),
    cache_type(_cache_type),
    num_agents(_num_agents)
{
    // Set up logger
    if (auto existing_console = spdlog::get("parser"); existing_console != nullptr) parser_console = existing_console;
    else parser_console = spdlog::stderr_color_mt("parser");
    parser_console->set_level(spdlog::level::debug);

    look_ahead_num = 1;
    delay_deadline_limit = 10;

    num_goals = 100;

    goals_gen_strategy = GoalGenerationType::MK;

    goals_max_k = 20;
    goals_max_m = 100;

    MT = std::mt19937(0);

    time_limit_sec = 10;

    _check();
}