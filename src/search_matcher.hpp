#ifndef SEARCH_MATCHER_HPP
#define SEARCH_MATCHER_HPP

#include "dfa.hpp"

#include <string>
#include <string_view>

// Builds a DFA that accepts a full text exactly when the regex pattern occurs
// somewhere inside it. It recognizes the language Sigma* L(pattern) Sigma*, so
// the existing full-text matchers (sequential, parallel, pruned, sfa) can be
// reused without changes to decide substring search.
//
// The alphabet Sigma is taken from the text and the pattern, so characters that
// are not part of the pattern (spaces, punctuation, log noise) do not stop the
// scan: the search keeps looking for a match at later positions.
Dfa build_search_dfa_from_regex(const std::string& pattern, std::string_view text);

#endif
