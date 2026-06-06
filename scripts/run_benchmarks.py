#!/usr/bin/env python3

from pathlib import Path
import argparse
import csv
import os
import re
import subprocess
import time


FULL_CASES = [
    ("a_star", "a*", "a_only"),
    ("abc_star", "(a|b|c)*", "abc_random"),
    ("abc_contains_tail", "(a|b|c)*abc(a|b|c)*", "abc_random"),
    ("abc_contains_block", "(a|b|c)*(ab*cac*b)(a|b|c)*", "abc_random"),
]

SEARCH_CASES = [
    ("aaa_search", "aaa", "a_only"),
    ("abc_search", "abc", "abc_random"),
    ("abc_tail_search", "(a|b|c)*abc", "abc_random"),
    ("abc_block_search", "ab*cac*b", "abc_random"),
]

SIZES = ["medium", "large", "huge", "gigantic"]
PAREM_STATS_PATTERN = re.compile(
    r"PAREM_STATS depth=(?P<depth>\d+) chunks=(?P<chunks>\d+) "
    r"avg_R=(?P<avg_R>[0-9.]+) max_R=(?P<max_R>\d+)"
)


def cases_for_task(task: str) -> list[tuple[str, str, str]]:
    if task == "full":
        return FULL_CASES
    if task == "search":
        return SEARCH_CASES
    raise ValueError(f"unknown task: {task}")


def run_once(
    executable: Path,
    pattern: str,
    input_path: Path,
    mode: str,
    task: str,
    threads: int,
) -> tuple[float, str, dict[str, str]]:
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

    env = os.environ.copy()
    if mode == "pruned":
        env["PAREM_STATS"] = "1"

    start = time.perf_counter()
    completed = subprocess.run(command, text=True, capture_output=True, env=env)
    elapsed = time.perf_counter() - start

    if completed.returncode not in (0, 2):
        message = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"benchmark command failed: {message}")

    stats = {"parem_depth": "", "avg_R": "", "max_R": ""}
    match = PAREM_STATS_PATTERN.search(completed.stderr)
    if match is not None:
        stats = {
            "parem_depth": match.group("depth"),
            "avg_R": match.group("avg_R"),
            "max_R": match.group("max_R"),
        }

    return elapsed, completed.stdout.strip(), stats


def main() -> int:
    parser = argparse.ArgumentParser(description="Run a small sequential/parallel benchmark.")
    parser.add_argument("--exe", default="build/regex_matcher")
    parser.add_argument("--input-dir", default="data/benchmark_inputs")
    parser.add_argument("--output", default="results/benchmark_baseline.csv")
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--threads", default="1,2,3,4,8,16")
    parser.add_argument("--modes", default="sequential,parallel,pruned,sfa")
    parser.add_argument("--tasks", default="search,full")
    parser.add_argument("--sizes", default=",".join(SIZES))
    parser.add_argument("--warmup", type=int, default=0)
    args = parser.parse_args()

    executable = Path(args.exe)
    input_dir = Path(args.input_dir)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    thread_counts = [int(value) for value in args.threads.split(",") if value.strip()]
    modes = [value.strip() for value in args.modes.split(",") if value.strip()]
    tasks = [value.strip() for value in args.tasks.split(",") if value.strip()]
    sizes = [value.strip() for value in args.sizes.split(",") if value.strip()]

    print("benchmark cases")
    for task in tasks:
        for case_name, pattern, file_prefix in cases_for_task(task):
            print(f"{task:6s} {case_name:22s} {pattern:35s} input={file_prefix}")

    rows = []
    for task in tasks:
        for case_name, pattern, file_prefix in cases_for_task(task):
            for size_label in sizes:
                input_path = input_dir / f"{file_prefix}_{size_label}.txt"
                text_size = input_path.stat().st_size

                for mode in modes:
                    mode_threads = [1] if mode == "sequential" else thread_counts

                    for threads in mode_threads:
                        for _ in range(args.warmup):
                            run_once(executable, pattern, input_path, mode, task, threads)

                        for repeat in range(args.repeats):
                            elapsed, result, stats = run_once(
                                executable,
                                pattern,
                                input_path,
                                mode,
                                task,
                                threads,
                            )
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
                                "parem_depth": stats["parem_depth"],
                                "avg_R": stats["avg_R"],
                                "max_R": stats["max_R"],
                            })

    with output_path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    print(f"wrote benchmark results to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
