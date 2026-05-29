#include "parallel_matcher.hpp"

#include <optional>
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
