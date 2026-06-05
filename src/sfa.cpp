#include "sfa.hpp"

#include "dense_dfa.hpp"

#include <queue>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace {

struct SfaMappingHash {
    std::size_t operator()(const SfaMapping& mapping) const {
        std::size_t hash = mapping.size();
        for (const std::size_t value : mapping) {
            hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        return hash;
    }
};

std::size_t lookup_or_add_state(
    const SfaMapping& mapping,
    std::vector<SfaMapping>& state_mappings,
    std::vector<bool>& final_sfa_states,
    std::unordered_map<SfaMapping, std::size_t, SfaMappingHash>& index_of,
    const Dfa& dfa,
    const DenseDfa& dense
) {
    const auto found = index_of.find(mapping);
    if (found != index_of.end()) {
        return found->second;
    }

    const std::size_t id = state_mappings.size();
    state_mappings.push_back(mapping);

    const std::size_t mapped_initial = mapping[dfa.initial_state()];
    const bool is_final =
        mapped_initial != DenseDfa::invalid_state && dense.is_final(mapped_initial);
    final_sfa_states.push_back(is_final);

    index_of[mapping] = id;
    return id;
}

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

SfaMapping compose_sfa_mappings(const SfaMapping& left, const SfaMapping& right) {
    SfaMapping composed(left.size(), DenseDfa::invalid_state);

    for (std::size_t state = 0; state < left.size(); ++state) {
        const std::size_t middle = left[state];
        if (middle == DenseDfa::invalid_state) {
            continue;
        }
        composed[state] = right[middle];
    }

    return composed;
}

bool mapping_is_accepting(const Sfa& sfa, const SfaMapping& mapping) {
    for (std::size_t sfa_state = 0; sfa_state < sfa.sfa_state_count(); ++sfa_state) {
        if (sfa.is_final(sfa_state) && same_mapping(sfa.mapping_for_state(sfa_state), mapping)) {
            return true;
        }
    }

    return false;
}

}  // namespace

SfaMapping make_identity_mapping(std::size_t dfa_state_count) {
    SfaMapping mapping(dfa_state_count, DenseDfa::invalid_state);
    for (std::size_t state = 0; state < dfa_state_count; ++state) {
        mapping[state] = state;
    }
    return mapping;
}

SfaMapping apply_symbol_to_mapping(
    const DenseDfa& dfa,
    const SfaMapping& mapping,
    char symbol
) {
    if (mapping.size() != dfa.state_count()) {
        throw std::invalid_argument("SFA mapping size does not match DFA state count");
    }

    SfaMapping next(mapping.size(), DenseDfa::invalid_state);
    for (std::size_t state = 0; state < mapping.size(); ++state) {
        const std::size_t from = mapping[state];
        if (from == DenseDfa::invalid_state) {
            continue;
        }
        next[state] = dfa.next_state(from, symbol);
    }
    return next;
}

bool same_mapping(const SfaMapping& left, const SfaMapping& right) {
    return left == right;
}

Sfa::Sfa(
    std::size_t dfa_state_count,
    std::vector<SfaMapping> state_mappings,
    std::size_t initial_sfa_state,
    std::vector<bool> final_sfa_states,
    std::vector<char> alphabet,
    std::vector<std::size_t> symbol_to_index,
    std::vector<std::vector<std::size_t>> transitions_by_symbol
)
    : dfa_state_count_(dfa_state_count),
      state_mappings_(std::move(state_mappings)),
      initial_sfa_state_(initial_sfa_state),
      final_sfa_states_(std::move(final_sfa_states)),
      alphabet_(std::move(alphabet)),
      symbol_to_index_(std::move(symbol_to_index)),
      transitions_by_symbol_(std::move(transitions_by_symbol)) {}

