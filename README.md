# Parallel Text Parsing: Regex Matching

This repository is for our CSE305 Concurrent Programming project at Ecole Polytechnique. We work on the "parallel text parsing" topic, and more specifically on the regular expression matching track.

The repository was developed progressively: first a correct sequential DFA baseline, then a parallel CPU implementation, then pruning, SFA, and benchmarks. We wanted the final repository to show the real evolution of the project, so the implementation was built in small, understandable steps rather than in one large commit at the end.

## Project Overview

Regular expressions are a standard tool for searching patterns in large texts, for example logs, source files, databases, DNA-like sequences, or general text. Mathematically, regular expressions describe regular languages, and a common way to match them is to build a finite automaton and run it through the input string.

The basic sequential DFA computation is simple:

```text
state = initial_state
for each character c in text:
    state = transition[state][c]
```

This takes linear time in the size of the text. The difficulty is that it looks inherently sequential, because the state after position `i` is needed before processing position `i + 1`. The main purpose of the project is to study how this dependency can be reorganized so that several CPU threads can work on different parts of the input.

The implementation now separates two related tasks:

- `full`: decide whether the whole input text is accepted by the regex. This is the clean automata problem used to build and validate the parallel DFA algorithms.
- `search`: decide whether the regex appears somewhere inside the text. This is closer to the project statement about searching large texts. We implement it by building a DFA for `Sigma* pattern Sigma*`, then running the same sequential or parallel engines on that DFA.

Counting occurrences is not implemented yet. It remains a possible extension, because it would require storing more information per chunk than just the ending state.

## Context And Motivation

The project description asks us to implement a parallelized regex or similar matching algorithm and test it on large texts. We choose the regex track because it connects naturally to deterministic finite automata and to the parallel prefix/reduction ideas from the course.

The project is not only about obtaining a fast program. We also need to understand when the parallel algorithm should help, when it should not, and why. In particular, the cost of synchronization, memory access, reduction, and thread management can easily dominate the computation for small inputs or large automata.

Our main references are:

- Holub and Stekr, "On Parallel Implementations of Deterministic Finite Automata"
- Memeti and Pllana, "PaREM: A Novel Approach for Parallel Regular Expression Matching"
- Sinya, Matsuzaki, and Sassa, "Simultaneous Finite Automata"

Holub and Stekr provide the baseline parallel DFA idea used in the project. PaREM and SFA are the two main directions we used for improvements after the baseline was correct.

## Course Connection

This project is meant to stay close to CSE305. The course defines concurrency as several processes or lightweight threads executing at the same time, either by time-sharing, several cores, or both. For this project we focus first on shared-memory CPU parallelism, using either C++ threads or OpenMP.

The PRAM part of the course is especially relevant. The parallel DFA algorithm can be seen as a reduction over functions: each chunk of text produces a function from starting DFA states to ending DFA states, and these functions are composed. Function composition is associative, so it fits the same general reasoning as parallel reductions and prefix computations.

The course also stresses that parallel programming is not automatically faster. Shared memory can create race conditions if several threads write to the same location. Locks can fix correctness issues, but they also introduce overhead and contention. Our implementation should therefore try to make each thread compute local data independently, then combine the results in a controlled reduction phase.

The benchmarks will use the course notions of:

- sequential time `T1`
- parallel time `Tp`
- speedup `Sp = T1 / Tp`
- efficiency `ep = Sp / p`
- work and critical path intuition

## Main Idea

If we split the text into chunks, we cannot simply run chunk 2 independently from chunk 1, because chunk 2 does not know its starting state.

Holub and Stekr's idea is to make each chunk compute more information. Instead of assuming one starting state, a chunk is processed from every possible DFA state. For a DFA with state set `Q`, each chunk produces a mapping:

```text
f_chunk: start_state -> end_state
```

For example, if the text is split into chunks `T0, T1, ..., Tp-1`, then each chunk gives a function:

```text
f0, f1, ..., fp-1
```

The full effect of the text is:

```text
f_total = fp-1 o ... o f1 o f0
```

where `o` means function composition. Since function composition is associative, the mappings can be combined by a reduction. This is the central algorithmic idea of the project.

In the CPU implementation, we do not need to materialize the full composed mapping for the final acceptance result. After all chunk mappings are computed, we can start from the DFA initial state and apply the mappings one after another:

```text
state = initial_state
state = f0[state]
state = f1[state]
...
```

This gives the same final state for full-text matching and avoids composing full mappings when only the initial DFA state is needed. For search, we first transform the regex into a search automaton for `Sigma* pattern Sigma*`, so the same chunk-mapping algorithm can still be used.

