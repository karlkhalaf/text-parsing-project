#!/usr/bin/env python3

from pathlib import Path
import argparse
import csv
import subprocess
import time


FULL_CASES = [
    ("a_star", "a*", "a_only"),
    ("ab_star", "(a|b)*", "ab_random"),
    ("abc_star", "(a|b|c)*", "abc_random"),
]

SEARCH_CASES = [
    ("aaa_search", "aaa", "a_only"),
    ("abb_search", "abb", "ab_random"),
    ("abc_search", "abc", "abc_random"),
    ("ab_complex_search", "(a|b)*abb", "ab_random"),
]

SIZES = ["small", "medium", "large", "xlarge"]


def cases_for_task(task: str) -> list[tuple[str, str, str]]:
    if task == "full":
        return FULL_CASES
    if task == "search":
        return SEARCH_CASES
    raise ValueError(f"unknown task: {task}")


def run_once(executable: Path, pattern: str, input_path: Path, mode: str, task: str, threads: int) -> tuple[float, str]:
    command = [
        str(executable),
        "--regex",
        pattern,
        "--input",
        str(input_path),
        "--mode",
        mode,
        "--task",
        task,
        "--threads",
        str(threads),
    ]

    start = time.perf_counter()
    completed = subprocess.run(command, text=True, capture_output=True)
    elapsed = time.perf_counter() - start

    if completed.returncode not in (0, 2):
        message = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"benchmark command failed: {message}")

    return elapsed, completed.stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a small sequential/parallel benchmark.")
    parser.add_argument("--exe", default="build/regex_matcher")
    parser.add_argument("--input-dir", default="data/benchmark_inputs")
    parser.add_argument("--output", default="results/benchmark_baseline.csv")
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--threads", default="1,2,3,4")
    parser.add_argument("--modes", default="sequential,parallel,pruned,sfa")
    parser.add_argument("--tasks", default="search,full")
    args = parser.parse_args()

    executable = Path(args.exe)
    input_dir = Path(args.input_dir)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    thread_counts = [int(value) for value in args.threads.split(",") if value.strip()]
    modes = [value.strip() for value in args.modes.split(",") if value.strip()]
    tasks = [value.strip() for value in args.tasks.split(",") if value.strip()]

    rows = []
    for task in tasks:
        for case_name, pattern, file_prefix in cases_for_task(task):
            for size_label in SIZES:
                input_path = input_dir / f"{file_prefix}_{size_label}.txt"
                text_size = input_path.stat().st_size

                for repeat in range(args.repeats):
                    for mode in modes:
                        mode_threads = [1] if mode == "sequential" else thread_counts

                        for threads in mode_threads:
                            elapsed, result = run_once(executable, pattern, input_path, mode, task, threads)
                            rows.append({
                                "task": task,
                                "case": case_name,
                                "pattern": pattern,
                                "input": str(input_path),
                                "size_label": size_label,
                                "text_size": text_size,
                                "mode": mode,
                                "threads": threads,
                                "repeat": repeat,
                                "runtime_seconds": f"{elapsed:.8f}",
                                "result": result,
                            })

    with output_path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print(f"wrote benchmark results to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
