#include "parallel_matcher.hpp"

#include <optional>
#include <thread>
#include <vector>

namespace {

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

    std::vector<std::size_t> all_states(const Dfa& dfa) {
        std::vector<std::size_t> states;
        states.reserve(dfa.state_count());

        for (std::size_t state = 0; state < dfa.state_count(); ++state) {
            states.push_back(state);
        }

        return states;
    }

}  // namespace

ChunkMapping simulate_chunk(const Dfa& dfa, std::string_view chunk) {
    ChunkMapping mapping(dfa.state_count(), INVALID_STATE);

    for (std::size_t start = 0; start < dfa.state_count(); ++start) {
        std::size_t current = start;
        bool ok = true;

        for (char symbol : chunk) {
            const std::optional<std::size_t> next = dfa.next_state(current, symbol);
            if (!next.has_value()) {
                ok = false;
                break;
            }
            current = *next;
        }

        if (ok) {
            mapping[start] = current;
        }
    }

    return mapping;
}

ChunkMapping simulate_chunk_for_states(
    const Dfa& dfa,
    std::string_view chunk,
    const std::vector<std::size_t>& start_states
) {
    ChunkMapping mapping(dfa.state_count(), INVALID_STATE);

    for (std::size_t start : start_states) {
        if (start >= dfa.state_count()) {
            continue;
        }

        std::size_t current = start;
        bool ok = true;

        for (char symbol : chunk) {
            const std::optional<std::size_t> next = dfa.next_state(current, symbol);
            if (!next.has_value()) {
                ok = false;
                break;
            }
            current = *next;
        }

        if (ok) {
            mapping[start] = current;
        }
    }

    return mapping;
}

std::vector<std::size_t> candidate_states_for_chunk(
    const Dfa& dfa,
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

    const char previous_last = previous_chunk.back();
    const char current_first = chunk.front();

    std::vector<std::size_t> reached_after_previous;
    std::vector<std::size_t> can_read_current;

    for (std::size_t state = 0; state < dfa.state_count(); ++state) {
        const std::optional<std::size_t> after_previous = dfa.next_state(state, previous_last);
        if (after_previous.has_value()) {
            reached_after_previous.push_back(*after_previous);
        }

        if (dfa.next_state(state, current_first).has_value()) {
            can_read_current.push_back(state);
        }
    }

    std::vector<std::size_t> candidates;
    for (std::size_t state : can_read_current) {
        bool seen = false;
        for (std::size_t reached : reached_after_previous) {
            if (state == reached) {
                seen = true;
                break;
            }
        }

        if (seen) {
            candidates.push_back(state);
        }
    }

    if (candidates.empty()) {
        return all_states(dfa);
    }

    return candidates;
}
ChunkMapping simulate_chunk_for_states(
    const Dfa& dfa,
    std::string_view chunk,
    const std::vector<std::size_t>& start_states
) {
    ChunkMapping mapping(dfa.state_count(), INVALID_STATE);

    for (std::size_t start : start_states) {
        if (start >= dfa.state_count()) {
            continue;
        }

        std::size_t current = start;
        bool ok = true;

        for (char symbol : chunk) {
            const std::optional<std::size_t> next = dfa.next_state(current, symbol);
            if (!next.has_value()) {
                ok = false;
                break;
            }
            current = *next;
        }

        if (ok) {
            mapping[start] = current;
        }
    }

    return mapping;
}

std::vector<std::size_t> candidate_states_for_chunk(
    const Dfa& dfa,
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

    const char previous_last = previous_chunk.back();
    const char current_first = chunk.front();

    std::vector<std::size_t> reached_after_previous;
    std::vector<std::size_t> can_read_current;

    for (std::size_t state = 0; state < dfa.state_count(); ++state) {
        const std::optional<std::size_t> after_previous = dfa.next_state(state, previous_last);
        if (after_previous.has_value()) {
            reached_after_previous.push_back(*after_previous);
        }

        if (dfa.next_state(state, current_first).has_value()) {
            can_read_current.push_back(state);
        }
    }

    std::vector<std::size_t> candidates;
    for (std::size_t state : can_read_current) {
        bool seen = false;
        for (std::size_t reached : reached_after_previous) {
            if (state == reached) {
                seen = true;
                break;
            }
        }

        if (seen) {
            candidates.push_back(state);
        }
    }

    if (candidates.empty()) {
        return all_states(dfa);
    }

    return candidates;
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
    if (chunk_count == 1) {
        return dfa.accepts(text);
    }

    const std::vector<std::string_view> chunks = split_text(text, chunk_count);
    if (chunks.empty()) {
        return dfa.is_final(dfa.initial_state());
    }

    ChunkMapping total = simulate_chunk(dfa, chunks[0]);
    for (std::size_t i = 1; i < chunks.size(); ++i) {
        total = compose_mappings(total, simulate_chunk(dfa, chunks[i]));
    }

    const std::size_t end_state = total[dfa.initial_state()];
    if (end_state == INVALID_STATE) {
        return false;
    }
    return dfa.is_final(end_state);
}

bool parallel_accepts_pruned(const Dfa& dfa, std::string_view text, std::size_t chunk_count) {
    if (chunk_count == 0) {
        return false;
    }
    if (chunk_count == 1) {
        return dfa.accepts(text);
    }

    const std::vector<std::string_view> chunks = split_text(text, chunk_count);
    if (chunks.empty()) {
        return dfa.is_final(dfa.initial_state());
    }

    ChunkMapping total = simulate_chunk_for_states(
        dfa,
        chunks[0],
        candidate_states_for_chunk(dfa, "", chunks[0], true)
    );

    for (std::size_t i = 1; i < chunks.size(); ++i) {
        const std::vector<std::size_t> candidates =
            candidate_states_for_chunk(dfa, chunks[i - 1], chunks[i], false);

        total = compose_mappings(total, simulate_chunk_for_states(dfa, chunks[i], candidates));
    }

    const std::size_t end_state = total[dfa.initial_state()];
    if (end_state == INVALID_STATE) {
        return false;
    }
    return dfa.is_final(end_state);
}


bool parallel_accepts_threads(const Dfa& dfa, std::string_view text, std::size_t thread_count) {
    if (thread_count == 0) {
        return false;
    }
    if (thread_count == 1) {
        return dfa.accepts(text);
    }

    const std::vector<std::string_view> chunks = split_text(text, thread_count);
    if (chunks.empty()) {
        return dfa.is_final(dfa.initial_state());
    }

    std::vector<ChunkMapping> mappings(chunks.size());
    std::vector<std::thread> workers;
    workers.reserve(chunks.size());

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        workers.emplace_back([&dfa, &chunks, &mappings, i]() {
            mappings[i] = simulate_chunk(dfa, chunks[i]);
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    ChunkMapping total = mappings[0];
    for (std::size_t i = 1; i < mappings.size(); ++i) {
        total = compose_mappings(total, mappings[i]);
    }

    const std::size_t end_state = total[dfa.initial_state()];
    if (end_state == INVALID_STATE) {
        return false;
    }
    return dfa.is_final(end_state);
}
