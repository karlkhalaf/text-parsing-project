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

The exact supported syntax will be fixed early and documented in the code and tests. We expect to support a student-project subset such as:

- literal characters
- concatenation
- union with `|`
- Kleene star `*`
- parentheses
- possibly `+`, `?`, and simple character classes if time allows

The baseline is important because every parallel result will be compared against it.

## Parallel DFA Algorithm

The first parallel version will follow Holub and Stekr's general DFA method for shared-memory machines:

1. Split the input text into `p` chunks.
2. In each thread, compute the effect of the local chunk from every possible DFA state.
3. Store one mapping per chunk.
4. Compose the mappings to obtain the final DFA state.
5. Compare the result with the sequential baseline.

For simple acceptance of the full text, each mapping only needs to store the ending state for each possible starting state.

For counting matches, the mapping must store more information. For each possible starting state, it may need:

```text
end_state
number_of_final_states_reached
```

Then the reduction combines both the ending state and the count. This is planned after the basic acceptance/matching version is correct.

The implementation should avoid unnecessary locks. Each thread can write into its own mapping vector, and the reduction phase can be implemented separately.

## Planned PaREM-Inspired Optimization

The full Holub-style algorithm tries every DFA state as a possible starting state for each chunk. PaREM improves this by trying to reduce the set of possible initial states for a chunk.

The rough idea is to use transition information around chunk boundaries. For a chunk, one can look at:

- states compatible with the first character of the current chunk
- states reachable from the last character of the previous chunk

PaREM describes this as taking an intersection of candidate sets, often written as something like:

```text
R = S intersect L
```

This can reduce the amount of speculative work, especially when the transition table is sparse.

For our project, this is an optimization milestone. We should first implement and validate the full Holub-style enumeration. Only after that will we add a PaREM-inspired pruning version and compare it in the benchmarks.

## Possible SFA Extension

Simultaneous Finite Automata are a more advanced approach. Instead of doing speculative simulation at runtime, SFA states represent mappings between states of the original automaton. This can reduce runtime overhead, because the input chunks can be processed as transitions of the SFA.

The tradeoff is that the automaton can become much larger. The SFA paper shows that this can work well for many practical regular expressions, but the state growth is a real concern.

For this project, SFA is optional. A reasonable final goal is to discuss it in the report, and maybe add a small prototype if the sequential matcher, parallel CPU version, and benchmarks are already finished.

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
- Compose mappings in the correct order.
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
- Compare full enumeration with the pruned version.
- Measure when pruning helps and when its overhead is not worth it.

### Milestone 6: Optional extensions

- Add a small SFA prototype or a theoretical comparison.
- Consider a GPU version only if the CPU version is already correct, benchmarked, and documented.

## Benchmark Plan

We plan to benchmark the following dimensions:

| Dimension | Planned values |
| --- | --- |
| Text size | small, medium, large, very large |
| Thread count | 1, 2, 4, 8, 16 if available |
| Regex complexity | simple literal, medium regex, larger DFA |
| Text type | random text, English text, log-like text, DNA-like text |
| Algorithm | sequential, parallel_full, later parallel_parem if implemented |

The main metrics will be:

- runtime
- speedup, `sequential_time / parallel_time`
- efficiency, `speedup / number_of_threads`
- number of DFA states
- memory usage if it is easy to measure reliably

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

This section will be updated once the first implementation is committed.

The build system should stay simple, probably CMake or a Makefile. The code should compile with a standard modern C++ compiler. The first parallel implementation will use CPU shared-memory parallelism, either OpenMP or `std::thread`.

The intended command-line interface may look like this:

```bash
./regex_matcher --regex "ab*c" --input data/sample.txt --mode sequential
./regex_matcher --regex "ab*c" --input data/sample.txt --mode parallel --threads 8
./regex_matcher --regex "ab*c" --input data/sample.txt --mode parem --threads 8
```

The exact interface may change as the implementation becomes clearer.

## Current Status

Current repository status:

- project topic chosen: parallel regex matching
- main references identified: Holub and Stekr, PaREM, SFA
- planned approach fixed: sequential DFA first, parallel CPU DFA second, benchmarks third, optimizations after correctness
- repository roadmap created in this README
- implementation not yet committed in this repository

The next commit should add the first minimal code structure and the sequential DFA baseline.

## References

- CSE305 project description, "Parallel text parsing"
- CSE305 course slides on lightweight threads, PRAM, CUDA, mutual exclusion, condition variables, and concurrent data structures
- Jan Holub and Stanislav Stekr, "On Parallel Implementations of Deterministic Finite Automata", 2009
- Suejb Memeti and Sabri Pllana, "PaREM: A Novel Approach for Parallel Regular Expression Matching", 2014
- Ryoma Sinya, Kiminori Matsuzaki, and Masataka Sassa, "Simultaneous Finite Automata: An Efficient Data-Parallel Model for Regular Expression Matching", 2013