The important complexity intuition is:

```text
sequential DFA:        O(n)
parallel DFA idea:     about O(|Q| n / p + |Q| log p)
```

Here `n` is the input size, `|Q|` is the number of DFA states, and `p` is the number of threads. This means the method can help for large texts and small or moderate DFAs, but the factor `|Q|` can become expensive. This tradeoff is one of the main points of the benchmark analysis.

## Sequential DFA Baseline

The first implementation milestone is a clean sequential matcher. It will define the supported regex syntax, build an automaton, and run it on the input text.

The planned pipeline is:

```text
regex -> NFA -> DFA -> sequential DFA run
```

The supported regex syntax is intentionally small, because the goal of the project is the automata and parallel matching algorithm rather than building a full industrial regex engine. The current implementation supports:

- literal non-operator characters;
- implicit concatenation, for example `ab`;
- union with `|`;
- Kleene star with `*`;
- parentheses for grouping.

Features such as `+`, `?`, `.`, character classes, escaping, and full `std::regex` syntax are not supported. This keeps the parser simple enough to explain clearly, while still allowing non-trivial NFAs and DFAs for the parallel algorithms.

The baseline is important because every parallel result will be compared against it.

## Parallel DFA Algorithm

The basic parallel version follows Holub and Stekr's general DFA method for shared-memory machines:

1. Split the input text into `p` chunks.
2. In each thread, compute the effect of the local chunk from every possible DFA state.
3. Store one mapping per chunk.
4. Apply the mappings in chunk order starting from the DFA initial state.
5. Compare the result with the sequential baseline.

For simple acceptance of the full text, each mapping only needs to store the ending state for each possible starting state.

For search, we use the same algorithm on a different DFA. Instead of matching only the pattern automaton, the program builds a search automaton that can skip characters before the match and stay accepting after a match has been found.

For counting matches, the mapping would need to store more information. For each possible starting state, it may need:

```text
end_state
number_of_final_states_reached
```

Then the reduction combines both the ending state and the count. We keep this as future work, because it is more invasive and needs careful tests for chunk boundaries and overlapping matches.

The implementation should avoid unnecessary locks. Each thread can write into its own mapping vector, and the final reconstruction is done after all threads have joined. The theoretical description uses function composition, but the CPU version only follows the state reached from the real initial state.

## PaREM-Inspired Pruned Version

The full Holub-style algorithm tries every DFA state as a possible starting state for each chunk. PaREM improves this by trying to reduce the set of possible initial states for a chunk.

The rough idea is to use transition information around chunk boundaries. For a chunk, one can look at:

- states compatible with the beginning of the current chunk
- states reachable from the end of the previous chunk

PaREM describes this as taking an intersection of candidate sets, often written as something like:

```text
R = S intersect L
```

This can reduce the amount of speculative work, especially when the transition table is sparse.

For our project, the `pruned` mode is a PaREM-inspired version adapted to our DFA matcher. It uses a small boundary depth, currently `k = 3`, to compute candidate states with the same `S intersect L` idea. It also stores the useful effect of a chunk as route vectors instead of always carrying a full mapping for every DFA state. The route vectors are then combined with an associative tree reduction. This gives us a second parallel mode to compare with the full Holub-style enumeration.

## SFA Precomputation Version

Simultaneous Finite Automata are a more advanced approach. Instead of doing speculative simulation at runtime, SFA states represent mappings between states of the original automaton. This can reduce runtime overhead, because the input chunks can be processed as transitions of the SFA.

The tradeoff is that the automaton can become much larger. The SFA paper shows that this can work well for many practical regular expressions, but the state growth is a real concern.

For this project, the precomputation-based extension is the SFA mode. The implementation builds states that are mappings between DFA states, then uses these mappings for sequential and parallel matching. This lets us compare the basic Holub-style version, the pruned version, and a more precomputation-oriented version.

## Implementation Roadmap

### Milestone 1: Sequential baseline

- Define the supported regex syntax.
- Implement or finalize the regex to NFA to DFA pipeline.
- Implement sequential DFA matching.
- Add tests for the supported syntax.
- Compare with `std::regex` on small examples when possible.

### Milestone 2: Basic parallel DFA

- Implement text splitting into chunks.
- For each chunk, compute `start_state -> end_state` for all states.
- Apply mappings in the correct order from the initial state.
- Validate parallel output against the sequential baseline.
- Start with a simple reduction, then improve it if needed.

### Milestone 3: Counting occurrences

- For each chunk and starting state, store both the end state and the number of final states reached.
- Combine counts during reduction.
- Add tests for repeated matches and boundary cases.

