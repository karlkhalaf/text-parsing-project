#include "nfa_builder.hpp"

#include <cassert>

void test_build_literal() {
    const Nfa nfa = build_nfa_from_pattern("a");

    assert(nfa.accepts("a"));
    assert(!nfa.accepts(""));
    assert(!nfa.accepts("b"));
}

void test_build_union() {
    const Nfa nfa = build_nfa_from_pattern("a|b");

    assert(nfa.accepts("a"));
    assert(nfa.accepts("b"));
    assert(!nfa.accepts("ab"));
}

void test_build_starred_group() {
    const Nfa nfa = build_nfa_from_pattern("(a|b)*");

    assert(nfa.accepts(""));
    assert(nfa.accepts("a"));
    assert(nfa.accepts("b"));
    assert(nfa.accepts("abab"));
    assert(!nfa.accepts("c"));
}

int main() {
    test_build_literal();
    test_build_union();
    test_build_starred_group();
    return 0;
}
