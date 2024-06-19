/*
 * utility functions
 */
#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <deque>
#include <random>
#include <regex>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fmt/core.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using Time = std::chrono::steady_clock;

// Time manager
struct Deadline {
  Time::time_point t_s;
  const double time_limit_ms;

  Deadline(double _time_limit_ms = 0);
  bool reset();
  double elapsed_ms() const;
  double elapsed_ns() const;
};

double elapsed_ms(const Deadline* deadline);
double elapsed_ns(const Deadline* deadline);
bool is_expired(const Deadline* deadline);

float get_random_float(std::mt19937* MT, float from = 0, float to = 1);
int get_random_int(std::mt19937* MT, int from, int to);

// Vertex
struct Vertex {
  const int id;         // index for V in Graph
  const int index;      // index for U (width * y + x) in Graph
  const int width;      // width of graph
  const int group;      // group number, supported for multiport 
  bool cargo = false;   // indicate cargo vertex
  std::vector<Vertex*> neighbor;

  Vertex(int _id, int _index, int _width, int _group);

  bool operator==(const Vertex& other) const {
    return id == other.id;
  }

  // Reload << operator
  friend std::ostream& operator<<(std::ostream& os, const Vertex& v) {
    int x = v.index % v.width;
    int y = v.index / v.width;
    os << "(" << x << ", " << y << ", " << v.group << ")";
    return os;
  }
};

// Locations for all agents
using Vertices = std::vector<Vertex*>;
using Goals = std::deque<Vertex*>;
using Config = std::vector<Vertex*>;
// Solution: a sequence of configurations
using Solution = std::vector<Config>;

// Overload the spdlog for Vertex
template <>
struct fmt::formatter<Vertex> {
  constexpr auto parse(fmt::format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const Vertex& v, FormatContext& ctx) {
    std::ostringstream oss;
    oss << v;  // Utilizing the existing operator<< overload
    return fmt::format_to(ctx.out(), "{}", oss.str());
  }
};

// Overload the spdlog for Vertices
template <>
struct fmt::formatter<std::vector<Vertex*>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const std::vector<Vertex*>& vec, FormatContext& ctx) {
    fmt::memory_buffer out;
    for (const auto& elem : vec) {
      if (elem != nullptr) {
        std::ostringstream oss;
        oss << *elem;  // Use the overloaded << operator for Vertex
        fmt::format_to(std::back_inserter(out), "{} ", oss.str());
      }
      else {
        fmt::format_to(std::back_inserter(out), "null ");
      }
    }
    return fmt::format_to(ctx.out(), "{}", to_string(out));
  }
};


// Cache Type
enum class CacheType {
  NONE,
  LRU,
  FIFO,
  RANDOM
};

inline bool is_cache(CacheType cache_type) {
  return cache_type != CacheType::NONE;
}

// Cache access result
struct CacheAccessResult {
  bool result;
  Vertex* goal;
  Vertex* garbage;

  inline CacheAccessResult(bool _result, Vertex* _goal, Vertex* _garbage = nullptr) : result(_result), goal(_goal), garbage(_garbage) {};
  bool operator==(const CacheAccessResult& other) const {
    return result == other.result && goal == other.goal && garbage == other.garbage;
  }
};

template <>
struct fmt::formatter<std::vector<unsigned int>> {
  // Presentation format: 'f' - fixed, 'e' - exponential.
  char presentation = 'f';

  // Parses format specifications of the form ['f' | 'e'].
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    // [ctx.begin(), ctx.end()) is a character range that contains a part of
    // the format string starting from the format specifications to be parsed,
    // e.g., in "{:f}", the range will contain "f}". The parser may consume
    // none of this range, in which case it returns ctx.begin().
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

    // Check if reached the end of the range:
    if (it != end && *it != '}') throw format_error("invalid format");

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the vector v to the provided buffer.
  template <typename FormatContext>
  auto format(const std::vector<unsigned int>& v, FormatContext& ctx) const -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.
    format_to(ctx.out(), "[");
    auto sep = "";
    for (auto& elem : v) {
      format_to(ctx.out(), "{}{}", sep, elem);
      sep = ", ";
    }
    return format_to(ctx.out(), "]");
  }
};

// Graph Type
enum class GraphType {
  SINGLE_PORT,
  MULTI_PORT
};

// Goals generation type
enum class GoalGenerationType {
  MK,
  Zhang,
  Real,
};