### Milestone 4: Benchmarks

- Add scripts for generating inputs.
- Add benchmark scripts that write CSV files.
- Add plotting scripts for the report.
- Test different input sizes, regexes, thread counts, and text types.

### Milestone 5: PaREM-inspired pruning

- Add candidate-state pruning around chunk boundaries.
- Store compact route vectors for the active chunk routes.
- Combine route vectors with an associative reduction.
- Compare full enumeration with the pruned version.
- Measure when pruning helps and when its overhead is not worth it.

## Benchmark Plan

We plan to benchmark the following dimensions:

| Dimension | Planned values |
| --- | --- |
| Text size | 1 million, 50 million, 500 million, and 1 billion characters |
| Thread count | 1, 2, 4, 8, and 16 in the final Ferrari run |
| Regex complexity | full-text star cases, search cases with a required substring, larger DFA |
| Text type | repeated `a`, random `a/b/c` text |
| Algorithm | sequential, parallel, pruned, sfa |

The benchmark runner separates the two tasks:

- full-text cases include two simple regexes, `a*` and `(a|b|c)*`, plus two larger automata, `(a|b|c)*abc(a|b|c)*` and `(a|b|c)*(ab*cac*b)(a|b|c)*`;
- search cases use two simple regexes, `aaa` and `abc`, plus two larger patterns, `(a|b|c)*abc` and `ab*cac*b`, because search benchmarks should require a real pattern occurrence and should not rely on regexes that accept the empty string.

The main metrics will be:

- runtime
- speedup, `sequential_time / parallel_time`
- efficiency, `speedup / number_of_threads`
- average candidate set size `R` for the pruned mode

Expected trends to verify:

- for small texts, parallel execution can be slower because of overhead
- for large texts and small DFAs, parallel execution should help
- for large DFAs, the `|Q|` factor can reduce or remove the speedup
- PaREM-style pruning can help when many transitions are impossible
- too many threads can stop helping because of overhead and memory bandwidth

## Correctness And Validation Plan

Correctness comes before speed. Every parallel result must be compared to the sequential baseline.

The tests should include:

- simple literal patterns
- concatenation
- union
- Kleene star
- parentheses
- repeated matching if counting is implemented
- boundary cases where a match crosses a chunk boundary
- random text where sequential and parallel outputs must agree

For the supported regex subset, we can also compare with `std::regex` on small examples. This is useful as an external check, but the final parallel implementation should still be based on our own DFA representation so that the algorithm remains visible.

## Build And Run

The project uses CMake and standard C++17. Starting from a fresh clone or after `git pull`, the full build and test sequence is:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The executable is then available at `build/regex_matcher`. The four main modes used in the report and benchmarks are:

- `sequential`, the dense sequential DFA baseline;
- `parallel`, the Holub-style chunk mapping algorithm;
- `pruned`, the PaREM-inspired version with candidate pruning and route vectors;
- `sfa`, the simultaneous finite automaton version.

The executable also has a `naive` mode, which keeps the older hashmap-based DFA matcher. It is mainly useful to check the effect of the dense DFA transition table and is not part of the final benchmark comparison.

It also supports two tasks:

- `search`, the default task, checks whether the regex occurs somewhere in the text;
- `full` checks whether the whole text is accepted by the regex.

Some small manual examples are:

```bash
./build/regex_matcher --regex "abb" --text "xxabbxx" --task search --mode sequential
./build/regex_matcher --regex "abb" --text "xxabbxx" --task search --mode parallel --threads 4
./build/regex_matcher --regex "abb" --text "xxabbxx" --task search --mode pruned --threads 4
./build/regex_matcher --regex "abb" --text "xxabbxx" --task search --mode sfa --threads 4
./build/regex_matcher --regex "(a|b)*" --text "abba" --task full --mode parallel --threads 4
```

The most useful command-line options are:

- `--regex PATTERN`, regex to compile;
- `--text TEXT`, input text given directly on the command line;
- `--input FILE`, input text read from a file;
- `--task search|full`, choose substring search or full-text acceptance;
- `--mode sequential|parallel|pruned|sfa|naive`, choose the matching algorithm;
- `--threads N`, number of worker threads for the parallel modes.

To reproduce the benchmark campaign from scratch, first build in Release mode as above, then generate the benchmark inputs:

```bash
python3 scripts/generate_inputs.py
```

This creates generated text files in `data/benchmark_inputs/`. The default benchmark sizes are:

- `medium`, 1 million characters;
- `large`, 50 million characters;
- `huge`, 500 million characters;
- `gigantic`, 1 billion characters.

