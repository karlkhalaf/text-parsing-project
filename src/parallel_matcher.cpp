#include "parallel_matcher.hpp"

#include "dense_dfa.hpp"

#include <algorithm>
#include <thread>
#include <vector>

namespace {

constexpr std::size_t PAREM_BOUNDARY_DEPTH = 3;

std::vector<std::string_view> split_text(std::string_view text, std::size_t chunk_count) {
    std::vector<std::string_view> chunks;
    if (chunk_count == 0) {
        return chunks;
    }

    chunks.reserve(chunk_count);
    const std::size_t n = text.size();
    std::size_t start = 0;

    for (std::size_t i = 0; i < chunk_count; ++i) {
        const std::size_t remaining_chunks = chunk_count - i;
        const std::size_t remaining_chars = n - start;
        const std::size_t len = remaining_chars / remaining_chunks;

        chunks.push_back(text.substr(start, len));
        start += len;
    }

    return chunks;
}

std::vector<std::size_t> all_states(const DenseDfa& dfa) {
    std::vector<std::size_t> states;
    states.reserve(dfa.state_count());

    for (std::size_t state = 0; state < dfa.state_count(); ++state) {
        states.push_back(state);
    }

    return states;
}

std::size_t apply_mappings_to_state(
    const std::vector<ChunkMapping>& mappings,
    std::size_t initial_state
) {
    std::size_t current = initial_state;

    for (const ChunkMapping& mapping : mappings) {
        if (current >= mapping.size() || mapping[current] == INVALID_STATE) {
            return INVALID_STATE;
        }
        current = mapping[current];
    }

    return current;
}

std::size_t walk_from_state(const DenseDfa& dfa, std::size_t state, std::string_view text) {
    std::size_t current = state;
    for (char symbol : text) {
        current = dfa.next_state(current, symbol);
        if (current == DenseDfa::invalid_state) {
            return DenseDfa::invalid_state;
        }
    }
    return current;
}

void mark_reachable_after_suffix(
    const DenseDfa& dfa,
    std::string_view suffix,
    std::vector<char>& reachable
) {
    reachable.assign(dfa.state_count(), false);
    for (std::size_t state = 0; state < dfa.state_count(); ++state) {
        const std::size_t end = walk_from_state(dfa, state, suffix);
        if (end != DenseDfa::invalid_state) {
            reachable[end] = true;
        }
    }
}

void mark_compatible_with_prefix(
    const DenseDfa& dfa,
    std::string_view prefix,
    std::vector<char>& compatible
) {
    compatible.assign(dfa.state_count(), false);
    for (std::size_t state = 0; state < dfa.state_count(); ++state) {
        if (walk_from_state(dfa, state, prefix) != DenseDfa::invalid_state) {
            compatible[state] = true;
        }
    }
}

ChunkMapping simulate_chunk_dense(const DenseDfa& dfa, std::string_view chunk) {
    ChunkMapping mapping(dfa.state_count(), INVALID_STATE);
    for (std::size_t start = 0; start < mapping.size(); ++start) {
        mapping[start] = start;
    }
    for (char symbol : chunk) {
        for (std::size_t start = 0; start < mapping.size(); ++start) {
            const std::size_t current = mapping[start];
            if (current == DenseDfa::invalid_state) {
                continue;
            }
            const std::size_t next = dfa.next_state(current, symbol);
            mapping[start] = next;
        }
    }
    return mapping;
}

ChunkMapping simulate_chunk_for_states_dense(
    const DenseDfa& dfa,
    std::string_view chunk,
    const std::vector<std::size_t>& start_states
) {
    ChunkMapping mapping(dfa.state_count(), INVALID_STATE);
    for (std::size_t start : start_states) {
        if (start < mapping.size()) {
            mapping[start] = start;
        }
    }
    for (char symbol : chunk) {
        for (std::size_t start : start_states) {
            if (start >= mapping.size()) {
                continue;
            }
            const std::size_t current = mapping[start];
            if (current == DenseDfa::invalid_state) {
                continue;
            }
            mapping[start] = dfa.next_state(current, symbol);
        }
    }
    return mapping;
}

std::vector<std::size_t> candidate_states_for_chunk_dense(
    const DenseDfa& dfa,
    std::string_view previous_chunk,
    std::string_view chunk,
    bool is_first_chunk
) {
    if (chunk.empty()) {
        return all_states(dfa);
    }

    if (is_first_chunk) {
        return {dfa.initial_state()};
    }

    if (previous_chunk.empty()) {
        return all_states(dfa);
    }

    const std::size_t suffix_len = std::min(PAREM_BOUNDARY_DEPTH, previous_chunk.size());
    const std::size_t prefix_len = std::min(PAREM_BOUNDARY_DEPTH, chunk.size());
    const std::string_view previous_suffix =
        previous_chunk.substr(previous_chunk.size() - suffix_len, suffix_len);
    const std::string_view current_prefix = chunk.substr(0, prefix_len);

    std::vector<char> reachable_from_previous_suffix(dfa.state_count(), false);
    std::vector<char> compatible_with_current_prefix(dfa.state_count(), false);
    mark_reachable_after_suffix(dfa, previous_suffix, reachable_from_previous_suffix);
    mark_compatible_with_prefix(dfa, current_prefix, compatible_with_current_prefix);

    std::vector<std::size_t> candidates;
    for (std::size_t state = 0; state < dfa.state_count(); ++state) {
        if (compatible_with_current_prefix[state] && reachable_from_previous_suffix[state]) {
            candidates.push_back(state);
        }
    }

    return candidates;
}

RouteVector simulate_route_for_states_dense(
    const DenseDfa& dfa,
    std::string_view chunk,
    const std::vector<std::size_t>& start_states
) {
    const ChunkMapping mapping = simulate_chunk_for_states_dense(dfa, chunk, start_states);
    return RouteVector::from_chunk_mapping(mapping, start_states);
}

std::size_t apply_routes_to_state_dense(
    const std::vector<RouteVector>& routes,
    std::size_t initial_state
) {
    if (routes.empty()) {
        return initial_state;
    }

    RouteVector accumulated = routes.front();
    for (std::size_t i = 1; i < routes.size(); ++i) {
        accumulated = routes[i].compose_with(accumulated);
    }

    return accumulated.apply(initial_state);
}

}  // namespace

