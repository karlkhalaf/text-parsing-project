#ifndef NFA_BUILDER_HPP
#define NFA_BUILDER_HPP

#include "nfa.hpp"
#include "regex_parser.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct NfaFragment {
    std::size_t start;
    std::size_t end;
};

Nfa build_nfa_from_postfix(const std::vector<RegexToken>& postfix);
Nfa build_nfa_from_pattern(const std::string& pattern);

#endif
