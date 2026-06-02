#!/usr/bin/env python3

from pathlib import Path
import argparse
import csv
import subprocess
import time


CASES = [
    ("a_star", "a*", "a_only"),
    ("ab_star", "(a|b)*", "ab_random"),
    ("abc_star", "(a|b|c)*", "abc_random"),
]

SIZES = ["small", "medium", "large"]


def run_once(executable: Path, pattern: str, text: str, mode: str, threads: int) -> tuple[float, str]:
    command = [
        str(executable),
        "--regex",
        pattern,
        "--text",
        text,
        "--mode",
        mode,
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
    parser.add_argument("--modes", default="sequential,parallel,pruned,precomputed")
    args = parser.parse_args()

    executable = Path(args.exe)
    input_dir = Path(args.input_dir)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    thread_counts = [int(value) for value in args.threads.split(",") if value.strip()]
    modes = [value.strip() for value in args.modes.split(",") if value.strip()]

    rows = []
    for case_name, pattern, file_prefix in CASES:
        for size_label in SIZES:
            input_path = input_dir / f"{file_prefix}_{size_label}.txt"
            text = input_path.read_text(encoding="utf-8")

            for repeat in range(args.repeats):
                for mode in modes:
                    mode_threads = [1] if mode == "sequential" else thread_counts

                    for threads in mode_threads:
                        elapsed, result = run_once(executable, pattern, text, mode, threads)
                        rows.append({
                            "case": case_name,
                            "pattern": pattern,
                            "input": str(input_path),
                            "size_label": size_label,
                            "text_size": len(text),
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