Sfa Sfa::build_from_dfa(const Dfa& dfa) {
    const DenseDfa dense(dfa);
    const std::vector<char>& alphabet = dense.alphabet();

    std::vector<std::size_t> symbol_to_index(256, invalid_sfa_state);
    for (std::size_t i = 0; i < alphabet.size(); ++i) {
        symbol_to_index[static_cast<unsigned char>(alphabet[i])] = i;
    }

    std::vector<SfaMapping> state_mappings;
    std::vector<bool> final_sfa_states;
    std::unordered_map<SfaMapping, std::size_t, SfaMappingHash> index_of;

    const SfaMapping identity = make_identity_mapping(dfa.state_count());
    const std::size_t initial_sfa_state = lookup_or_add_state(
        identity,
        state_mappings,
        final_sfa_states,
        index_of,
        dfa,
        dense
    );

    std::queue<std::size_t> pending;
    std::vector<bool> seen(state_mappings.size(), false);
    seen[initial_sfa_state] = true;
    pending.push(initial_sfa_state);

    while (!pending.empty()) {
        const std::size_t sfa_state = pending.front();
        pending.pop();

        const SfaMapping mapping = state_mappings[sfa_state];
        for (char symbol : alphabet) {
            const SfaMapping next_mapping = apply_symbol_to_mapping(dense, mapping, symbol);
            const std::size_t next_sfa_state = lookup_or_add_state(
                next_mapping,
                state_mappings,
                final_sfa_states,
                index_of,
                dfa,
                dense
            );

            if (next_sfa_state >= seen.size()) {
                seen.resize(next_sfa_state + 1, false);
            }
            if (!seen[next_sfa_state]) {
                seen[next_sfa_state] = true;
                pending.push(next_sfa_state);
            }
        }
    }

    const std::size_t sfa_state_count = state_mappings.size();
    std::vector<std::vector<std::size_t>> transitions_by_symbol(
        alphabet.size(),
        std::vector<std::size_t>(sfa_state_count, invalid_sfa_state)
    );

    for (std::size_t sfa_state = 0; sfa_state < sfa_state_count; ++sfa_state) {
        const SfaMapping& mapping = state_mappings[sfa_state];
        for (std::size_t symbol_index = 0; symbol_index < alphabet.size(); ++symbol_index) {
            const SfaMapping next_mapping =
                apply_symbol_to_mapping(dense, mapping, alphabet[symbol_index]);
            transitions_by_symbol[symbol_index][sfa_state] = index_of.at(next_mapping);
        }
    }

    return Sfa(
        dfa.state_count(),
        std::move(state_mappings),
        initial_sfa_state,
        std::move(final_sfa_states),
        alphabet,
        std::move(symbol_to_index),
        std::move(transitions_by_symbol)
    );
}

bool Sfa::accepts(std::string_view text) const {
    std::size_t current = initial_sfa_state_;

    for (char symbol : text) {
        current = next_sfa_state(current, symbol);
        if (current == invalid_sfa_state) {
            return false;
        }
    }

    return is_final(current);
}

bool Sfa::accepts_parallel(std::string_view text, std::size_t thread_count) const {
    if (thread_count == 0) {
        thread_count = 1;
    }
    if (text.empty()) {
        return accepts(text);
    }

    const std::vector<std::string_view> chunks = split_text(text, thread_count);
    if (chunks.empty()) {
        return accepts(text);
    }

    std::vector<std::size_t> chunk_end_states(chunks.size(), invalid_sfa_state);
    std::vector<std::thread> workers;
    workers.reserve(chunks.size());

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        workers.emplace_back([this, &chunks, &chunk_end_states, i]() {
            std::size_t current = initial_sfa_state_;
            for (char symbol : chunks[i]) {
                current = next_sfa_state(current, symbol);
                if (current == invalid_sfa_state) {
                    return;
                }
            }
            chunk_end_states[i] = current;
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    SfaMapping composed = make_identity_mapping(dfa_state_count_);
    for (std::size_t i = 0; i < chunk_end_states.size(); ++i) {
        if (chunk_end_states[i] == invalid_sfa_state) {
            return false;
        }
        composed = compose_sfa_mappings(composed, mapping_for_state(chunk_end_states[i]));
    }

    return mapping_is_accepting(*this, composed);
}

std::size_t Sfa::sfa_state_count() const {
    return state_mappings_.size();
}

std::size_t Sfa::initial_sfa_state() const {
    return initial_sfa_state_;
}

bool Sfa::is_final(std::size_t sfa_state) const {
    check_sfa_state(sfa_state);
    return final_sfa_states_[sfa_state];
}

std::size_t Sfa::next_sfa_state(std::size_t from, char symbol) const {
    check_sfa_state(from);
    const std::size_t index = symbol_index(symbol);
    if (index == invalid_sfa_state) {
        return invalid_sfa_state;
    }
    return transitions_by_symbol_[index][from];
}

std::size_t Sfa::underlying_dfa_state_count() const {
    return dfa_state_count_;
}

const SfaMapping& Sfa::mapping_for_state(std::size_t sfa_state) const {
    check_sfa_state(sfa_state);
    return state_mappings_[sfa_state];
}

const std::vector<char>& Sfa::alphabet() const {
    return alphabet_;
}

std::size_t Sfa::symbol_index(char symbol) const {
    if (static_cast<unsigned char>(symbol) >= symbol_to_index_.size()) {
        return invalid_sfa_state;
    }
    return symbol_to_index_[static_cast<unsigned char>(symbol)];
}

void Sfa::check_sfa_state(std::size_t sfa_state) const {
    if (sfa_state >= state_mappings_.size()) {
        throw std::out_of_range("invalid SFA state");
    }
}
