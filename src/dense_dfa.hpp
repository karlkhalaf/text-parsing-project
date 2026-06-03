#ifndef DENSE_DFA_HPP
#define DENSE_DFA_HPP

#include "dfa.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

class DenseDfa {
public:
    static constexpr std::size_t invalid_state = static_cast<std::size_t>(-1);

    explicit DenseDfa(const Dfa& dfa);

    bool accepts(std::string_view text) const;
    std::size_t next_state(std::size_t from, char symbol) const;

    std::size_t state_count() const;
    std::size_t initial_state() const;
    bool is_final(std::size_t state) const;
    const std::vector<char>& alphabet() const;

private:
    std::size_t symbol_index(char symbol) const;
    void check_state(std::size_t state) const;

    std::size_t initial_state_;
    std::vector<bool> final_states_;
    std::vector<char> alphabet_;
    std::vector<std::size_t> symbol_to_index_;
    std::vector<std::vector<std::size_t>> transitions_by_symbol_;
};

#endif
