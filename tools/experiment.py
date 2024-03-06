import argparse
import itertools
import logging
import subprocess
import pandas as pd
import yaml

from pathlib import Path
from typing import TypedDict, List

BASE_PATH = Path(__file__).absolute().parent

LOG = logging.getLogger(__name__)
logging.basicConfig(level=logging.INFO, format="%(message)s")

class ExperimentParameters(TypedDict):
    map: List[str]
    cache: List[str]
    look_ahead: List[str]
    delay_deadline: List[str]
    ngoals: List[int]
    gg: List[str]
    goals_k: List[int]
    goals_m: List[int]
    nagents: List[int]
    seed: int  # Assuming a single value for simplicity, modify as needed.
    time_limit_sec: int  # Assuming a single value for simplicity, modify as needed.
    output_step_result: str
    output_csv_result: str
    output_throughput_result: str
    log_short: bool
    debug: bool

def load_experiment(exp_name: str) -> ExperimentParameters | None:
    exp_path = BASE_PATH / "experiment" / f"{exp_name}.yaml"
    if not exp_path.exists():
        LOG.error(f"Experiment file {exp_path} not found.")
        return None

    with open(exp_path) as f:
        return yaml.safe_load(f)

def generate_combinations(params: ExperimentParameters):
    keys = params.keys()
    values = (params[key] if isinstance(params[key], list) else [params[key]] for key in keys)
    for combination in itertools.product(*values):
        yield dict(zip(keys, combination))

def check_and_create_csv(output_csv_path: str):
    # Convert string path to Path object for easier handling
    csv_path = Path(output_csv_path)
    if not csv_path.exists():
        # Ensure the directory exists
        csv_path.parent.mkdir(parents=True, exist_ok=True)
        # Create the file and write the header
        with open(csv_path, 'w') as csv_file:
            csv_file.write("map_name,cache,look_ahead,delay_deadline,goal_generation_type,ngoals,nagents,seed,verbose,time_limit_sec,goals_m,goals_k,cache_hit_rate,makespan,p0_steps,p50_steps,p99steps\n")

def check_and_create_throughput(output_throughput_path: str):
    # Convert string path to Path object for easier handling
    throughput_path = Path(output_throughput_path)
    if not throughput_path.exists():
        # Ensure the directory exists
        throughput_path.parent.mkdir(parents=True, exist_ok=True)
        # Create the file and write the header
        with open(throughput_path, 'w') as csv_file:
            csv_file.write("map_name,cache,look_ahead,delay_deadline,goal_generation_type,ngoals,nagents,seed,verbose,time_limit_sec,goals_m,goals_k\n")

def run_experiment(params: ExperimentParameters):
    check_and_create_csv(params.get("output_csv_result", "./result/result.csv"))
    check_and_create_throughput(params.get("output_throughput_result", "./result/throughput.csv"))

    cmd_base = [
        "./build/CAL-MAPF",
        "--map", params["map"],
        "--cache", params["cache"],
        "--look-ahead", str(params["look_ahead"]),
        "--delay-deadline", str(params["delay_deadline"]),
        "--ngoals", str(params["ngoals"]),
        "-gg", params["gg"],
        "--goals-k", str(params["goals_k"]),
        "--goals-m", str(params["goals_m"]),
        "--nagents", str(params["nagents"]),
        "--seed", str(params.get("seed", 0)),
        "--time_limit_sec", str(params.get("time_limit_sec", 10)),
        "--output_step_result", params.get("output_step_result", "./result/step_result.txt"),
        "--output_csv_result", params.get("output_csv_result", "./result/result.csv"),
        "--output_throughput_result", params.get("output_throughput_result", "./result/throughput.csv")
    ]
    if params.get("log_short", False):
        cmd_base.append("--log_short")
    if params.get("debug", False):
        cmd_base.append("--debug")

    LOG.info(f"Executing: {' '.join(cmd_base)}")
    subprocess.run(cmd_base, check=True)

def main():
    parser = argparse.ArgumentParser(description="Run lacam experiments with different parameters.")
    parser.add_argument("experiment", help="Experiment name to run.")
    args = parser.parse_args()

    exp_params = load_experiment(args.experiment)
    if exp_params is None:
        return

    for combination in generate_combinations(exp_params):
        try:
            run_experiment(combination)
        except subprocess.CalledProcessError as e:
            LOG.error(f"Experiment failed with error: {e}")

if __name__ == "__main__":
    main()
