#include "sfa.hpp"

#include "dense_dfa.hpp"

#include <stdexcept>

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
    std::size_t dfa_initial_state,
    std::vector<SfaMapping> state_mappings,
    std::size_t initial_sfa_state,
    std::vector<bool> final_sfa_states,
    std::vector<char> alphabet,
    std::vector<std::size_t> symbol_to_index,
    std::vector<std::vector<std::size_t>> transitions_by_symbol
)
    : dfa_state_count_(dfa_state_count),
      dfa_initial_state_(dfa_initial_state),
      state_mappings_(std::move(state_mappings)),
      initial_sfa_state_(initial_sfa_state),
      final_sfa_states_(std::move(final_sfa_states)),
      alphabet_(std::move(alphabet)),
      symbol_to_index_(std::move(symbol_to_index)),
      transitions_by_symbol_(std::move(transitions_by_symbol)) {}

Sfa Sfa::build_from_dfa(const Dfa& /*dfa*/) {
    throw std::logic_error("Sfa::build_from_dfa is not implemented yet (phase 3)");
}

bool Sfa::accepts(std::string_view /*text*/) const {
    throw std::logic_error("Sfa::accepts is not implemented yet (phase 4)");
}

bool Sfa::accepts_parallel(std::string_view /*text*/, std::size_t /*thread_count*/) const {
    throw std::logic_error("Sfa::accepts_parallel is not implemented yet (phase 5)");
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