ChunkMapping simulate_chunk(const Dfa& dfa, std::string_view chunk) {
    const DenseDfa dense(dfa);
    return simulate_chunk_dense(dense, chunk);
}

ChunkMapping simulate_chunk_for_states(
    const Dfa& dfa,
    std::string_view chunk,
    const std::vector<std::size_t>& start_states
) {
    const DenseDfa dense(dfa);
    return simulate_chunk_for_states_dense(dense, chunk, start_states);
}

std::vector<std::size_t> candidate_states_for_chunk(
    const Dfa& dfa,
    std::string_view previous_chunk,
    std::string_view chunk,
    bool is_first_chunk
) {
    const DenseDfa dense(dfa);
    return candidate_states_for_chunk_dense(dense, previous_chunk, chunk, is_first_chunk);
}

RouteVector simulate_route_for_states(
    const Dfa& dfa,
    std::string_view chunk,
    const std::vector<std::size_t>& start_states
) {
    const DenseDfa dense(dfa);
    return simulate_route_for_states_dense(dense, chunk, start_states);
}

std::size_t apply_routes_to_state(
    const std::vector<RouteVector>& routes,
    std::size_t initial_state
) {
    return apply_routes_to_state_dense(routes, initial_state);
}

std::size_t apply_routes_parallel(
    const std::vector<RouteVector>& routes,
    std::size_t initial_state
) {
    if (routes.empty()) {
        return initial_state;
    }
    return reduce_routes_parallel(routes).apply(initial_state);
}

ChunkMapping compose_mappings(const ChunkMapping& left, const ChunkMapping& right) {
    ChunkMapping composed(left.size(), INVALID_STATE);

    for (std::size_t start = 0; start < left.size(); ++start) {
        const std::size_t mid = left[start];
        if (mid == INVALID_STATE) {
            continue;
        }
        if (mid >= right.size() || right[mid] == INVALID_STATE) {
            continue;
        }
        composed[start] = right[mid];
    }

    return composed;
}

