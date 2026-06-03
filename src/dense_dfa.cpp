#include "dense_dfa.hpp"

#include <stdexcept>

DenseDfa::DenseDfa(const Dfa& dfa)
    : initial_state_(dfa.initial_state()),
      final_states_(dfa.state_count(), false),
      alphabet_(dfa.alphabet()),
      symbol_to_index_(256, invalid_state),
      transitions_by_symbol_(
          alphabet_.size(),
          std::vector<std::size_t>(dfa.state_count(), invalid_state)
      ) {
    for (std::size_t state = 0; state < dfa.state_count(); ++state) {
        final_states_[state] = dfa.is_final(state);
    }

    for (std::size_t i = 0; i < alphabet_.size(); ++i) {
        const unsigned char symbol = static_cast<unsigned char>(alphabet_[i]);
        symbol_to_index_[symbol] = i;
    }

    for (std::size_t i = 0; i < alphabet_.size(); ++i) {
        const char symbol = alphabet_[i];
        for (std::size_t state = 0; state < dfa.state_count(); ++state) {
            const auto next = dfa.next_state(state, symbol);
            if (next.has_value()) {
                transitions_by_symbol_[i][state] = *next;
            }
        }
    }
}

bool DenseDfa::accepts(std::string_view text) const {
    std::size_t current = initial_state_;

    for (char symbol : text) {
        current = next_state(current, symbol);
        if (current == invalid_state) {
            return false;
        }
    }

    return is_final(current);
}

std::size_t DenseDfa::next_state(std::size_t from, char symbol) const {
    check_state(from);
    const std::size_t index = symbol_index(symbol);
    if (index == invalid_state) {
        return invalid_state;
    }
    return transitions_by_symbol_[index][from];
}

std::size_t DenseDfa::state_count() const {
    return final_states_.size();
}

std::size_t DenseDfa::initial_state() const {
    return initial_state_;
}

bool DenseDfa::is_final(std::size_t state) const {
    check_state(state);
    return final_states_[state];
}

const std::vector<char>& DenseDfa::alphabet() const {
    return alphabet_;
}

std::size_t DenseDfa::symbol_index(char symbol) const {
    return symbol_to_index_[static_cast<unsigned char>(symbol)];
}

void DenseDfa::check_state(std::size_t state) const {
    if (state >= final_states_.size()) {
        throw std::out_of_range("invalid DFA state");
    }
}