The generated inputs are not committed to Git, and the full default generation needs several GB of disk space. To run exactly the same benchmark setup used for the final results, use:

```bash
python3 scripts/run_benchmarks.py \
  --exe build/regex_matcher \
  --repeats 3 \
  --threads 1,2,4,8,16 \
  --tasks search,full \
  --modes sequential,parallel,pruned,sfa \
  --warmup 1
python3 scripts/plot_results.py
```

The benchmark runner uses the regex cases defined in `scripts/run_benchmarks.py`. The full-text cases are `a*`, `(a|b|c)*`, `(a|b|c)*abc(a|b|c)*`, and `(a|b|c)*(ab*cac*b)(a|b|c)*`. The search cases are `aaa`, `abc`, `(a|b|c)*abc`, and `ab*cac*b`.

The scripts produce:

- `results/benchmark_baseline.csv`, raw timings;
- `results/benchmark_summary.csv`, detailed averages for each regex case;
- `results/benchmark_overview.csv`, averages grouped by task, input size, mode, and thread count;
- `results/benchmark_report_speedups.csv`, a compact table for the report;
- `results/benchmark_case_details.csv`, per-regex details;
- `results/plots/`, detailed per-regex plots;
- `results/plots_summary/`, summary plots.

For a quick smoke test that does not generate the full 1B-character inputs, use:

```bash
python3 scripts/generate_inputs.py --sizes medium
python3 scripts/run_benchmarks.py \
  --exe build/regex_matcher \
  --sizes medium \
  --repeats 1 \
  --threads 1,2 \
  --modes sequential,parallel \
  --tasks search
python3 scripts/plot_results.py
```

On Windows/MSYS2, the executable path may be `build/regex_matcher.exe`. Generated input files and result files are ignored by Git.

### Benchmark Analysis Summary

We ran the benchmark campaign in Release mode on `ferrari.polytechnique.fr`, using both tasks:

- full-text acceptance (`full`);
- substring search (`search`).

The benchmark compared the four implemented modes: `sequential`, `parallel`, `pruned`, and `sfa`. For each case, results were averaged over several regexes of the same task. The final run used input sizes of 1M, 50M, 500M, and 1B characters, thread counts 1, 2, 4, 8, and 16, three measured repetitions, and one warmup run.

The detailed CSV and per-regex plots are useful for checking individual cases, but the most readable results are the averaged summaries. On the largest input size (`gigantic`, 1B characters), the average speedups were:

| Task | Mode | 4 threads | 16 threads |
| --- | --- | ---: | ---: |
| search | parallel | 0.56x | 1.07x |
| search | pruned | 1.28x | 1.84x |
| search | sfa | 1.88x | 2.43x |
| full | parallel | 0.71x | 1.23x |
| full | pruned | 1.39x | 2.10x |
| full | sfa | 2.05x | 2.82x |

These results match the expected behavior from the algorithms. For small inputs, the parallel versions do not always help because file reading, automaton construction, thread creation, memory access, and reduction overhead are large compared with the scan itself. For larger inputs, the scan cost is better amortized. The basic Holub-style `parallel` mode is correct but often weak, especially on the larger regexes, because it pays the cost of considering several DFA states. The `pruned` mode improves this by reducing candidate states, with average candidate sets often around 1.5 to 2 states on the final cases. The `sfa` mode gives the best average speedup on the largest inputs, but it should be read as a precomputation-oriented approach because SFA construction and possible state growth are part of the tradeoff.

The main conclusion is that parallel regex matching is not automatically faster. It becomes useful when the input is large enough and when the extra automaton work is amortized. This is the main CSE305 point shown by the benchmark.

The final report will state the machine used for the benchmark runs, including the CPU and number of cores. The code is intended to be buildable with CMake on the Salle info machines.

## References

- CSE305 project description, "Parallel text parsing"
- CSE305 course slides on lightweight threads, PRAM, CUDA, mutual exclusion, condition variables, and concurrent data structures
- Jan Holub and Stanislav Stekr, "On Parallel Implementations of Deterministic Finite Automata", 2009
- Suejb Memeti and Sabri Pllana, "PaREM: A Novel Approach for Parallel Regular Expression Matching", 2014
- Ryoma Sinya, Kiminori Matsuzaki, and Masataka Sassa, "Simultaneous Finite Automata: An Efficient Data-Parallel Model for Regular Expression Matching", 2013

## Acknowledgement

We acknowledge using AI-assisted tools for debugging help, code review, LaTeX editing, and benchmark/plotting support.
