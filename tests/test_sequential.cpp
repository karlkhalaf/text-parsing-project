#include "matcher.hpp"

#include <cassert>

static void check(const char* pattern, const char* text, bool expected) {
    const Dfa dfa = build_dfa_from_regex(pattern);
    assert(dfa.accepts(text) == expected);
}

int main() {
    check("a", "", false);
    check("a", "a", true);
    check("a", "aa", false);

    check("a|b", "a", true);
    check("a|b", "b", true);
    check("a|b", "ab", false);

    check("(a|b)*", "", true);
    check("(a|b)*", "abba", true);
    check("(a|b)*", "c", false);

    return 0;
}
