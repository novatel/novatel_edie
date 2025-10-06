"""
Script to build and run benchmarks on local machine in current and reference states.
Saves results to JSON files for comparison via compare_benchmarks.py.

NOTE: Currently only works on Windows builds due to hardcoded benchmark executable path.
TODO: Add platform detection and appropriate path construction for Linux.
"""

import argparse
import os
import subprocess
import shutil
from typing import List


def run_command(cmd: List[str] | str, cwd: str = "..", check: bool = True,
                show_output: bool = False) -> subprocess.CompletedProcess:
    """Run a shell command and return the result.
    
    Args:
        cmd: Command to run (list of args or string).
        cwd: Directory in which to run the command.
        check: Raise a CalledProcessError if the command fails.

    Returns:
        CompletedProcess instance with command results returned by subprocess.run().
    """
    print(f"Running: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    
    try:
        result = subprocess.run(cmd, shell=isinstance(cmd, str), cwd=cwd, 
                                capture_output=True, text=True, check=check)
        if show_output and result.stdout:
            print(f"Output: {result.stdout.strip()}")
        return result
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {e}")
        print(f"Stdout: {e.stdout}")
        print(f"Stderr: {e.stderr}")
        raise

def build_benchmarks(build_dir: str, cmake_args: List[str] | None = None):
    """Build the project in the specified directory.

    Args:
        build_dir: Directory in which the project is built.
        cmake_args: List of additional CMake arguments.
    """
    cmake_args = cmake_args or []
    
    # Configure
    configure_cmd = [
        "cmake", "-S", ".", "-B", build_dir,
        "-DCMAKE_BUILD_TYPE=Release",
        "-DBUILD_BENCHMARKS=ON", 
        "-DBUILD_TESTS=OFF", 
        "-DBUILD_EXAMPLES=OFF"
    ] + cmake_args
    run_command(configure_cmd)
    
    # Build
    build_cmd = ["cmake", "--build", build_dir, "--config", "Release", "--parallel"]
    run_command(build_cmd)

def run_benchmarks(build_dir: str, output_file: str):
    """Run the benchmark and save results to JSON file.

    Args:
        build_dir: Directory in which the benchmarks are built.
        output_file: Path to save the JSON benchmark results.
    """
    benchmark_exe = os.path.join("..", build_dir, "bin", "windows-AMD64-msvc-Release", "benchmark.exe")
    print(f"Running benchmark executable: {benchmark_exe}")
    
    cmd = [benchmark_exe, "--benchmark_format=json"]
    result = run_command(cmd, cwd=None)
    
    # Skip any non-JSON lines at the beginning
    lines = result.stdout.strip().split('\n')
    json_start = 0
    for i, line in enumerate(lines):
        if line.strip().startswith('{'):
            json_start = i
            break
    
    json_output = '\n'.join(lines[json_start:])
    
    with open(output_file, 'w') as f:
        f.write(json_output)
    
    print(f"Benchmark results saved to {output_file}")

def main():
    parser = argparse.ArgumentParser(description="Compare benchmarks between current state and another commit/branch")
    parser.add_argument("--reference", "-r", default="main", 
                       help="Reference commit/branch to compare against (default: main)")
    parser.add_argument("--build-dir", default="build-benchmark",
                       help="Base directory for builds (default: build-benchmark)")
    parser.add_argument("--cmake-args", nargs="*", default=[],
                       help="Additional CMake arguments")
    parser.add_argument("--keep-builds", action="store_true",
                       help="Keep build directories after comparison")
    
    args = parser.parse_args()
    
    # Get current git info
    current_state = ""
    try:
        # Try to get current branch name. If in detached HEAD, run_command will raise
        # a CalledProcessError
        current_state = run_command(["git", "symbolic-ref", "--short", "HEAD"]).stdout.strip()
        print(f"Current branch: {current_state}")
    except subprocess.CalledProcessError:
        # In detached HEAD state, get current commit hash
        current_state = run_command(["git", "rev-parse", "HEAD"]).stdout.strip()
        print(f"In detached HEAD state at commit: {current_state}")

    print(f"Reference: {args.reference}")
    
    current_build_dir = f"{args.build_dir}-current"
    reference_build_dir = f"{args.build_dir}-{args.reference}"
    stashed = False
    
    try:
        print("\n=== Building current state ===")
        build_benchmarks(current_build_dir, args.cmake_args)
        
        print("\n=== Running current benchmarks ===")
        run_benchmarks(current_build_dir, "current_benchmark.json")
        
        # Stash any uncommitted changes
        stash_result = run_command(["git", "stash", "push", "-m", "benchmark_comparison_temp"], check=False)
        stashed = stash_result.returncode == 0 and "No local changes to save" not in stash_result.stdout

        print(f"\n=== Switching to reference: {args.reference} ===")
        
        # Checkout reference
        run_command(["git", "checkout", args.reference])
        
        # Build and run reference
        print(f"\n=== Building reference state ({args.reference}) ===")
        build_benchmarks(reference_build_dir, args.cmake_args)
        
        print(f"\n=== Running reference benchmarks ===")
        run_benchmarks(reference_build_dir, "main_benchmark.json") # TODO maybe have configurable name in compare_benchmarks.py,
                                                                  # as this is not necessarily 'main'
        
        print("\nBenchmark runs completed. Run compare_benchmarks.py to compare results.")
        
    finally:
        if len(current_state) > 0:
            # Return to original state
            print(f"\n=== Returning to original state ===")
            run_command(["git", "checkout", current_state])
        
        # Restore stashed changes if any
        if stashed:
            run_command(["git", "stash", "pop"], check=False)

        # Cleanup
        if not args.keep_builds:
            print("\n=== Cleaning up build directories ===")
            for build_dir in [current_build_dir, reference_build_dir]:
                rel_dir = os.path.join("..", build_dir)
                if os.path.exists(rel_dir):
                    shutil.rmtree(rel_dir)
                    print(f"Removed {rel_dir}")

if __name__ == "__main__":
    main()