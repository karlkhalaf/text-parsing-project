#ifndef DFA_HPP
#define DFA_HPP

#include <cstddef>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

class Dfa {
public:
    Dfa(std::size_t state_count, std::size_t initial_state);

    void set_final(std::size_t state, bool is_final = true);
    void add_transition(std::size_t from, char symbol, std::size_t to);

    bool accepts(std::string_view text) const;
    std::optional<std::size_t> next_state(std::size_t from, char symbol) const;

    std::size_t state_count() const;
    std::size_t initial_state() const;
    bool is_final(std::size_t state) const;
    std::vector<char> alphabet() const;

private:
    void check_state(std::size_t state) const;

    std::size_t initial_state_;
    std::vector<bool> final_states_;
    std::vector<std::unordered_map<char, std::size_t>> transitions_;
};

#endif
