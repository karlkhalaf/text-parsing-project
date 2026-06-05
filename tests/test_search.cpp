#include "parallel_matcher.hpp"
#include "search_matcher.hpp"
#include "sfa.hpp"

#include <cassert>

// On the search DFA, full-text acceptance means "the pattern occurs somewhere in
// the text". We check that every mode agrees with the sequential baseline and
// with the expected substring-search result.
static void check_search(const char* pattern, const char* text, bool expected) {
    const Dfa dfa = build_search_dfa_from_regex(pattern, text);

    const bool sequential = dfa.accepts(text);
    assert(sequential == expected);

    for (std::size_t threads : {1u, 2u, 3u, 4u}) {
        assert(parallel_accepts_threads(dfa, text, threads) == expected);
        assert(parallel_accepts_pruned_threads(dfa, text, threads) == expected);
    }

    const Sfa sfa = Sfa::build_from_dfa(dfa);
    assert(sfa.accepts(text) == expected);
    for (std::size_t threads : {1u, 2u, 3u, 4u}) {
        assert(sfa.accepts_parallel(text, threads) == expected);
    }
}

int main() {
    // Literal pattern found / not found inside surrounding noise.
    check_search("ab", "xxabyy", true);
    check_search("ab", "acac", false);
    check_search("ab", "a---b", false);

    // Union matches a single character anywhere.
    check_search("a|b", "xxbxx", true);
    check_search("a|b", "zzzz", false);

    // Larger expression with a required suffix.
    check_search("(a|b)*abb", "xxaababbxx", true);
    check_search("(a|b)*abb", "cccc", false);

    // Match at the very start and at the very end of the text.
    check_search("ab", "abxxxx", true);
    check_search("ab", "xxxxab", true);

    return 0;
}
