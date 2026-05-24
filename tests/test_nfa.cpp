#include "nfa.hpp"

#include <cassert>

static Nfa build_a_or_b_nfa() {
    Nfa nfa;

    const std::size_t s0 = 0;
    const std::size_t s1 = nfa.add_state();
    const std::size_t s2 = nfa.add_state();
    const std::size_t s3 = nfa.add_state();

    nfa.set_initial(s0);
    nfa.add_final(s3);

    nfa.add_epsilon(s0, s1);
    nfa.add_epsilon(s0, s2);
    nfa.add_transition(s1, 'a', s3);
    nfa.add_transition(s2, 'b', s3);

    return nfa;
}

void test_accepts_single_char_from_union() {
    const Nfa nfa = build_a_or_b_nfa();

    assert(nfa.accepts("a"));
    assert(nfa.accepts("b"));
    assert(!nfa.accepts(""));
    assert(!nfa.accepts("ab"));
    assert(!nfa.accepts("c"));
}

int main() {
    test_accepts_single_char_from_union();
    return 0;
}
