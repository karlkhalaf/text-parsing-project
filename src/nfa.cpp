#include "nfa.hpp"

#include <queue>
#include <stdexcept>
#include <unordered_set>

Nfa::Nfa() : initial_state_(0) {
    add_state();
}

std::size_t Nfa::add_state() {
    final_states_.push_back(false);
    transitions_.emplace_back();
    epsilon_transitions_.emplace_back();
    return transitions_.size() - 1;
}

void Nfa::set_initial(std::size_t state) {
    check_state(state);
    initial_state_ = state;
}

void Nfa::add_final(std::size_t state) {
    check_state(state);
    final_states_[state] = true;
}

void Nfa::add_transition(std::size_t from, char symbol, std::size_t to) {
    check_state(from);
    check_state(to);
    transitions_[from][symbol] = to;
}

void Nfa::add_epsilon(std::size_t from, std::size_t to) {
    check_state(from);
    check_state(to);
    epsilon_transitions_[from].push_back(to);
}

bool Nfa::accepts(std::string_view text) const {
    std::vector<std::size_t> current = epsilon_closure({initial_state_});

    for (char symbol : text) {
        current = move(current, symbol);
        if (current.empty()) {
            return false;
        }
        current = epsilon_closure(current);
    }

    for (std::size_t state : current) {
        if (is_final(state)) {
            return true;
        }
    }
    return false;
}

std::size_t Nfa::state_count() const {
    return transitions_.size();
}

std::size_t Nfa::initial_state() const {
    return initial_state_;
}

bool Nfa::is_final(std::size_t state) const {
    check_state(state);
    return final_states_[state];
}

void Nfa::check_state(std::size_t state) const {
    if (state >= transitions_.size()) {
        throw std::out_of_range("invalid NFA state");
    }
}

std::vector<std::size_t> Nfa::epsilon_closure(std::vector<std::size_t> states) const {
    std::unordered_set<std::size_t> seen(states.begin(), states.end());
    std::queue<std::size_t> queue;

    for (std::size_t state : states) {
        queue.push(state);
    }

    while (!queue.empty()) {
        const std::size_t state = queue.front();
        queue.pop();

        for (std::size_t next : epsilon_transitions_[state]) {
            if (seen.insert(next).second) {
                queue.push(next);
            }
        }
    }

    return std::vector<std::size_t>(seen.begin(), seen.end());
}

std::vector<std::size_t> Nfa::move(std::vector<std::size_t> states, char symbol) const {
    std::unordered_set<std::size_t> result;

    for (std::size_t state : states) {
        const auto it = transitions_[state].find(symbol);
        if (it != transitions_[state].end()) {
            result.insert(it->second);
        }
    }

    return std::vector<std::size_t>(result.begin(), result.end());
}

const std::unordered_map<char, std::size_t>& Nfa::transitions_from(std::size_t state) const {
    check_state(state);
    return transitions_[state];
}
const std::vector<std::size_t>& Nfa::epsilon_from(std::size_t state) const {
    check_state(state);
    return epsilon_transitions_[state];
}
