CAL-MAPF, a caching approach in the warehouse based on the LACAM algorithm in Lifelong Muli-Agent Path Finding

## Building

All you need is [CMake](https://cmake.org/) (≥v3.16). The code is written in C++(17).

First, clone this repo with submodules.

```sh
git clone --recursive https://github.com/HarukiMoriarty/CAL-MAPF.git
cd CAL-MAPF
```
Then, install dependencies

```sh
sudo apt-get update
sudo apt-get install -y libspdlog-dev libfmt-dev libboost-all-dev
```

Last, build the project.

```sh
cmake -B build && make -C build
```

## Command

```
-ac / --agent-capacity          | Capacity of agents. Defaults to 100.
-ct / --cache-type              | Type of cache to use: NONE, LRU, FIFO, RANDOM. Defaults to NONE.
-dl / --debug-log               | Enable debug logging. Implicitly true when set.
-ddl / --delay-deadline-limit   | Delay deadline limit for task assignment. Defaults to 1.
-ggs / --goals-gen-strategy     | Strategy for goals generation: MK, Zhang, Real. (Required)
-gmk / --goals-max-k            | Maximum 'k' different goals in 'm' segments of all goals. Defaults to 0.
-gmm / --goals-max-m            | Maximum 'k' different goals in 'm' segments of all goals. Defaults to 100.
-lan / --look-ahead-num         | Number for look-ahead logic. Defaults to 1.
-mf / --map-file                | Path to the map file. (Required)
-na / --num-agents              | Number of agents to use. (Required)
-ng / --num-goals               | Number of goals to achieve. (Required)
-ocf / --output-csv-file        | Path to the throughput output file. Defaults to './result/result.csv'.
-osrf / --output-step-file      | Path to the step result output file. Defaults to './result/step_result.txt'.
-otf / --output-throughput-file | Path to the throughput output file. Defaults to './result/throughput.csv'.
-op / --optimize                | Enable optimization. Enable checking empty space for cache insert while moving.
-rdfp / --real-dist-file-path   | Path to the real distribution data file. Defaults to './data/order_data.csv'.
-rs / --random-seed             | Seed for random number generation. Defaults to 0.
-slf / --short-log-format       | Enable short log format. Implicitly true when set.
-tls / --time-limit-sec         | Time limit in seconds. Defaults to 10.
-vof / --visual-output-file     | Path to the visual output file. Defaults to './result/vis.yaml'.
```

## Assumption

1. Assume cargo in the warehouse is infinite