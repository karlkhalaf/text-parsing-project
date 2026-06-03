#include "dense_dfa.hpp"
#include "sfa.hpp"

#include <cassert>
#include <stdexcept>

static void test_identity_and_apply_symbol() {
    Dfa dfa(3, 0);
    dfa.add_transition(0, 'a', 1);
    dfa.add_transition(1, 'b', 2);
    dfa.set_final(2);

    const DenseDfa dense(dfa);
    const SfaMapping identity = make_identity_mapping(dfa.state_count());

    assert(same_mapping(identity, make_identity_mapping(3)));
    assert(identity[0] == 0);
    assert(identity[1] == 1);

    const SfaMapping after_a = apply_symbol_to_mapping(dense, identity, 'a');
    assert(after_a[0] == 1);
    assert(after_a[1] == DenseDfa::invalid_state);

    const SfaMapping after_ab = apply_symbol_to_mapping(dense, after_a, 'b');
    assert(after_ab[0] == 2);
    assert(dense.accepts("ab") == (after_ab[0] == 2 && dense.is_final(2)));
}

static void test_build_stub_throws() {
    const Dfa dfa(1, 0);
    bool threw = false;
    try {
        (void)Sfa::build_from_dfa(dfa);
    } catch (const std::logic_error&) {
        threw = true;
    }
    assert(threw);
}

int main() {
    test_identity_and_apply_symbol();
    test_build_stub_throws();
    return 0;
}
