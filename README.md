# Parallel Text Parsing: Regex Matching

This repository is for our CSE305 Concurrent Programming project at Ecole Polytechnique. We work on the "parallel text parsing" topic, and more specifically on the regular expression matching track.

At this stage, the repository is being developed progressively. The first goal is to obtain a correct sequential DFA baseline, then a parallel CPU implementation, then benchmarks and optimizations. We want the final repository to show the real evolution of the project, so the implementation will be built in small, understandable steps rather than in one large commit at the end.

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

Holub and Stekr provide the baseline parallel DFA idea that we plan to implement first. PaREM and SFA are useful for possible optimizations or discussion after the baseline is correct.

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

Here `n` is the input size, `|Q|` is the number of DFA states, and `p` is the number of threads. This means the method can help for large texts and small or moderate DFAs, but the factor `|Q|` can become expensive. This tradeoff will be part of the final analysis.

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

The first parallel version will follow Holub and Stekr's general DFA method for shared-memory machines:

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

For our project, the `pruned` mode is a PaREM-inspired version adapted to our DFA matcher. It uses a small boundary depth, currently two characters, to compute candidate states with the same `S intersect L` idea. It also stores the useful effect of a chunk as route vectors instead of always carrying a full mapping for every DFA state. The route vectors are then combined with an associative tree reduction. This gives us a second parallel mode to compare with the full Holub-style enumeration.

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

- Extend the matcher to count occurrences if feasible.
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

### Milestone 6: Optional extensions

- Add a small SFA prototype or a theoretical comparison.
- Consider a GPU version only if the CPU version is already correct, benchmarked, and documented.

## Benchmark Plan

We plan to benchmark the following dimensions:

| Dimension | Planned values |
| --- | --- |
| Text size | 1 million, 50 million, 500 million, and 1 billion characters |
| Thread count | 1, 2, 3, 4, 8, and 16 |
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

We should not claim performance results before benchmarks are actually run.

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

## Expected Final Repository Structure

The repository is currently at the roadmap stage. The intended structure may evolve, but the target is:

```text
src/
  main.cpp
  dfa.hpp
  dfa.cpp
  nfa.hpp
  nfa.cpp
  regex_parser.hpp
  regex_parser.cpp
  parallel_matcher.hpp
  parallel_matcher.cpp
  benchmark.cpp

tests/
  test_dfa.cpp
  test_regex.cpp
  test_parallel.cpp

scripts/
  generate_inputs.py
  run_benchmarks.py
  plot_results.py

data/
  small examples or descriptions of generated data

results/
  CSV files and plots produced later

report/
  report draft and figures

slides/
  final presentation material

README.md
CMakeLists.txt or Makefile
```

Large generated input files should not be committed. The repository should contain scripts and small examples, not huge benchmark data.

## Build And Run

The build system is CMake. On a machine with CMake installed, the usual commands are:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The command-line interface currently supports sequential, full parallel, pruned parallel, and SFA matching. The default task is `search`, because that is the closest to the project statement. The `full` task is still available for the simpler automata membership problem and for validation.

```bash
./regex_matcher --regex "abb" --text "xxabbxx" --mode sequential --task search
./regex_matcher --regex "abb" --text "xxabbxx" --mode parallel --task search --threads 4
./regex_matcher --regex "abb" --text "xxabbxx" --mode pruned --task search --threads 4
./regex_matcher --regex "abb" --text "xxabbxx" --mode sfa --task search --threads 4
./regex_matcher --regex "(a|b)*" --text "abba" --mode parallel --task full --threads 4
```

### CLI Options

The executable can be run with different tasks and algorithms, so the evaluator can choose the case they want to test.

The most useful options are:

- `--regex PATTERN`, regex to compile;
- `--text TEXT`, input text given directly on the command line;
- `--input FILE`, input text read from a file;
- `--task search|full`, choose substring search or full-text acceptance;
- `--mode sequential|parallel|pruned|sfa`, choose the matching algorithm;
- `--threads N`, number of worker threads for the parallel modes.

The default task is `search`, because it is closest to the project statement about looking for a pattern inside a large text. In this task, the program builds a DFA for `Sigma* pattern Sigma*`, so running the usual DFA acceptance algorithm answers the question "does the pattern occur somewhere in the text?". The `full` task is still kept because it is the clean automata membership problem and it is useful for validating the parallel DFA algorithm.

Examples:

```bash
./regex_matcher --regex "abb" --text "xxabbxx" --task search --mode sequential
./regex_matcher --regex "abb" --text "xxabbxx" --task search --mode parallel --threads 4
./regex_matcher --regex "(a|b)*" --text "abba" --task full --mode sfa --threads 4
./regex_matcher --regex "error" --input data/log.txt --task search --mode pruned --threads 4
```

All modes use the same regex-to-DFA pipeline first. The difference is only how the resulting automaton is evaluated on the text:

- `sequential` runs the DFA normally from left to right;
- `parallel` follows the Holub-style chunk mapping method;
- `pruned` adds the PaREM-inspired candidate pruning and route representation;
- `sfa` uses the precomputed simultaneous finite automaton representation.

The benchmark scripts are a first baseline for the current implementation. They generate input files, run the sequential, full parallel, pruned parallel, and SFA matchers on both `full` and `search` tasks, and prepare a CSV summary:

```bash
python3 scripts/generate_inputs.py
python3 scripts/run_benchmarks.py --exe build/regex_matcher --repeats 5 --threads 1,2,3,4,8,16 --tasks search,full --modes sequential,parallel,pruned,sfa --warmup 1
python3 scripts/plot_results.py
```

