#include "dense_dfa.hpp"
#include "matcher.hpp"

#include <cassert>

static void check_same_as_dfa(const char* pattern, const char* text) {
    const Dfa dfa = build_dfa_from_regex(pattern);
    const DenseDfa dense(dfa);

    assert(dfa.accepts(text) == dense.accepts(text));
}

static void test_manual_transition_table() {
    Dfa dfa(3, 0);
    dfa.add_transition(0, 'a', 1);
    dfa.add_transition(1, 'b', 2);
    dfa.set_final(2);

    const DenseDfa dense(dfa);

    assert(dense.state_count() == 3);
    assert(dense.initial_state() == 0);
    assert(dense.next_state(0, 'a') == 1);
    assert(dense.next_state(1, 'b') == 2);
    assert(dense.next_state(0, 'b') == DenseDfa::invalid_state);
    assert(dense.accepts("ab"));
    assert(!dense.accepts("a"));
}

int main() {
    test_manual_transition_table();

    check_same_as_dfa("a", "a");
    check_same_as_dfa("a", "");
    check_same_as_dfa("a|b", "b");
    check_same_as_dfa("(a|b)*", "abba");
    check_same_as_dfa("(a|b)*", "c");

    return 0;
}
