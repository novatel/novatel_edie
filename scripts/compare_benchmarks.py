import json
import sys
import subprocess
import argparse
import os
import tempfile
from pathlib import Path

def compare_benchmark_outputs(main_file, current_file):
    with open(current_file, encoding="utf-8") as f:
        current_data = json.load(f)

    with open(main_file, encoding="utf-8") as f:
        main_data = json.load(f)

    current_dict = {b["name"]: b["real_time"] for b in current_data["benchmarks"] if b["name"].strip().endswith("_median")}
    main_dict = {b["name"]: b["real_time"] for b in main_data["benchmarks"] if b["name"].strip().endswith("_median")}

    local_summary_path = Path(__file__).resolve().parent / "benchmark_summary.md"
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY") or local_summary_path
    success = True

    with open(summary_path, "w", encoding="utf-8") as summary:
        print("## Benchmark Comparison Results", file=summary)
        print("| Benchmark | Main median (ns) | Current median (ns) | Difference |", file=summary)
        print("|-----------|------------------|---------------------|------------|", file=summary)

        for name in main_dict:
            main_time = main_dict[name]
            current_time = current_dict.get(name)
            if current_time is not None:
                diff_pct = ((current_time - main_time) / main_time) * 100
                symbol = "✅"
                if diff_pct > 10:
                    symbol = "❌"
                    print(f"::error::Benchmark '{name}' is {diff_pct:.2f}% slower than main")
                    success = False
                print(f"| {symbol} {name} | {main_time:.2f} | {current_time:.2f} | {diff_pct:+.2f}% |", file=summary)
            else:
                print(f"| ⚠️ {name} (missing) | {main_time:.2f} | - | - |", file=summary)
                print(f"::warning::Benchmark '{name}' from main not found in current branch")

        for name in set(current_dict.keys()) - set(main_dict.keys()):
            print(f"| 🆕 {name} (new) | - | {current_dict[name]:.2f} | - |", file=summary)
            print(f"::notice::New benchmark '{name}' found in current branch")

    return success

def run_benchmark(executable, repetitions, output_file):
    subprocess.run([executable, f"--benchmark_repetitions={repetitions}", "--benchmark_out_format=json",
                    "--benchmark_report_aggregates_only=true", f"--benchmark_out={output_file}"], check=True)

def main():
    parser = argparse.ArgumentParser(description="Run and compare benchmark results between two executables.")
    parser.add_argument("main", help="Path to the main benchmark executable")
    parser.add_argument("current", help="Path to the current benchmark executable")
    parser.add_argument("--repetitions", type=int, default=10, help="Number of times to repeat each benchmark")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmp_dir:
        main_json = Path(tmp_dir) / "main_benchmark.json"
        current_json = Path(tmp_dir) / "current_benchmark.json"

        try:
            run_benchmark(args.main, args.repetitions, str(main_json))
            run_benchmark(args.current, args.repetitions, str(current_json))
        except subprocess.CalledProcessError as e:
            print(f"Error running benchmarks: {e}")
            sys.exit(1)

        return compare_benchmark_outputs(main_json, current_json)

if __name__ == "__main__":
    if not main():
        print("::error::Some benchmarks are significantly slower than main!")
        sys.exit(1)
