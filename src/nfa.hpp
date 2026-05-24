#ifndef NFA_HPP
#define NFA_HPP

#include <cstddef>
#include <unordered_map>
#include <vector>

class Nfa {
public:
    Nfa();

    std::size_t add_state();
    void set_initial(std::size_t state);
    void add_final(std::size_t state);
    void add_transition(std::size_t from, char symbol, std::size_t to);
    void add_epsilon(std::size_t from, std::size_t to);

    std::size_t state_count() const;
    std::size_t initial_state() const;
    bool is_final(std::size_t state) const;

private:
    void check_state(std::size_t state) const;

    std::size_t initial_state_;
    std::vector<bool> final_states_;
    std::vector<std::unordered_map<char, std::size_t>> transitions_;
    std::vector<std::vector<std::size_t>> epsilon_transitions_;
};

#endif