This produces both detailed and summarized outputs:

- `results/benchmark_summary.csv`, detailed averages for each regex case
- `results/benchmark_overview.csv`, averages grouped by task, input size, mode, and thread count
- `results/benchmark_report_speedups.csv`, a compact table for the report
- `results/benchmark_case_details.csv`, per-regex details for comparing simple and larger cases
- `results/plots/`, detailed per-regex plots
- `results/plots_summary/`, easier-to-read summary plots

For a quick local smoke test, use only the smallest generated inputs:

```bash
python3 scripts/generate_inputs.py --sizes medium
python3 scripts/run_benchmarks.py --exe build/regex_matcher --sizes medium --repeats 1 --threads 1 --modes sequential,parallel --tasks search
```

Generated input files and result files are ignored by Git. We will use the same scripts as a starting point for the final benchmark comparison.

### Benchmark Analysis Summary

We ran the benchmark campaign in Release mode on one reference machine, using both tasks:

- full-text acceptance (`full`);
- substring search (`search`).

The benchmark compared the four implemented modes: `sequential`, `parallel`, `pruned`, and `sfa`. For each case, results were averaged over several regexes of the same task. The run used 1, 2, 3, and 4 threads, with 5 measured repetitions and one warmup run.

The detailed CSV and per-regex plots are useful for checking individual cases, but the most readable results are the averaged summaries. On the largest input size (`huge`), the average speedups at 4 threads were:

| Task | parallel | pruned | sfa |
| --- | ---: | ---: | ---: |
| search | 1.63x | 2.49x | 3.17x |
| full | 2.01x | 2.99x | 3.17x |

These results match the expected behavior from the algorithms. For small inputs, the parallel versions do not always help because thread creation, file reading, automaton construction, and reduction overhead dominate. For larger inputs, the scan cost becomes large enough that parallelism is useful. The basic Holub-style `parallel` mode works, but it pays the cost of considering several DFA states. The `pruned` mode improves this by reducing candidate states and using compact routes. The `sfa` mode gives the best average speedup on the largest inputs, but it also has a construction cost and possible state-growth cost, so it should be interpreted as a precomputation-oriented approach.

The main conclusion is that parallel regex matching is not automatically faster. It becomes useful when the input is large enough and when the extra automaton work is amortized. This is the main CSE305 point shown by the benchmark.

The final report will state the machine used for the benchmark runs, including the CPU and number of cores. The code is intended to be buildable with CMake on the Salle info machines.

## Current Status

Current repository status:

- project topic chosen: parallel regex matching
- main references identified: Holub and Stekr, PaREM, SFA
- planned approach fixed: sequential DFA first, parallel CPU DFA second, benchmarks third, optimizations after correctness
- repository roadmap created in this README
- first C++ project skeleton added
- basic DFA representation and sequential full-text acceptance started
- first small DFA test added using a manually built automaton
- first regex tokenizer started for literals, |, *, and parentheses
- NFA skeleton with states, transitions, epsilon moves, and sequential acceptance on a hand-built automaton
- small NFA test added for a manual `a|b` automaton
- regex parser can build postfix notation for literals, concatenation, union, star, and parentheses
- Thompson construction builds an NFA from postfix regexes for our supported syntax
- subset construction converts NFA to DFA for the supported regex subset
- end-to-end sequential matcher added: regex -> NFA -> DFA -> accepts(text)
- end-to-end sequential tests added
- dense DFA transition table added as the planned base for the optimized parallel matchers
- full and pruned modes now use the dense DFA transition table
- parallel DFA chunk simulation added, with final state reconstruction from the initial DFA state
- parallel multi-threaded DFA matching added (chunk mappings computed with std::thread)
- PaREM-inspired candidate filtering added with a two-character boundary depth
- pruned mode stores compact route vectors instead of full mappings when possible
- route vectors can be combined with a parallel tree-style reduction
- pruned parallel mode exposed in the CLI and benchmark runner
- baseline benchmark scripts added for input generation, timing, CSV summaries, and plots
- plots compare all non-sequential modes against the sequential baseline
- SFA construction and sequential SFA matching added
- parallel SFA mode exposed in the CLI and benchmark runner
- search task added through a `Sigma* pattern Sigma*` automaton, reusing the same matching engines
- benchmark runner separates full-text regex cases from search regex cases
- final benchmark comparison has been run locally, with CSV summaries and plots generated
- report benchmark tables and plots have been prepared from the measured results

The next step is to finalize the report and slides, making sure the benchmark discussion stays consistent with the measured results and the project references. The main implemented algorithms are now the sequential baseline, the full parallel matcher, the pruned parallel matcher, and the SFA matcher, each usable for both full-text acceptance and search.

## References

- CSE305 project description, "Parallel text parsing"
- CSE305 course slides on lightweight threads, PRAM, CUDA, mutual exclusion, condition variables, and concurrent data structures
- Jan Holub and Stanislav Stekr, "On Parallel Implementations of Deterministic Finite Automata", 2009
- Suejb Memeti and Sabri Pllana, "PaREM: A Novel Approach for Parallel Regular Expression Matching", 2014
- Ryoma Sinya, Kiminori Matsuzaki, and Masataka Sassa, "Simultaneous Finite Automata: An Efficient Data-Parallel Model for Regular Expression Matching", 2013

## Acknowledgement

We acknowledge using AI-assisted tools, mainly Codex, for debugging help, code review, LaTeX editing, and benchmark/plotting support. All algorithmic choices, implementation details, and final explanations are reviewed and understood by the team.
