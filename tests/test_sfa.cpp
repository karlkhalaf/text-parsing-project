#include "dense_dfa.hpp"
#include "matcher.hpp"
#include "sfa.hpp"

#include <cassert>

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

static void test_build_from_dfa() {
    Dfa dfa(3, 0);
    dfa.add_transition(0, 'a', 1);
    dfa.add_transition(1, 'b', 2);
    dfa.set_final(2);

    const Sfa sfa = Sfa::build_from_dfa(dfa);
    assert(sfa.sfa_state_count() > 0);
    assert(!sfa.is_final(sfa.initial_sfa_state()));
    assert(sfa.accepts("ab"));
    assert(!sfa.accepts("a"));
}

static void test_sfa_accepts_manual() {
    Dfa dfa(2, 0);
    dfa.add_transition(0, 'x', 1);
    dfa.set_final(1);

    const Sfa sfa = Sfa::build_from_dfa(dfa);
    assert(sfa.accepts("x"));
    assert(!sfa.accepts(""));
    assert(!sfa.accepts("y"));
}

static void check_sfa_matches_dfa(const char* pattern, const char* text) {
    const Dfa dfa = build_dfa_from_regex(pattern);
    const Sfa sfa = Sfa::build_from_dfa(dfa);
    assert(dfa.accepts(text) == sfa.accepts(text));
}

int main() {
    test_identity_and_apply_symbol();
    test_build_from_dfa();
    test_sfa_accepts_manual();
    check_sfa_matches_dfa("a", "a");
    check_sfa_matches_dfa("a|b", "b");
    check_sfa_matches_dfa("(a|b)*", "abba");
    check_sfa_matches_dfa("(a|b)*", "c");
    return 0;
}
