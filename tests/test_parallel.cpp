#include "matcher.hpp"
#include "parallel_matcher.hpp"

#include <cassert>

static void check_same(const char* pattern, const char* text, std::size_t chunk_count) {
    const Dfa dfa = build_dfa_from_regex(pattern);
    assert(dfa.accepts(text) == parallel_accepts(dfa, text, chunk_count));
}

static void check_same_threads(const char* pattern, const char* text, std::size_t threads) {
    const Dfa dfa = build_dfa_from_regex(pattern);
    assert(dfa.accepts(text) == parallel_accepts_threads(dfa, text, threads));
}

int main() {
    check_same("a", "", 1);
    check_same("a", "", 4);
    check_same("a", "a", 1);
    check_same("a", "a", 3);

    check_same("a|b", "b", 1);
    check_same("a|b", "b", 2);
    check_same("a|b", "ab", 4);

    check_same("(a|b)*", "abba", 1);
    check_same("(a|b)*", "abba", 5);
    check_same("(a|b)*", "c", 3);

    check_same_threads("(a|b)*", "abba", 4);
    check_same_threads("(a|b)*", "c", 4);
    check_same_threads("a|b", "b", 2);

    return 0;
}