bool parallel_accepts(const Dfa& dfa, std::string_view text, std::size_t chunk_count) {
    if (chunk_count == 0) {
        return false;
    }
    const DenseDfa dense(dfa);
    if (chunk_count == 1) {
        return dense.accepts(text);
    }

    const std::vector<std::string_view> chunks = split_text(text, chunk_count);
    if (chunks.empty()) {
        return dense.is_final(dense.initial_state());
    }

    std::vector<ChunkMapping> mappings;
    mappings.reserve(chunks.size());
    for (std::string_view chunk : chunks) {
        mappings.push_back(simulate_chunk_dense(dense, chunk));
    }

    const std::size_t end_state = apply_mappings_to_state(mappings, dense.initial_state());
    if (end_state == INVALID_STATE) {
        return false;
    }
    return dense.is_final(end_state);
}

bool parallel_accepts_pruned(const Dfa& dfa, std::string_view text, std::size_t chunk_count) {
    if (chunk_count == 0) {
        return false;
    }
    const DenseDfa dense(dfa);
    if (chunk_count == 1) {
        return dense.accepts(text);
    }

    const std::vector<std::string_view> chunks = split_text(text, chunk_count);
    if (chunks.empty()) {
        return dense.is_final(dense.initial_state());
    }

    std::vector<RouteVector> routes;
    routes.reserve(chunks.size());
    routes.push_back(simulate_route_for_states_dense(
        dense,
        chunks[0],
        candidate_states_for_chunk_dense(dense, "", chunks[0], true)
    ));

    for (std::size_t i = 1; i < chunks.size(); ++i) {
        const std::vector<std::size_t> candidates =
            candidate_states_for_chunk_dense(dense, chunks[i - 1], chunks[i], false);

        routes.push_back(simulate_route_for_states_dense(dense, chunks[i], candidates));
    }

    const std::size_t end_state = apply_routes_to_state_dense(routes, dense.initial_state());
    if (end_state == INVALID_STATE) {
        return false;
    }
    return dense.is_final(end_state);
}


bool parallel_accepts_threads(const Dfa& dfa, std::string_view text, std::size_t thread_count) {
    if (thread_count == 0) {
        return false;
    }
    const DenseDfa dense(dfa);
    if (thread_count == 1) {
        return dense.accepts(text);
    }

    const std::vector<std::string_view> chunks = split_text(text, thread_count);
    if (chunks.empty()) {
        return dense.is_final(dense.initial_state());
    }

    std::vector<ChunkMapping> mappings(chunks.size());
    std::vector<std::thread> workers;
    workers.reserve(chunks.size());

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        workers.emplace_back([&dense, &chunks, &mappings, i]() {
            mappings[i] = simulate_chunk_dense(dense, chunks[i]);
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    const std::size_t end_state = apply_mappings_to_state(mappings, dense.initial_state());
    if (end_state == INVALID_STATE) {
        return false;
    }
    return dense.is_final(end_state);
}

bool parallel_accepts_pruned_threads(
    const Dfa& dfa,
    std::string_view text,
    std::size_t thread_count
) {
    if (thread_count == 0) {
        return false;
    }
    const DenseDfa dense(dfa);
    if (thread_count == 1) {
        return dense.accepts(text);
    }

    const std::vector<std::string_view> chunks = split_text(text, thread_count);
    if (chunks.empty()) {
        return dense.is_final(dense.initial_state());
    }

    std::vector<RouteVector> routes(chunks.size(), RouteVector(dense.state_count()));
    std::vector<std::thread> workers;
    workers.reserve(chunks.size());

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        workers.emplace_back([&dense, &chunks, &routes, i]() {
            const bool is_first_chunk = (i == 0);
            const std::string_view previous_chunk =
                is_first_chunk ? std::string_view() : chunks[i - 1];

            const std::vector<std::size_t> candidates =
                candidate_states_for_chunk_dense(dense, previous_chunk, chunks[i], is_first_chunk);

            routes[i] = simulate_route_for_states_dense(dense, chunks[i], candidates);
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    const std::size_t end_state = apply_routes_parallel(routes, dense.initial_state());
    if (end_state == INVALID_STATE) {
        return false;
    }
    return dense.is_final(end_state);
}
