import json
import sys
import os

with open("current_benchmark.json") as f:
    current_data = json.load(f)

with open("main_benchmark.json") as f:
    main_data = json.load(f)

current = {b["name"]: b["real_time"] for b in current_data["benchmarks"]}
main = {b["name"]: b["real_time"] for b in main_data["benchmarks"]}

summary_path = os.environ.get("GITHUB_STEP_SUMMARY", "benchmark_summary.md")
failed = False

with open(summary_path, "a") as summary:
    print("## Benchmark Comparison Results", file=summary)
    print("| Benchmark | Main (ns) | Current (ns) | Difference |", file=summary)
    print("|-----------|-----------|--------------|------------|", file=summary)

    for name in main:
        main_time = main[name]
        current_time = current.get(name)
        if current_time is not None:
            diff_pct = ((current_time - main_time) / main_time) * 100
            symbol = "✅"
            if diff_pct > 0:
                symbol = "❌"
                print(f"::error::Benchmark '{name}' is {diff_pct:.2f}% slower than main")
                failed = True
            print(f"| {symbol} {name} | {main_time:.2f} | {current_time:.2f} | {diff_pct:+.2f}% |", file=summary)
        else:
            print(f"| ⚠️ {name} (missing) | {main_time:.2f} | - | - |", file=summary)
            print(f"::warning::Benchmark '{name}' from main not found in current branch")

    for name in current:
        if name not in main:
            print(f"| 🆕 {name} (new) | - | {current[name]:.2f} | - |", file=summary)
            print(f"::notice::New benchmark '{name}' found in current branch")

if failed:
    print("::error::Some benchmarks are significantly slower than main!")
    sys.exit(1)
