"""Compare benchmark runtimes and flag statistically significant slowdowns.

Each benchmark uses a one-sided Welch t-test with a 10% slowdown margin. Repeated,
randomly interleaved runs account for sample variability and help distribute
time-varying runner noise across both versions.
"""

import argparse
import json
import os
import random
import re
import statistics
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path
from scipy import stats

# Per-benchmark significance level for flagging a slowdown.
PVALUE_THRESHOLD = 0.05

def clean_benchmark_name(name: str) -> str:
    """Clean the benchmark name by removing the min_time suffix."""
    return re.sub(r"/min_time:[\d.]+", "", name)

def run_single_benchmark(executable, output_file):
    """Run a single benchmark executable and return the results as a dictionary mapping
    benchmark names to CPU times."""

    subprocess.run(
        [
            executable,
            "--benchmark_repetitions=1",
            "--benchmark_out_format=json",
            f"--benchmark_out={output_file}",
        ],
        check=True,
    )
    with open(output_file, encoding="utf-8") as f:
        data = json.load(f)
    return {clean_benchmark_name(b["name"]): b["cpu_time"] for b in data["benchmarks"]}

def compare_results(main_times, current_times):
    """Compare benchmark results from the main/current branches  and write a summary
    to a markdown file."""

    local_summary_path = Path(__file__).resolve().parent / "benchmark_summary.md"
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY") or local_summary_path
    success = True

    with open(summary_path, "w", encoding="utf-8") as summary:
        print("## Benchmark Comparison Results", file=summary)
        print("| Benchmark | Main Mean | Main Stddev | Current Mean | Current Stddev | Diff Mean | p value |", file=summary)
        print("|-----------|-----------|-------------|--------------|----------------|-----------|---------|", file=summary)

        for name in main_times:
            main_vals = main_times[name]
            main_mean = statistics.mean(main_vals)
            main_stddev = statistics.stdev(main_vals)

            if name in current_times:
                current_vals = current_times[name]
                current_mean = statistics.mean(current_vals)
                current_stddev = statistics.stdev(current_vals)

                res = stats.ttest_ind_from_stats(
                    mean1=current_mean,
                    std1=current_stddev,
                    nobs1=len(current_vals),
                    mean2=1.1 * main_mean, # 10% mean slowdown tolerance
                    std2=main_stddev,
                    nobs2=len(main_vals),
                    equal_var=False,
                    alternative="greater"
                )

                symbol = "✅"
                if res.pvalue < PVALUE_THRESHOLD:
                    symbol = "❌"
                    print(f"::error::Benchmark '{name}' is {current_mean / main_mean - 1:.2%} slower than main")
                    success = False

                print(f"| {symbol} {name} | {main_mean:.2f} | {main_stddev:.2f} | {current_mean:.2f} | {current_stddev:.2f} | {current_mean / main_mean - 1:.2%} | {res.pvalue:.2f} |", file=summary)
            else:
                print(f"| ⚠️ {name} (missing) | {main_mean:.2f} | {main_stddev:.2f} | - | - | - | - |", file=summary)
                print(f"::warning::Benchmark '{name}' from main not found in current branch")

        for name in set(current_times.keys()) - set(main_times.keys()):
            current_vals = current_times[name]
            current_mean = statistics.mean(current_vals)
            current_stddev = statistics.stdev(current_vals)
            print(f"| 🆕 {name} (new) | - | - | {current_mean:.2f} | {current_stddev:.2f} | - | - |", file=summary)
            print(f"::notice::New benchmark '{name}' found in current branch")

    return success

def main():
    parser = argparse.ArgumentParser(description="Run interleaved benchmarks and compare CPU times.")
    parser.add_argument("main", help="Path to the main benchmark executable")
    parser.add_argument("current", help="Path to the current benchmark executable")
    parser.add_argument("--repetitions", type=int, default=10, help="Number of repetitions per executable")
    args = parser.parse_args()

    main_times = defaultdict(list)
    current_times = defaultdict(list)
    main_runs = 0
    current_runs = 0

    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_output = Path(tmp_dir) / "output.json"

        for _ in range(args.repetitions * 2):
            if main_runs < args.repetitions and current_runs < args.repetitions:
                run_main = random.random() < 0.5
            elif main_runs < args.repetitions:
                run_main = True
            else:
                run_main = False

            try:
                if run_main:
                    for name, cpu_time in run_single_benchmark(args.main, tmp_output).items():
                        main_times[name].append(cpu_time)
                    main_runs += 1
                else:
                    for name, cpu_time in run_single_benchmark(args.current, tmp_output).items():
                        current_times[name].append(cpu_time)
                    current_runs += 1
            except subprocess.CalledProcessError as e:
                print(f"Error running benchmark: {e}")
                sys.exit(1)

    return compare_results(main_times, current_times)

if __name__ == "__main__":
    if not main():
        print("::error::Some benchmarks are significantly slower than main!")
        sys.exit(1)
