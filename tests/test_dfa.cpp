#include "dfa.hpp"

#include <cassert>

void test_accepts_exact_word() {
    Dfa dfa(3, 0);
    dfa.add_transition(0, 'a', 1);
    dfa.add_transition(1, 'b', 2);
    dfa.set_final(2);

    assert(dfa.accepts("ab"));
    assert(!dfa.accepts(""));
    assert(!dfa.accepts("a"));
    assert(!dfa.accepts("abc"));
    assert(!dfa.accepts("ac"));
}

void test_accepts_a_star() {
    Dfa dfa(1, 0);
    dfa.set_final(0);
    dfa.add_transition(0, 'a', 0);

    assert(dfa.accepts(""));
    assert(dfa.accepts("a"));
    assert(dfa.accepts("aaaa"));
    assert(!dfa.accepts("b"));
    assert(!dfa.accepts("aaab"));
}

int main() {
    test_accepts_exact_word();
    test_accepts_a_star();

    return 0;
}
