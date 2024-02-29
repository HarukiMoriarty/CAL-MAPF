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

## Assumption

1. Assume cargo in the warehouse is infinite
2. Assume agents can bring infinite cargos back to the cache
3. Assume cargoes that are evicted from the cache can immediately disappear