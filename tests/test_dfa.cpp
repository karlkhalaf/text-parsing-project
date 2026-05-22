#include "dfa.hpp"

#include <cassert>

int main() {
    Dfa dfa(3, 0);
    dfa.add_transition(0, 'a', 1);
    dfa.add_transition(1, 'b', 2);
    dfa.set_final(2);

    assert(dfa.accepts("ab"));
    assert(!dfa.accepts(""));
    assert(!dfa.accepts("a"));
    assert(!dfa.accepts("abc"));
    assert(!dfa.accepts("ac"));

    return 0;
}
