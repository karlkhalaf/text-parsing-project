#include "dfa.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

Dfa::Dfa(std::size_t state_count, std::size_t initial_state)
    : initial_state_(initial_state),
      final_states_(state_count, false),
      transitions_(state_count) {
    if (state_count == 0) {
        throw std::invalid_argument("a DFA must have at least one state");
    }
    check_state(initial_state_);
}

void Dfa::set_final(std::size_t state, bool is_final) {
    check_state(state);
    final_states_[state] = is_final;
}

void Dfa::add_transition(std::size_t from, char symbol, std::size_t to) {
    check_state(from);
    check_state(to);
    transitions_[from][symbol] = to;
}

bool Dfa::accepts(std::string_view text) const {
    std::size_t current = initial_state_;

    for (char symbol : text) {
        std::optional<std::size_t> next = next_state(current, symbol);
        if (!next.has_value()) {
            return false;
        }
        current = *next;
    }

    return is_final(current);
}

std::optional<std::size_t> Dfa::next_state(std::size_t from, char symbol) const {
    check_state(from);
    const auto it = transitions_[from].find(symbol);
    if (it == transitions_[from].end()) {
        return std::nullopt;
    }
    return it->second;
}

std::size_t Dfa::state_count() const {
    return transitions_.size();
}

std::size_t Dfa::initial_state() const {
    return initial_state_;
}

bool Dfa::is_final(std::size_t state) const {
    check_state(state);
    return final_states_[state];
}

std::vector<char> Dfa::alphabet() const {
    std::unordered_set<char> symbols;

    for (const auto& state_transitions : transitions_) {
        for (const auto& transition : state_transitions) {
            symbols.insert(transition.first);
        }
    }

    std::vector<char> result(symbols.begin(), symbols.end());
    std::sort(result.begin(), result.end());
    return result;
}

void Dfa::check_state(std::size_t state) const {
    if (state >= transitions_.size()) {
        throw std::out_of_range("invalid DFA state");
    }
}
