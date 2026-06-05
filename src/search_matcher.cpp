#include "search_matcher.hpp"

#include "dfa_builder.hpp"
#include "nfa.hpp"
#include "nfa_builder.hpp"

#include <array>
#include <vector>

namespace {

std::vector<char> collect_search_alphabet(const Nfa& pattern, std::string_view text) {
    std::array<bool, 256> seen{};

    for (char symbol : text) {
        seen[static_cast<unsigned char>(symbol)] = true;
    }
    for (std::size_t state = 0; state < pattern.state_count(); ++state) {
        for (const auto& edge : pattern.transitions_from(state)) {
            seen[static_cast<unsigned char>(edge.first)] = true;
        }
    }

    std::vector<char> alphabet;
    for (int c = 0; c < 256; ++c) {
        if (seen[c]) {
            alphabet.push_back(static_cast<char>(c));
        }
    }
    return alphabet;
}

}  // namespace

Dfa build_search_dfa_from_regex(const std::string& pattern, std::string_view text) {
    const Nfa pattern_nfa = build_nfa_from_pattern(pattern);
    const std::vector<char> alphabet = collect_search_alphabet(pattern_nfa, text);

    Nfa search;
    const std::size_t start = 0;
    const std::size_t offset = search.state_count();

    for (std::size_t i = 0; i < pattern_nfa.state_count(); ++i) {
        search.add_state();
    }

    for (std::size_t i = 0; i < pattern_nfa.state_count(); ++i) {
        for (const auto& edge : pattern_nfa.transitions_from(i)) {
            search.add_transition(offset + i, edge.first, offset + edge.second);
        }
        for (std::size_t target : pattern_nfa.epsilon_from(i)) {
            search.add_epsilon(offset + i, offset + target);
        }
    }

    const std::size_t end = search.add_state();

    // Sigma* prefix: from the start we may skip any character and try to start a
    // match at the next position, and we always enter the pattern through epsilon.
    search.add_epsilon(start, offset + pattern_nfa.initial_state());
    for (char symbol : alphabet) {
        search.add_transition(start, symbol, start);
    }

    // Once the pattern is matched we move to an absorbing accepting state and stay
    // there for the rest of the text (the Sigma* suffix), so a match is not lost.
    for (std::size_t i = 0; i < pattern_nfa.state_count(); ++i) {
        if (pattern_nfa.is_final(i)) {
            search.add_epsilon(offset + i, end);
        }
    }
    for (char symbol : alphabet) {
        search.add_transition(end, symbol, end);
    }

    search.set_initial(start);
    search.add_final(end);

    return build_dfa_from_nfa(search);
}
