"""
A script to generate a stub file for a nanobind module with dependencies.
"""

import argparse
import sys
from typing import List
from pathlib import Path
import importlib

from nanobind.stubgen import main as stubgen_main

def parse_options(args: List[str]) -> argparse.Namespace:
    """Parse the command line options."""
    parser = argparse.ArgumentParser(
        prog="cleanup_stubs.py",
        description="Generate stubs for nanobind-based extensions.",
    )

    parser.add_argument(
        "-o",
        "--output-file",
        metavar="FILE",
        dest="output_file",
        default=None,
        help="write generated stubs to the specified file",
    )

    parser.add_argument(
        "-i",
        "--import",
        action="append",
        metavar="PATH",
        dest="imports",
        default=[],
        help="add the directory to the Python import path (can specify multiple times)",
    )

    parser.add_argument(
        "-m",
        "--module",
        metavar="MODULE",
        dest="module",
        help="generate a stub for the specified module (can specify multiple times)",
    )

    parser.add_argument(
        "-p",
        "--pattern-file",
        metavar="FILE",
        dest="pattern_file",
        default=None,
        help="apply the given patterns to the generated stub (see the docs for syntax)",
    )

    parser.add_argument(
        "-d",
        "--dependency-module",
        action="append",
        metavar="FILE",
        dest="dependency_modules",
        default=[],
        help="treat the specified module as a dependency (can specify multiple times)",
    )


    opt = parser.parse_args(args)
    return opt

def call_stubgen(args: argparse.Namespace):
    stubgen_args = []

    stubgen_args.extend(["-m", args.module])
    if args.output_file:
        stubgen_args.extend(["-o", args.output_file])

    if args.pattern_file:
        stubgen_args.extend(["-p", args.pattern_file])

    for imp in args.imports:
        stubgen_args.extend(["-i", imp])

    stubgen_main(stubgen_args)


def main():
    args = parse_options(sys.argv[1:])

    # Setup relative import paths
    for i in args.imports:
        sys.path.insert(0, i)

    for dep in args.dependency_modules:
        importlib.import_module(dep)

    call_stubgen(args)

    # Convert to pathlib Path
    output_path = Path(args.output_file)

    print(f"PATH: {output_path}")
    with open(output_path, "r") as file:
        lines = file.readlines()

    # Process lines to replace exact "import py_common" lines
    processed_lines = []
    for line in lines:
        specifies_dependency = False
        for dep in args.dependency_modules:
            if line.strip() == f"import {dep}":
                processed_lines.append(F"import novatel_edie.{dep} as {dep}\n")
                specifies_dependency = True
        if specifies_dependency or line.strip().startswith("from ."):
            pass
        else:
            processed_lines.append(line)

    # Write back to file
    with open(output_path, "w") as file:
        file.writelines(processed_lines)



if __name__ == "__main__":
    main()
