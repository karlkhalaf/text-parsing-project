#!/usr/bin/env python3

from collections import defaultdict
from pathlib import Path
import argparse
import csv

SIZE_ORDER = ["small", "medium", "large", "xlarge", "xxlarge", "huge", "gigantic"]
MODE_ORDER = ["sequential", "parallel", "pruned", "sfa"]


def average(values: list[float]) -> float:
    return sum(values) / len(values)


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as csv_file:
        return list(csv.DictReader(csv_file))


def compute_summary(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    grouped: dict[tuple[str, str, str, str, str], list[float]] = defaultdict(list)
    r_values: dict[tuple[str, str, str, str, str], list[float]] = defaultdict(list)
    max_r_values: dict[tuple[str, str, str, str, str], list[float]] = defaultdict(list)
    patterns: dict[tuple[str, str], str] = {}
    for row in rows:
        task = row.get("task", "full")
        key = (task, row["case"], row["size_label"], row["mode"], row["threads"])
        grouped[key].append(float(row["runtime_seconds"]))
        patterns[(task, row["case"])] = row.get("pattern", "")
        if row.get("avg_R"):
            r_values[key].append(float(row["avg_R"]))
        if row.get("max_R"):
            max_r_values[key].append(float(row["max_R"]))

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
            "pattern": patterns.get((task, case), ""),
            "size_label": size_label,
            "mode": mode,
            "threads": threads,
            "avg_runtime_seconds": f"{runtime:.8f}",
            "speedup_vs_sequential": f"{speedup:.4f}",
            "efficiency": f"{efficiency:.4f}",
            "avg_R": f"{average(r_values[(task, case, size_label, mode, threads)]):.2f}"
            if r_values[(task, case, size_label, mode, threads)] else "",
            "max_R": f"{max(max_r_values[(task, case, size_label, mode, threads)]):.0f}"
            if max_r_values[(task, case, size_label, mode, threads)] else "",
        })

    return summary


