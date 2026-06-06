#!/usr/bin/env python3

from pathlib import Path
import argparse
import random


DEFAULT_SIZES = {
    "small": 1_000,
    "medium": 10_000,
    "large": 100_000,
    "xlarge": 1_000_000,
    "xxlarge": 10_000_000,
    "huge": 50_000_000,
}


def write_repeated(path: Path, symbol: str, size: int) -> None:
    path.write_text(symbol * size, encoding="utf-8")


def write_random(path: Path, alphabet: str, size: int, rng: random.Random) -> None:
    text = "".join(rng.choice(alphabet) for _ in range(size))
    path.write_text(text, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate small benchmark inputs.")
    parser.add_argument("--output-dir", default="data/benchmark_inputs")
    parser.add_argument("--seed", type=int, default=305)
    parser.add_argument("--sizes", default=",".join(DEFAULT_SIZES.keys()))
    args = parser.parse_args()

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    rng = random.Random(args.seed)
    sizes = [value.strip() for value in args.sizes.split(",") if value.strip()]

    for label in sizes:
        size = DEFAULT_SIZES[label]
        write_repeated(output_dir / f"a_only_{label}.txt", "a", size)
        write_random(output_dir / f"ab_random_{label}.txt", "ab", size, rng)
        write_random(output_dir / f"abc_random_{label}.txt", "abc", size, rng)

    print(f"wrote benchmark inputs to {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
