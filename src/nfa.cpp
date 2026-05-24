#include "nfa.hpp"

#include <stdexcept>

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