def write_summary(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def size_key(size_label: str) -> int:
    return SIZE_ORDER.index(size_label) if size_label in SIZE_ORDER else len(SIZE_ORDER)


def mode_key(mode: str) -> int:
    return MODE_ORDER.index(mode) if mode in MODE_ORDER else len(MODE_ORDER)


def compute_overview(summary: list[dict[str, str]]) -> list[dict[str, str]]:
    grouped: dict[tuple[str, str, str, str], dict[str, list[float] | set[str]]] = defaultdict(
        lambda: {"runtime": [], "speedup": [], "efficiency": [], "avg_R": [], "max_R": [], "cases": set()}
    )
    for row in summary:
        key = (row["task"], row["size_label"], row["mode"], row["threads"])
        grouped[key]["runtime"].append(float(row["avg_runtime_seconds"]))
        grouped[key]["speedup"].append(float(row["speedup_vs_sequential"]))
        grouped[key]["efficiency"].append(float(row["efficiency"]))
        grouped[key]["cases"].add(row["case"])
        if row.get("avg_R"):
            grouped[key]["avg_R"].append(float(row["avg_R"]))
        if row.get("max_R"):
            grouped[key]["max_R"].append(float(row["max_R"]))

    overview = []
    for (task, size_label, mode, threads), values in sorted(
        grouped.items(),
        key=lambda item: (item[0][0], size_key(item[0][1]), mode_key(item[0][2]), int(item[0][3])),
    ):
        overview.append({
            "task": task,
            "size_label": size_label,
            "mode": mode,
            "threads": threads,
            "case_count": len(values["cases"]),
            "avg_runtime_seconds": f"{average(values['runtime']):.8f}",
            "avg_speedup_vs_sequential": f"{average(values['speedup']):.4f}",
            "avg_efficiency": f"{average(values['efficiency']):.4f}",
            "avg_R": f"{average(values['avg_R']):.2f}" if values["avg_R"] else "",
            "max_R": f"{max(values['max_R']):.0f}" if values["max_R"] else "",
        })

    return overview


def compute_report_table(overview: list[dict[str, str]]) -> list[dict[str, str]]:
    grouped: dict[tuple[str, str, str], dict[str, str]] = {}
    for row in overview:
        key = (row["task"], row["size_label"], row["mode"])
        item = grouped.setdefault(key, {
            "task": row["task"],
            "size_label": row["size_label"],
            "mode": row["mode"],
            "case_count": row["case_count"],
            "speedup_t1": "",
            "speedup_t2": "",
            "speedup_t3": "",
            "speedup_t4": "",
            "efficiency_t4": "",
            "runtime_t4_seconds": "",
            "avg_R_t4": "",
            "max_R_t4": "",
        })
        threads = row["threads"]
        if threads in {"1", "2", "3", "4"}:
            item[f"speedup_t{threads}"] = row["avg_speedup_vs_sequential"]
        if threads == "4":
            item["efficiency_t4"] = row["avg_efficiency"]
            item["runtime_t4_seconds"] = row["avg_runtime_seconds"]
            item["avg_R_t4"] = row.get("avg_R", "")
            item["max_R_t4"] = row.get("max_R", "")

    return [
        grouped[key]
        for key in sorted(grouped, key=lambda key: (key[0], size_key(key[1]), mode_key(key[2])))
    ]


def compute_case_table(summary: list[dict[str, str]]) -> list[dict[str, str]]:
    grouped: dict[tuple[str, str, str, str], dict[str, str]] = {}
    for row in summary:
        key = (row["task"], row["size_label"], row["case"], row["pattern"])
        item = grouped.setdefault(key, {
            "task": row["task"],
            "size_label": row["size_label"],
            "case": row["case"],
            "pattern": row["pattern"],
            "sequential_time": "",
            "parallel_t4_speedup": "",
            "pruned_t4_speedup": "",
            "pruned_avg_R_t4": "",
            "pruned_max_R_t4": "",
            "sfa_t4_speedup": "",
            "parallel_t16_speedup": "",
            "pruned_t16_speedup": "",
            "sfa_t16_speedup": "",
            "best_t4_mode": "",
            "best_t4_speedup": "",
        })

        mode = row["mode"]
        threads = row["threads"]
        if mode == "sequential" and threads == "1":
            item["sequential_time"] = row["avg_runtime_seconds"]
        if threads == "4" and mode in {"parallel", "pruned", "sfa"}:
            item[f"{mode}_t4_speedup"] = row["speedup_vs_sequential"]
            if mode == "pruned":
                item["pruned_avg_R_t4"] = row.get("avg_R", "")
                item["pruned_max_R_t4"] = row.get("max_R", "")
        if threads == "16" and mode in {"parallel", "pruned", "sfa"}:
            item[f"{mode}_t16_speedup"] = row["speedup_vs_sequential"]

    for item in grouped.values():
        candidates = [
            ("parallel", item["parallel_t4_speedup"]),
            ("pruned", item["pruned_t4_speedup"]),
            ("sfa", item["sfa_t4_speedup"]),
        ]
        candidates = [(mode, float(value)) for mode, value in candidates if value]
        if candidates:
            best_mode, best_speedup = max(candidates, key=lambda value: value[1])
            item["best_t4_mode"] = best_mode
            item["best_t4_speedup"] = f"{best_speedup:.4f}"

    return [
        grouped[key]
        for key in sorted(grouped, key=lambda key: (key[0], size_key(key[1]), key[2]))
    ]


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


def try_plot_overview(overview: list[dict[str, str]], output_dir: Path) -> None:
    try:
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not installed, skipped overview plots")
        return

    output_dir.mkdir(parents=True, exist_ok=True)
    tasks = sorted({row["task"] for row in overview})
    sizes = sorted({row["size_label"] for row in overview}, key=size_key)
    largest_sizes = sizes[-3:]

    for task in tasks:
        for size_label in largest_sizes:
            rows = [
                row for row in overview
                if row["task"] == task and row["size_label"] == size_label and row["mode"] != "sequential"
            ]
            if not rows:
                continue

            plt.figure(figsize=(7, 4))
            for mode in [mode for mode in MODE_ORDER if mode != "sequential"]:
                mode_rows = sorted(
                    [row for row in rows if row["mode"] == mode],
                    key=lambda row: int(row["threads"]),
                )
                if not mode_rows:
                    continue
                threads = [int(row["threads"]) for row in mode_rows]
                speedups = [float(row["avg_speedup_vs_sequential"]) for row in mode_rows]
                plt.plot(threads, speedups, marker="o", label=mode)

            plt.xlabel("threads")
            plt.ylabel("average speedup vs sequential")
            plt.title(f"Average speedup by thread count, {task}, {size_label}")
            plt.legend()
            plt.tight_layout()
            plt.savefig(output_dir / f"overview_speedup_threads_{task}_{size_label}.png")
            plt.close()

        plt.figure(figsize=(7, 4))
        for mode in MODE_ORDER:
            y_values = []
            x_labels = []
            for size_label in sizes:
                selected_threads = "1" if mode == "sequential" else "4"
                row = next(
                    (
                        row for row in overview
                        if row["task"] == task
                        and row["size_label"] == size_label
                        and row["mode"] == mode
                        and row["threads"] == selected_threads
                    ),
                    None,
                )
                if row is not None:
                    x_labels.append(size_label)
                    y_values.append(float(row["avg_runtime_seconds"]))
            if y_values:
                plt.plot(x_labels, y_values, marker="o", label=f"{mode} ({'1' if mode == 'sequential' else '4'} threads)")

        plt.xlabel("input size")
        plt.ylabel("average runtime, seconds")
        plt.yscale("log")
        plt.title(f"Average runtime by input size, {task}")
        plt.legend()
        plt.tight_layout()
        plt.savefig(output_dir / f"overview_runtime_by_size_{task}.png")
        plt.close()

        plt.figure(figsize=(7, 4))
        for mode in [mode for mode in MODE_ORDER if mode != "sequential"]:
            y_values = []
            x_labels = []
            for size_label in sizes:
                row = next(
                    (
                        row for row in overview
                        if row["task"] == task
                        and row["size_label"] == size_label
                        and row["mode"] == mode
                        and row["threads"] == "4"
                    ),
                    None,
                )
                if row is not None:
                    x_labels.append(size_label)
                    y_values.append(float(row["avg_speedup_vs_sequential"]))
            if y_values:
                plt.plot(x_labels, y_values, marker="o", label=mode)

        plt.xlabel("input size")
        plt.ylabel("average speedup at 4 threads")
        plt.title(f"Average 4-thread speedup by input size, {task}")
        plt.legend()
        plt.tight_layout()
        plt.savefig(output_dir / f"overview_speedup_by_size_{task}.png")
        plt.close()

    print(f"wrote overview plots to {output_dir}")


def print_table(title: str, headers: list[str], rows: list[list[str]]) -> None:
    print()
    print(title)
    widths = [len(header) for header in headers]
    for row in rows:
        for i, value in enumerate(row):
            widths[i] = max(widths[i], len(value))

    print(" | ".join(header.ljust(widths[i]) for i, header in enumerate(headers)))
    print("-+-".join("-" * width for width in widths))
    for row in rows:
        print(" | ".join(value.ljust(widths[i]) for i, value in enumerate(row)))


def print_terminal_summary(
    overview: list[dict[str, str]],
    report_table: list[dict[str, str]],
    case_table: list[dict[str, str]],
) -> None:
    largest_size = max({row["size_label"] for row in overview}, key=size_key)
    largest_rows = [
        row for row in report_table
        if row["size_label"] == largest_size and row["mode"] != "sequential"
    ]
    speedup_rows = []
    for task in sorted({row["task"] for row in largest_rows}, reverse=True):
        for mode in [mode for mode in MODE_ORDER if mode != "sequential"]:
            row = next((item for item in largest_rows if item["task"] == task and item["mode"] == mode), None)
            if row is None:
                continue
            speedup_rows.append([
                task,
                mode,
                row["speedup_t1"],
                row["speedup_t2"],
                row["speedup_t3"],
                row["speedup_t4"],
                row["efficiency_t4"],
                row["avg_R_t4"],
            ])

    print_table(
        f"{largest_size} input average speedups",
        ["task", "mode", "1 thread", "2 threads", "3 threads", "4 threads", "eff@4", "avg_R@4"],
        speedup_rows,
    )

    case_rows = []
    for row in case_table:
        if row["size_label"] != largest_size:
            continue
        case_rows.append([
            row["task"],
            row["case"],
            row["sequential_time"],
            row["parallel_t4_speedup"],
            row["pruned_t4_speedup"],
            row["pruned_avg_R_t4"],
            row["sfa_t4_speedup"],
            row["best_t4_mode"],
        ])

    print_table(
        f"{largest_size} per-regex 4-thread details",
        ["task", "case", "seq time", "parallel", "pruned", "avg_R", "sfa", "best"],
        case_rows,
    )

    best_rows = []
    for task in sorted({row["task"] for row in overview}, reverse=True):
        for size_label in sorted({row["size_label"] for row in overview if row["task"] == task}, key=size_key):
            task_size_rows = [
                row for row in overview
                if row["task"] == task and row["size_label"] == size_label
            ]
            sequential = next(row for row in task_size_rows if row["mode"] == "sequential")
            best = min(task_size_rows, key=lambda row: float(row["avg_runtime_seconds"]))
            best_rows.append([
                task,
                size_label,
                sequential["avg_runtime_seconds"],
                best["mode"],
                best["threads"],
                best["avg_speedup_vs_sequential"],
            ])

    print_table(
        "Best average mode by input size",
        ["task", "size", "seq time", "best mode", "threads", "speedup"],
        best_rows,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize and plot benchmark CSV results.")
    parser.add_argument("--input", default="results/benchmark_baseline.csv")
    parser.add_argument("--summary", default="results/benchmark_summary.csv")
    parser.add_argument("--overview", default="results/benchmark_overview.csv")
    parser.add_argument("--report-table", default="results/benchmark_report_speedups.csv")
    parser.add_argument("--case-table", default="results/benchmark_case_details.csv")
    parser.add_argument("--plot-dir", default="results/plots")
    parser.add_argument("--overview-plot-dir", default="results/plots_summary")
    args = parser.parse_args()

    rows = load_rows(Path(args.input))
    summary = compute_summary(rows)
    overview = compute_overview(summary)
    report_table = compute_report_table(overview)
    case_table = compute_case_table(summary)
    write_summary(Path(args.summary), summary)
    write_summary(Path(args.overview), overview)
    write_summary(Path(args.report_table), report_table)
    write_summary(Path(args.case_table), case_table)
    try_plot(summary, Path(args.plot_dir))
    try_plot_overview(overview, Path(args.overview_plot_dir))
    print_terminal_summary(overview, report_table, case_table)
    print(f"wrote summary to {args.summary}")
    print(f"wrote overview to {args.overview}")
    print(f"wrote report table to {args.report_table}")
    print(f"wrote case table to {args.case_table}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
