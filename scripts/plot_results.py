#!/usr/bin/env python3

from collections import defaultdict
from pathlib import Path
import argparse
import csv


def average(values: list[float]) -> float:
    return sum(values) / len(values)


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as csv_file:
        return list(csv.DictReader(csv_file))


def compute_summary(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    grouped: dict[tuple[str, str, str, str, str], list[float]] = defaultdict(list)
    for row in rows:
        task = row.get("task", "full")
        key = (task, row["case"], row["size_label"], row["mode"], row["threads"])
        grouped[key].append(float(row["runtime_seconds"]))

    sequential_times: dict[tuple[str, str, str], float] = {}
    for (task, case, size_label, mode, threads), values in grouped.items():
        if mode == "sequential" and threads == "1":
            sequential_times[(task, case, size_label)] = average(values)

    summary = []
    for (task, case, size_label, mode, threads), values in sorted(grouped.items()):
        runtime = average(values)
        baseline = sequential_times.get((task, case, size_label), runtime)
        speedup = baseline / runtime if runtime > 0 else 0.0
        thread_count = int(threads)
        efficiency = speedup / thread_count if thread_count > 0 else 0.0
        summary.append({
            "task": task,
            "case": case,
            "size_label": size_label,
            "mode": mode,
            "threads": threads,
            "avg_runtime_seconds": f"{runtime:.8f}",
            "speedup_vs_sequential": f"{speedup:.4f}",
            "efficiency": f"{efficiency:.4f}",
        })

    return summary


def write_summary(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def try_plot(summary: list[dict[str, str]], output_dir: Path) -> None:
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not installed, wrote CSV summary only")
        return

    output_dir.mkdir(parents=True, exist_ok=True)
    comparison_rows = [row for row in summary if row["mode"] != "sequential"]
    if not comparison_rows:
        print("no non-sequential benchmark rows to plot")
        return

    for task in sorted({row.get("task", "full") for row in comparison_rows}):
        task_rows = [row for row in comparison_rows if row.get("task", "full") == task]

        for case in sorted({row["case"] for row in task_rows}):
            case_rows = [row for row in task_rows if row["case"] == case]

            for size_label in sorted({row["size_label"] for row in case_rows}):
                rows = [row for row in case_rows if row["size_label"] == size_label]

                plt.figure(figsize=(7, 4))
                for mode in sorted({row["mode"] for row in rows}):
                    mode_rows = sorted(
                        [row for row in rows if row["mode"] == mode],
                        key=lambda row: int(row["threads"]),
                    )
                    threads = [int(row["threads"]) for row in mode_rows]
                    speedups = [float(row["speedup_vs_sequential"]) for row in mode_rows]
                    plt.plot(threads, speedups, marker="o", label=mode)

                plt.xlabel("threads")
                plt.ylabel("speedup vs sequential")
                plt.title(f"Speedup comparison, {task}, {case}, {size_label}")
                plt.legend()
                plt.tight_layout()
                plt.savefig(output_dir / f"speedup_{task}_{case}_{size_label}.png")
                plt.close()

    print(f"wrote plots to {output_dir}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize and plot benchmark CSV results.")
    parser.add_argument("--input", default="results/benchmark_baseline.csv")
    parser.add_argument("--summary", default="results/benchmark_summary.csv")
    parser.add_argument("--plot-dir", default="results/plots")
    args = parser.parse_args()

    rows = load_rows(Path(args.input))
    summary = compute_summary(rows)
    write_summary(Path(args.summary), summary)
    try_plot(summary, Path(args.plot_dir))
    print(f"wrote summary to {args.summary}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
