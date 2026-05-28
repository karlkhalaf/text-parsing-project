#include "matcher.hpp"

#include "dfa_builder.hpp"
#include "nfa_builder.hpp"

Dfa build_dfa_from_regex(const std::string& pattern) {
    const Nfa nfa = build_nfa_from_pattern(pattern);
    return build_dfa_from_nfa(nfa);
}

